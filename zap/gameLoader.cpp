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

#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++

#include "gameLoader.h"
#include "teleporter.h"

#include "config.h"     // For CmdLineSettings

#include "../tnl/tnl.h"
#include "../tnl/tnlLog.h"

#include <stdio.h>

#ifdef __APPLE__
#include "Directory.h" // For GetLevels
#endif

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

static char *argv[MaxArgc];
static char argv_buffer[MaxArgc][MaxArgLen];
static int argc;
static int argLen = 0;
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

inline void addArg()
{
   if(argc < MaxArgc)
   {
      argv[argc][argLen] = 0;
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
   argString = string;
   char c;

   for(U32 i = 0; i < MaxArgc; i++)
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
   goto stateAddCharToIdent;
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
   /*    Handle newlines and tabs embedded in string itself...  probably better not to.
   if(c == '\\')
   {
      c = getNextChar();
      if(c == 'n')
      {
         addCharToArg('\n');
         goto stateReadString;
      }
      if(c == 't')
      {
         addCharToArg('\');
         goto stateReadString;
      }
      if(c == '\\')
      {
         addCharToArg('\\');
         goto stateReadString;
      }
      if(c == '\n' || !c)
      {
         addArg();
         goto stateLineParseDone;
      }
   }
   */
   addCharToArg(c);
   goto stateReadString;
stateEatingComment:
   c = getNextChar();
   if(c != '\n' && c)
      goto stateEatingComment;
stateLineParseDone:
   if(argc)
      processLevelLoadLine(argc, (const char **) argv);
   argc = 0;
   argLen = 0;
   if(c)
      goto stateEatingWhitespace;
   return 0;     
}  // parseArgs



bool LevelLoader::initLevelFromFile(const char *file)
{
   FILE *f = fopen(file, "r");
   if(!f)
      return false;

   char fileData[98304];      // Max level size = 96k -- that's really big!

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   fclose(f);

   return true;
}


#undef MaxArgc
#undef MaxArgLen

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TNL_OS_MAC_OSX

#  ifdef TNL_OS_WIN32
#     include "dirent.h"        // Need local copy for Windows builds
#  else
#     include <dirent.h>        // Need standard copy for Linux
#  endif

using namespace std;

// Read files from folder
// Based on http://www.linuxquestions.org/questions/programming-9/c-list-files-in-directory-379323/
// Is this platform independent?  Works on Windows and now on Linux, but apparently not on a Mac
// OSX version in Directory.mm
bool getLevels(string dir, Vector<string> &files)
{
   DIR *dp;
   struct dirent *dirp;

   //string dir = "levels";
   //if (subdir != "")
   //   dir += "\\" + subdir;         // OK, this isn't really cross platform, but this will likely need to be modded
   //                                 // for Linux anyway, and the Mac stuff is handled elsewhere...
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
#endif      // #ifndef TNL_OS_MAC_OSX


// Sorts alphanumerically
S32 QSORT_CALLBACK alphaSort(string *a, string *b)
{
   return stricmp((a)->c_str(), (b)->c_str());        // Is there something analagous to stricmp for strings (as opposed to c_strs)?
}


extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;
extern Vector<StringTableEntry> gLevelList;


// Create the definititve list of levels for hosting a game
void LevelListLoader::buildLevelList()
{
   // If user specified a list of levels on the command line, use those
   if(gCmdLineSettings.specifiedLevels.size() > 0)
   {
      gLevelList = gCmdLineSettings.specifiedLevels;
      return;
   }

   // If neither -leveldir nor -alllevels were not specified on the cmd line, and the INI file has some specified, use those
   if(gCmdLineSettings.levelDir == "" && !gCmdLineSettings.alllevels && gIniSettings.levelList.size() > 0)
   {
      gLevelList = gIniSettings.levelList;
      return;
   }

   // Otherwise we need to build our level list by looking at the filesystem  (n.b. gLevelDir defaults to the "levels" folder under the bitfighter install dir)
   gLevelList.clear();
   Vector<string> levelfiles;

   if(!getLevels(gLevelDir, levelfiles))    // True if error reading level...  print message... or just PANIC!!
   {
      logprintf("Could not read any levels from the levels folder \"%s\".", gLevelDir.c_str());
   }

   levelfiles.sort(alphaSort);   // Just to be sure...

   for (S32 i = 0; i < levelfiles.size(); i++)
      gLevelList.push_back(StringTableEntry(levelfiles[i].c_str()));
}

};

