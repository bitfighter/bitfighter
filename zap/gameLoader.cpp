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

#include "tnl.h"
#include "tnlLog.h"

#include <stdio.h>


using namespace TNL;


namespace Zap
{

// GCC wants storage for these, for some reason.  Unfortunately VC++ doesn't want that.
#ifndef WIN32
const int LevelLoader::MaxArgc;
const int LevelLoader::MaxArgLen;
#endif

// For readability and laziness...
#define MaxArgc LevelLoader::MaxArgc
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
      if(argc < MaxArgc && argLen < MaxArgLen-1)
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

// Each line of the file is parsed separately by processLevelLoadLine in game.cpp or UIEditor.cpp

int LevelLoader::parseArgs(const char *string)
{
   argc = 0;
   argLen = 0;
   idLen = 0;

   argString = string;
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
      processLevelLoadLine(argc, (U32)atoi(id), (const char **) argv);
   }
   argc = 0;
   argLen = 0;
   idLen = 0;

   if(c)
      goto stateEatingWhitespace;
   return 0;
}  // parseArgs


// Reads files by chunks, converts to lines
bool LevelLoader::initLevelFromFile(const char *filename)
{
   char levelChunk[4096];     // Data buffer for reading in chunks of our level file
   const S32 minCur = 50;     // Provide a little breathing room for this file reading algorithm to work; arbitrary value, works well
   FILE *file = fopen(filename, "r");

#ifdef SAM_ONLY
   // Wanting to know which map the game crashes on.
   logprintf("Loading %s", filename);
#endif

   if(!file)               // Can't open file
      return false;

   S32 cur = 0;            // Cursor, or pointer to character in our chunk
   S32 lastByteRead = -1;  // Position of last byte read in chunk; if read an entire chunk, will be c. 4096

   while(lastByteRead != 0)
   {
      // Read a chunk of the level file, filling any space in levelChunk buffer after cur
      lastByteRead = cur + fread(&levelChunk[cur], 1, sizeof(levelChunk) - 1 - cur, file);

      // Advance cursor to end of chunk
      cur = lastByteRead - 1;

      if(cur == sizeof(levelChunk) - 2)       // Haven't finished the file yet
      {
         while(levelChunk[cur] != '\n' && cur > minCur)  // cur > minCur indicates the line is too long
            cur--;      // Back cursor up, looking for final \n in our chunk

         if(cur == minCur) 
            logprintf(LogConsumer::LogWarning, "Load level ==> Some lines too long in file %s (max len = %d)", filename, sizeof(levelChunk) - minCur);
      }
      cur++;   // Advance cursor past that last \n, now points to first char of second line in chunk

      TNLAssert(cur >= 0 && cur < sizeof(levelChunk), "Load level from file, Cur out of range");

      char c = levelChunk[cur];     // Read a char, hold onto it for a second
      levelChunk[cur] = 0;          // Replace it with null

      parseArgs(levelChunk);        // parseArgs will read from the beginning of the chunk until it hits the null we just inserted
      levelChunk[cur] = c;          // Replace the null with our saved char

      // Now get rid of that line we just processed with parseArgs, by copying the data starting at cur back to the beginning of our chunk
      S32 cur2 = 0;
      while(cur + cur2 != lastByteRead)      // Don't go beyond the data we've read
      {
         levelChunk[cur2] = levelChunk[cur + cur2];      // Copy
         cur2++;                                         // Advance
      }
      cur = cur2;                            // Move cur back to cur2
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

// Read files from folder
// Based on http://www.linuxquestions.org/questions/programming-9/c-list-files-in-directory-379323/
// Note: used to include special getLevels() in Directory.mm for Mac only, not needed anymore.
bool getLevels(string dir, Vector<string> &files)
{
   DIR *dp;
   struct dirent *dirp;

   if((dp = opendir(dir.c_str())) == NULL)
      return false;

   while ((dirp = readdir(dp)) != NULL)
   {
      string name = string(dirp->d_name);
      if(name.length() > 6 && name.substr(name.length() - 6, 6) == ".level")
         files.push_back(name);
   }

   closedir(dp);
   return true;
}


// Sorts alphanumerically
S32 QSORT_CALLBACK alphaSort(string *a, string *b)
{
   return stricmp((a)->c_str(), (b)->c_str());        // Is there something analagous to stricmp for strings (as opposed to c_strs)?
}


extern ConfigDirectories gConfigDirs;
extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern Vector<StringTableEntry> gLevelSkipList;

// Create a list of levels for hosting a game, but does not read the files or do any validation of them
Vector<string> LevelListLoader::buildLevelList()
{
   Vector<string> levelList;

   // If user specified a list of levels on the command line, use those
   if(gCmdLineSettings.specifiedLevels.size() > 0)
      levelList = gCmdLineSettings.specifiedLevels;
   else
   {
      // Otherwise we need to build our level list by looking at the filesystem  
      // (n.b. gLevelDir defaults to the "levels" folder under the Bitfighter data install dir)
      Vector<string> levelfiles;

      if(!getLevels(gConfigDirs.levelDir, levelfiles))    // True if error reading level...  print message... or just PANIC!!
      {
         logprintf(LogConsumer::LogError, "Could not read any levels from the levels folder \"%s\".", gConfigDirs.levelDir.c_str());
         return levelList;    // empty
      }

      levelfiles.sort(alphaSort);   // Just to be sure...

      for (S32 i = 0; i < levelfiles.size(); i++)
         levelList.push_back(levelfiles[i]);
   }

   removeSkippedLevels(levelList);
   return levelList;
}


extern string lcase(string strToConvert);

// Remove any levels listed in gLevelSkipList from gLevelList.  Not foolproof!
void LevelListLoader::removeSkippedLevels(Vector<std::string> &levelList)
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

