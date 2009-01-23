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

#include "../tnl/tnl.h"
#include "../tnl/tnlLog.h"
#include <stdio.h>

using namespace TNL;

extern U32 gMasterPort;
extern const char *gMasterName;
extern const char *gJasonOutFile;

extern U32 gLatestReleasedCSProtocol;

extern Vector<char *> MOTDTypeVecOld;
extern Vector<char *> MOTDStringVecOld;

extern Vector<U32> MOTDVersionVec;
extern Vector<char *> MOTDStringVec;


void processConfigLine(int argc, const char **argv)
{

   if(!stricmp(argv[0], "port") && argc > 1)             // port --> set port
      gMasterPort = atoi(argv[1]);

   // The following chunk can go away when we retire CMProtocol version 0
   else if(!stricmp(argv[0], "motd") && argc > 2)        // motd --> set motd for version 0 clients
   {
      char *type = strdup(argv[1]);       // version
      char *message = strdup(argv[2]);    // message

      MOTDTypeVecOld.push_back(type);
      MOTDStringVecOld.push_back(message);
   }

   // CMProtocol version 1 entries look a bit different, but serves the same basic function...
   else if(!stricmp(argv[0], "setmotd") && argc > 2)        // setmotd --> set motd for version 1+ clients
   {
      U32 version = atoi(argv[1]);        // Build version this message corresponds to, allows us to set different messages for different clients
      char *message = strdup(argv[2]);    // Message

      MOTDVersionVec.push_back(version);
      MOTDStringVec.push_back(message);
   }

   else if(!stricmp(argv[0], "name") && argc > 1)        // name --> set server's name
      gMasterName = strdup(argv[1]);

   else if(!stricmp(argv[0], "protocol") && argc > 1)    // protocol --> latest and greatest version of c-s protocol
      gLatestReleasedCSProtocol = atoi(argv[1]);

   else if(!stricmp(argv[0], "json_file") && argc > 1)   // json file
      gJasonOutFile = strdup(argv[1]);
}

enum {
   MaxArgc = 128,
   MaxArgLen = 100,
};

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

int parseArgs(const char *string)
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
         addCharToArg('\t');
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
   addCharToArg(c);
   goto stateReadString;
stateEatingComment:
   c = getNextChar();
   if(c != '\n' && c)
      goto stateEatingComment;
stateLineParseDone:
   if(argc)
      processConfigLine(argc, (const char **) argv);
   argc = 0;
   argLen = 0;
   if(c)
      goto stateEatingWhitespace;
   return numObjects;
}

void readConfigFile()
{
   FILE *f = fopen("master.cfg", "r");
   if(!f)
   {
      logprintf("Unable to open config file.");
      return;
   }

   // Clean out our string structs
   for(S32 i = 0; i < MOTDTypeVecOld.size(); i++)
   {
      free(MOTDTypeVecOld[i]);
      free(MOTDStringVecOld[i]);
   }
   MOTDTypeVecOld.clear();
   MOTDStringVecOld.clear();


   // And again, for our newer structs
   for(S32 i = 0; i < MOTDStringVec.size(); i++)
      free(MOTDStringVec[i]);

   MOTDStringVec.clear();
   MOTDVersionVec.clear();


   char fileData[32768];

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   if(gLatestReleasedCSProtocol == 0)
      logprintf("Unable to find a valid protocol line in cfg file... disabling update checks!");

   fclose(f);
}

