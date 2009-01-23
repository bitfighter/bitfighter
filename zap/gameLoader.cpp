//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "../tnl/tnl.h"
#include "../tnl/tnlLog.h"

#include <stdio.h>

using namespace TNL;
namespace Zap
{

const S32 MaxArgc = 128;    // At most MaxArgc args on a single line,
const S32 MaxArgLen = 100;  // each at most MaxArgLen bytes long  (enforced in addCharToArg)

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
// This is rather ugly!

// Each line of the file is parsed separately by processLevelLoadLine in game.cpp or UIEditor.cpp

int LevelLoader::parseArgs(const char *string)
{
   int numObjects = 0;

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
   return numObjects;               // Never incremented, always returns 0
}  // parseArgs



bool LevelLoader::initLevelFromFile(const char *file)
{
   FILE *f = fopen(file, "r");
   if(!f)
   {
      logprintf("Unable to open level file %s!!", file);
      return false;
   }

   char fileData[65536];      // Max level size = 64k -- that's pretty big!

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   fclose(f);

   return true;
}



};
