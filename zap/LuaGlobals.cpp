//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LuaModule.h"
#include "LuaBase.h"
#include "stringUtils.h"
#include "GameSettings.h"
#include "Console.h"
#include "version.h"

#include <tnl.h>
#include <tnlLog.h>
#include <tnlRandom.h>


using namespace TNL;

namespace Zap
{

using namespace LuaArgs;

// helper function
static string buildPrintString(lua_State *L)
{
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  string out;

  lua_getglobal(L, "tostring");
  for(i = 1; i <= n; i++) 
  {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
    if(i > 1) 
       out += "\t";

    out += s;
    lua_pop(L, 1);  /* pop result */
  }

  return out;
}

/**
 * @luafunc static void global::logprint(any val)
 * 
 * @brief Print to bitfighter's logging engine.
 * 
 * @descr This will (currently) print to the bitfighter log file as well as
 * stdout.
 * 
 * @param val The value you wish to print.
 */
S32 lua_logprint(lua_State *L)
{
   string str = buildPrintString(L);

   logprintf(LogConsumer::LuaBotMessage, "%s", str.c_str());

   return 0;
}


/**
 * @luafunc static void global::print(any val)
 *
 * @brief Print to the in-game console.
 *
 * @descr This hijacks Lua's normal 'print' command that goes to stdout and
 * instead redirects it to the in-game console.
 *
 * @param val The value you wish to print.
 */
S32 lua_print(lua_State *L)
{
   string str = buildPrintString(L);
   logprintf(LogConsumer::ConsoleMsg, "%s", str.c_str());

   return 0;
}


/**
 * @luafunc static num global::getRandomNumber(int m, int n)
 *
 * @brief Better random number generated than that included with Lua.
 *
 * @descr General structure and peculiar error messages taken from lua math lib.
 * Docs for that are as follows; we should adhere to them as well:
 *
 * This function is an interface to the simple pseudo-random generator function
 * rand provided by ANSI C. (No guarantees can be given for its statistical
 * properties.) When called without arguments, returns a uniform pseudo-random
 * real number in the range [0,1). When called with an integer number m,
 * math.random returns a uniform pseudo-random integer in the range [1, m]. When
 * called with two integer numbers m and n, math.random returns a uniform
 * pseudo-random integer in the range [m, n].
 *
 * @param m (optional) Used for either [1, m] or [m, n] as described above.
 * @param n (optional) Used for [m, n] as described above.
 *
 * @return A random number as described above
 */
S32 lua_getRandomNumber(lua_State *L)
{
   S32 args = lua_gettop(L);

   if(args == 0)
      return returnFloat(L, TNL::Random::readF());

   if(args == 1)
   {
      S32 max = luaL_checkint(L, 1);
      luaL_argcheck(L, 1 <= max, 1, "interval is empty");
      return returnInt(L, TNL::Random::readI(1, max));
   }

   if(args == 2)
   {
      int min = luaL_checkint(L, 1);
      int max = luaL_checkint(L, 2);
      luaL_argcheck(L, min <= max, 2, "interval is empty");
      return returnInt(L, TNL::Random::readI(min, max));
   }

   else
      return luaL_error(L, "wrong number of arguments");
}


/**
 * @luafunc static int global::getMachineTime()
 *
 * @brief Get the time according to the system clock.
 *
 * @return Machine time (i.e. wall clock time) in milliseconds.
 */
S32 lua_getMachineTime(lua_State *L)
{
   return returnInt(L, Platform::getRealMilliseconds());
}


/**
 * @luafunc static string global::findFile(string filename)
 *
 * @brief Finds a specific file to load from various Lua folders.
 *
 * @descr Scans our scripts, levels, and robots directories for a file, in
 * preparation for loading.
 *
 * @param filename The file you're looking for.
 *
 * @return The path to the file in question, or nil if it can not be found.
 */
S32 lua_findFile(lua_State *L)
{
   checkArgList(L, "global", "findFile");

   string filename = getString(L, 1, "");

   FolderManager *folderManager = gSettings.getFolderManager();

   string fullname = folderManager->findScriptFile(filename);     // Looks in luadir, levelgens dir, bots dir

   lua_pop(L, 1);    // Remove passed arg from stack
   TNLAssert(lua_gettop(L) == 0 || dumpStack(L), "Stack not cleared!");

   if(fullname == "")
   {
      logprintf(LogConsumer::LogError, "Could not find script file \"%s\"...", filename.c_str());
      return returnNil(L);
   }

   return returnString(L, fullname.c_str());
}


/**
 * @luafunc static string global::readFromFile(string filename)
 *
 * @brief Reads in a file from our sandboxed IO directory.
 *
 * @descr Reads an entire file from the filesystem into a string.
 *
 * @param filename The filename to read.
 *
 * @return The contents of the file as a string.
 *
 * @note This function will only look for files in the `screenshot` directory of
 * the Bitfighter resource folder.
 */
S32 lua_readFromFile(lua_State *L)
{
   checkArgList(L, "global", "readFromFile");

   string filename = extractFilename(getString(L, 1, ""));

   if(filename == "")
      returnNil(L);

   FolderManager *folderManager = gSettings.getFolderManager();

   string contents;
   readFile(folderManager->getScreenshotDir() + getFileSeparator() + filename, contents);
   return returnString(L, contents.c_str());
}


/**
 * @luafunc static void global::writeToFile(string filename, string contents, bool append)
 * 
 * @brief Write or append to a file on the filesystem.
 * 
 * @descr This is in a sandboxed environment and will only allow writing to a
 * specific directory.
 * 
 * @param filename The filename to write.
 * @param contents The contents to save to the file.
 * @param append (optional) If `true`, append to the file instead of creating
 * a new one.
 *
 * @note This function will only write to files in the `screenshot` directory of
 * the Bitfighter resource folder.
 */
S32 lua_writeToFile(lua_State *L)
{
   S32 profile = checkArgList(L, "global", "writeToFile");

   // Sanitize the path.  Only a file name is allowed!
   string filename = extractFilename(getString(L, 1, ""));
   string contents = getString(L, 2, "");

   bool append = false;
   if(profile == 1)
      append = getBool(L, 3);

   if(filename != "" && contents != "")
   {
      FolderManager *folderManager = gSettings.getFolderManager();

      string filePath = folderManager->getScreenshotDir() + getFileSeparator() + filename;

      writeFile(filePath, contents, append);
   }

   return 0;
}


/**
 * @luafunc static string global::getVersion()
 *
 * @brief Get the current Bitfighter version as a string.
 *
 * @return Bitfighter Version.
 */
S32 lua_getVersion(lua_State *L)
{
   return returnString(L, ZAP_GAME_RELEASE);
}


/**
 * These static methods are independent of gameplay and do not perform any actions on game objects
 */
//               Fn name    Param profiles         Profile count
#define LUA_STATIC_METHODS(METHOD) \
      METHOD(logprint,        ARRAYDEF({{ ANY,   END }}), 1 ) \
      METHOD(print,           ARRAYDEF({{ ANY,   END }}), 1 ) \
      METHOD(getRandomNumber, ARRAYDEF({{        END }}), 1 ) \
      METHOD(getMachineTime,  ARRAYDEF({{        END }}), 1 ) \
      METHOD(findFile,        ARRAYDEF({{ STR,   END }}), 1 ) \
      METHOD(writeToFile,     ARRAYDEF({{ STR, STR, END }, { STR, STR, BOOL, END }}), 2 ) \
      METHOD(readFromFile,    ARRAYDEF({{ STR,   END }}), 1 ) \
      METHOD(getVersion,      ARRAYDEF({{        END }}), 1 ) \

GENERATE_LUA_STATIC_METHODS_TABLE(global, LUA_STATIC_METHODS);

#undef LUA_STATIC_METHODS


};
