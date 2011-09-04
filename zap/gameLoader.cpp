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
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "gameLoader.h"
#include "teleporter.h"

#include "config.h"     // For CmdLineSettings
#include "stringUtils.h"

#include "tnl.h"
#include "tnlLog.h"

#include <stdio.h>


using namespace TNL;


namespace Zap
{

// GCC wants storage for these, for some reason.  Unfortunately VC++ doesn't want that.
#ifndef WIN32
const int LevelLoader::MAX_LEVEL_LINE_ARGS;
const int LevelLoader::MaxArgLen;
#endif

// For readability and laziness...
#define MaxArgc LevelLoader::MAX_LEVEL_LINE_ARGS
#define MaxArgLen LevelLoader::MaxArgLen
#define MaxIdLen LevelLoader::MaxIdLen

static char *argv[MaxArgc];
static char id[MaxIdLen];

static char argv_buffer[MaxArgc][MaxArgLen];
static int argc;
static int argLen = 0;
static int idLen = 0;
static const char *argString;


inline char getNextChar()
{
   while(*argString == '\r')
      argString++;
   return *argString++;
}


inline void addCharToArg(char c)
{
   if(c >= ' ' && c <= '~')         // Limit ourselves to printable chars
      if(argc < MaxArgc && argLen < MaxArgLen - 1)
      {
         argv[argc][argLen] = c;
         argLen++;
      }
}


inline void addCharToID(char c)
{
   if(c >= '0' && c <= '9')         // Limit ourselves to numerics
      if(idLen < MaxIdLen - 1)
      {
         id[idLen] = c;
         idLen++;
      }
}


inline void addArg()
{
   if(argc < MaxArgc)
   {
      argv[argc][argLen] = 0;       // Null terminate the string
      argc++;
      argLen = 0;
   }
}



// Parse the contents of the level file string is the file data itself
// This is rather ugly!  Totally old school!

// Each line of the file is handled separately by processLevelLoadLine in game.cpp or UIEditor.cpp

void LevelLoader::parseLevelLine(const char *line, GridDatabase *database, bool inEditor, const string &levelFileName)
{
   argc = 0;
   argLen = 0;
   idLen = 0;

   argString = line;

   char c;

   for(S32 i = 0; i < MaxArgc; i++)
      argv[i] = argv_buffer[i];

stateEatingWhitespace:
   c = getNextChar();
   if(c == ' ' || c == '\t')
      goto stateEatingWhitespace;
   if(c == '\n' || !c)
      goto stateLineParseDone;
   if(c == '\"')
      goto stateReadString;
   if(c == '#')
      goto stateEatingComment;


stateAddCharToIdent:
   addCharToArg(c);
   c = getNextChar();
   if(c == ' ' || c == '\t')
   {
      addArg();
      goto stateEatingWhitespace;
   }
   if(c == '\n' || !c)
   {
      addArg();
      goto stateLineParseDone;
   }
   if(c == '\"')
   {
      addArg();
      goto stateReadString;
   }
   if(c == '!' && argc == 0)     // ID's can only appear in first arg
   {
      goto stateAddCharToID;
   }
   goto stateAddCharToIdent;


stateAddCharToID:
   c = getNextChar();           // First time here we know c == '!', so let's just move on to the next one

   if(c == ' ' || c == '\t')
   {
      addArg();
      goto stateEatingWhitespace;
   }
   if(c == '\n' || !c)
   {
      addArg();
      goto stateLineParseDone;
   }

   addCharToID(c);
   goto stateAddCharToID;


stateReadString:
   c = getNextChar();
   if(c == '\"')
   {
      addArg();
      goto stateEatingWhitespace;
   }
   if(c == '\n' || !c)
   {
      addArg();
      goto stateLineParseDone;
   }

   addCharToArg(c);
   goto stateReadString;


stateEatingComment:
   c = getNextChar();
   if(c != '\n' && c)
      goto stateEatingComment;


stateLineParseDone:
   if(argc)
   {
      id[idLen] = 0;
      try
      {
         processLevelLoadLine(argc, (U32)atoi(id), (const char **) argv, database, inEditor, levelFileName);
      }
      catch(LevelLoadException &e)
      {
         logprintf("Level Error: Can't parse %s: %s", line, e.what());
      }
   }
   argc = 0;
   argLen = 0;
   idLen = 0;

   if(c)
      goto stateEatingWhitespace;
}  // parseLevelLine


// Reads files by chunks, converts to lines
bool LevelLoader::loadLevelFromFile(const char *filename, bool inEditor, GridDatabase *database)
{
   char levelChunk[MAX_LEVEL_LINE_LENGTH];     // Data buffer for reading in chunks of our level file
   FILE *file = fopen(filename, "r");

#ifdef SAM_ONLY
   // In case the level crash the game trying to load, want to know which file is the problem. 
   logprintf("Loading %s", filename);
#endif

   if(!file)               // Can't open file
      return false;

   S32 cur = 0;            // Cursor, or pointer to character in our chunk
   S32 lastByteRead = -1;  // Position of last byte read in chunk; if read an entire chunk, will be c. 4096

   while(lastByteRead != 0)
   {
      // Read a chunk of the level file, filling any space in levelChunk buffer after cur
      lastByteRead = (S32)fread(&levelChunk[cur], 1, sizeof(levelChunk) - 1 - cur, file);   // using -1 to make room for a NULL character
      TNLAssert(lastByteRead >= 0, "lastByteRead is negative while reading level");

      if(lastByteRead < 0) 
         lastByteRead = 0;  // Makes sure there are no errors if somehow this is negative

      lastByteRead += cur;

      // Advance cursor to end of chunk
      cur = lastByteRead - 1;

      if(cur == sizeof(levelChunk) - 2)       // Haven't finished the file yet
      {
         while(levelChunk[cur] != '\n' && cur != 0)  // cur == 0 indicates the line is too long
            cur--;      // Back cursor up, looking for final \n in our chunk

         if(cur == 0)
         {
            logprintf(LogConsumer::LogWarning, "Load level ==> Some lines too long in file %s (max len = %d)", filename, sizeof(levelChunk) - 2);  // -2 : need room for NULL and \n character
				cur = lastByteRead - 1; // Did not find \n, go back to end of chunk. Without this line, it will freeze in endless loop.
         }
                     // small cur number (cur > 0) is OK, as cur will then have to be a big number on next pass.
      }
      cur++;   // Advance cursor past that last \n, now points to first char of second line in chunk

      TNLAssert(cur >= 0 && cur < (S32)sizeof(levelChunk), "LevelLoader::loadLevelFromFile, Cur out of range");

      char c = levelChunk[cur];     // Read a char, hold onto it for a second
      levelChunk[cur] = 0;          // Replace it with null

      parseLevelLine(levelChunk, database, inEditor, filename); // ParseLevelLine will read from the beginning of the chunk until it hits the null we just inserted
      levelChunk[cur] = c;                            // Replace the null with our saved char

      // Now get rid of that line we just processed with parseLevelLine, by copying the data starting at cur back to the beginning of our chunk
      S32 cur2 = 0;
      while(cur + cur2 != lastByteRead)      // Don't go beyond the data we've read
      {
         levelChunk[cur2] = levelChunk[cur + cur2];      // Copy
         cur2++;                                         // Advance
      }
      cur = cur2;                            // Move cur back from cur2
   }

   fclose(file);

   return true;
}


#undef MaxArgc
#undef MaxArgLen

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#  ifdef TNL_OS_WIN32
#     include "dirent.h"        // Need local copy for Windows builds
#  else
#     include <dirent.h>        // Need standard copy for *NIXes
#  endif

using namespace std;

// Sorts alphanumerically
S32 QSORT_CALLBACK alphaSort(string *a, string *b)
{
   return stricmp((a)->c_str(), (b)->c_str());        // Is there something analagous to stricmp for strings (as opposed to c_strs)?
}


extern ConfigDirectories gConfigDirs;
extern CmdLineSettings gCmdLineSettings;
extern Vector<StringTableEntry> gLevelSkipList;

// Create a list of levels for hosting a game, but does not read the files or do any validation of them
Vector<string> LevelListLoader::buildLevelList(const string &levelFolder, bool ignoreCmdLine)
{
   Vector<string> levelList;

   // If user specified a list of levels on the command line, use those, unless ignoreCmdLine was set to true
   if(!ignoreCmdLine && gCmdLineSettings.specifiedLevels.size() > 0)
      levelList = gCmdLineSettings.specifiedLevels;
   else
   {
      // Build our level list by looking at the filesystem  
      Vector<string> levelfiles;

      if(!getFilesFromFolder(levelFolder, levelfiles, "level"))    // True if error reading level...  print message... or just PANIC!!
      {
         logprintf(LogConsumer::LogError, "Could not read any levels from the levels folder \"%s\".", levelFolder.c_str());
         return levelList;    // empty
      }

      levelfiles.sort(alphaSort);   // Just to be sure...

      for(S32 i = 0; i < levelfiles.size(); i++)
         levelList.push_back(levelfiles[i]);
   }

   removeSkippedLevels(levelList);

   return levelList;
}


extern string lcase(string strToConvert);

// Remove any levels listed in gLevelSkipList from gLevelList.  Not foolproof!
void LevelListLoader::removeSkippedLevels(Vector<string> &levelList)
{
   for(S32 i = 0; i < levelList.size(); i++)
   {
      // Make sure we have the right extension
      string filename_i = lcase(levelList[i]);
      if(filename_i.find(".level") == string::npos)
         filename_i += ".level";

      for(S32 j = 0; j < gLevelSkipList.size(); j++)
         if(!strcmp(filename_i.c_str(), gLevelSkipList[j].getString()))
         {
            logprintf(LogConsumer::ServerFilter, "Loader skipping level %s listed in LevelSkipList (see INI file)", levelList[i].c_str());
            levelList.erase(i);
            i--;
            break;
         }
   }
}


};

