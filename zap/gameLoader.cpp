//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "gameLoader.h"
#include "stringUtils.h"

#include "tnl.h"
#include "tnlLog.h"

#include <fstream>
#include <sstream>

#include <stdio.h>

#  ifdef TNL_OS_WIN32
#     include "../other/dirent.h"        // Need local copy for Windows builds
#  else
#     include <dirent.h>        // Need standard copy for *NIXes
#  endif


using namespace TNL;


namespace Zap
{

// TODO: These are no longer needed, but some other parts of the code still use them
const S32 LevelLoader::MaxArgLen = 100;               // Each at most MaxArgLen bytes long  (enforced in addCharToArg)
const S32 LevelLoader::MaxIdLen = S32_MAX_DIGITS + 1; // Max 32-bit int is 10 digits, plus room for a null
const S32 LevelLoader::MaxLevelLineLength = 4096;     // Max total level line length we'll tolerate


// Constructor
LevelLoader::LevelLoader()
{
   // Do nothing
}

// Destructor
LevelLoader::~LevelLoader()
{
   // Do nothing
}


// Parse the contents of the level file string is the file data itself
// This is rather ugly!  Totally old school!

// Each line of the file is handled separately by processLevelLoadLine in game.cpp or UIEditor.cpp

void LevelLoader::parseLevelLine(const char *line, GridDatabase *database, const string &levelFileName)
{
   Vector<string> args = parseString(string(line));
   U32 argc = args.size();
   S32 id = 0;
   const char** argv = new const char* [argc];

   for(U32 i = 0; i < argc; i++)
   {
      argv[i] = args[i].c_str();
   }

   if(argc >= 1)
   {
      size_t pos = args[0].find("!");
      if(pos != string::npos)
      {
         id = atoi(args[0].substr(pos, args[0].size() - pos).c_str());
      }
   }

   try
   {
      processLevelLoadLine(argc, id, (const char **) argv, database, levelFileName);
   }
   catch(LevelLoadException &e)
   {
      logprintf("Level Error: Can't parse %s: %s", line, e.what());  // TODO: fix "line" variable having hundreds of level lines
   }

   delete[] argv;
}



void LevelLoader::loadLevelFromString(const string &contents, GridDatabase* database, const string &filename)
{
   istringstream iss(contents);
   string line;
   while(std::getline(iss, line))
   {
      parseLevelLine(line.c_str(), database, filename);
   }
}

// Reads files by chunks, converts to lines
bool LevelLoader::loadLevelFromFile(const string &filename, GridDatabase *database)
{
   string contents = readFile(filename);
   if(contents == "")
   {
      return false;
   }

   loadLevelFromString(contents, database, filename);

#ifdef SAM_ONLY
   // In case the level crash the game trying to load, want to know which file is the problem. 
   logprintf("Loading %s", filename.c_str());
#endif

   return true;
}


#undef MaxArgc
#undef MaxArgLen

////////////////////////////////////////
////////////////////////////////////////


using namespace std;


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelLoadException::LevelLoadException(string str) : msg(str)
{
   // Do nothing
}

// Destructor, required by gcc to avoid "looser throw" error
LevelLoadException::~LevelLoadException() throw ()
{
   // Do nothing
}


const char *LevelLoadException::what() const throw ()
{
   return msg.c_str();
}


};

