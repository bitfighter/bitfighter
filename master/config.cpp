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


#include "../zap/SharedConstants.h"
#include "../zap/stringUtils.h"

#include "tnl.h"
#include "tnlLog.h"
#include <stdio.h>
#include <string>

using namespace TNL;
using namespace std;

extern string lcase(string strToConvert);

extern U32 gMasterPort;
extern string gMasterName;
extern string gJasonOutFile;

// Variables for managing access to MySQL
extern string gMySqlAddress;
extern string gDbUsername;
extern string gDbPassword;

// Variables for verifying usernames/passwords in PHPBB3
extern string gPhpbb3Database;
extern string gPhpbb3TablePrefix;

extern bool gWriteStatsToDatabase;
extern string gStatsDatabaseAddress;
extern string gStatsDatabaseName;
extern string gStatsDatabaseUsername;
extern string gStatsDatabasePassword;


extern U32 gLatestReleasedCSProtocol;

extern Vector<string> MOTDTypeVecOld;
extern Vector<string> MOTDStringVecOld;

extern Vector<U32> MOTDVersionVec;
extern Vector<string> MOTDStringVec;

void processConfigLine(int argc, string argv[])
{
   if(argv[0] == "port" && argc > 1)             // port --> set port
      gMasterPort = atoi(argv[1].c_str());

   // The following chunk can go away when we retire CMProtocol version 0
   else if(argv[0] == "motd" && argc > 2)   // motd --> set motd for version 0 clients
   {
      MOTDTypeVecOld.push_back(argv[1]);    // version
      MOTDStringVecOld.push_back(argv[2]);  // message
   }

   // CMProtocol version 1 entries look a bit different, but serves the same basic function...
   else if(argv[0] == "setmotd" && argc > 2)   // setmotd --> set motd for version 1+ clients
   {
      U32 version = atoi(argv[1].c_str());     // Build version this message corresponds to, so we can set different messages for different clients
      MOTDVersionVec.push_back(version);
      MOTDStringVec.push_back(argv[2]);
   }

   // New usemotd directive tells server to use the message stored in the file motd
   else if(argv[0] == "usemotd" && argc > 2)   // usemotd --> read motd file from local folder
   {
      U32 version = atoi(argv[1].c_str());     // Game build this message corresponds to, allows us to set different messages for different clients
      char *file = strdup(argv[2].c_str());    // Message stored in this file

      FILE *f = fopen(file, "r");
      if(!f)
      {
         logprintf(LogConsumer::LogError, "Unable to open MOTD file \"%s\" -- using default MOTD.", file);

         // Save a default message
         MOTDStringVec.push_back("Welcome to Bitfighter!");
         MOTDVersionVec.push_back(version);

         return;
      }

      char message[MOTD_LEN];
      if(fgets(message, MOTD_LEN, f))
         MOTDStringVec.push_back(message);
      else
         MOTDStringVec.push_back("Welcome to Bitfighter!");

      MOTDVersionVec.push_back(version);
      
      fclose(f);
   }

   else if(argv[0] == "name" && argc > 1)        // name --> set server's name
      gMasterName = argv[1];

   else if(argv[0] == "protocol" && argc > 1)    // protocol --> latest and greatest version of c-s protocol
      gLatestReleasedCSProtocol = atoi(argv[1].c_str());

   else if(argv[0] == "json_file" && argc > 1)   // json file
      gJasonOutFile = argv[1];

   else if(argv[0] == "my_sql_address" && argc > 1)      // DB address
      gMySqlAddress = argv[1];      

   else if(argv[0] == "db_username" && argc > 1)         // Username needed to access db
      gDbUsername = argv[1];    

   else if(argv[0] == "db_password" && argc > 1)         // Password needed to access db
      gDbPassword = argv[1];   

   else if(argv[0] == "phpbb3_database" && argc > 1)     // Name of phpbb3 database
      gPhpbb3Database = argv[1]; 

   else if(argv[0] == "phpbb3_table_prefix" && argc > 1) // Prefix used for phpbb3 tables
      gPhpbb3TablePrefix = argv[1];  

   else if(argv[0] == "stats_database_addr" && argc > 1)
      gStatsDatabaseAddress = argv[1];

   else if(argv[0] == "stats_database_name" && argc > 1)
      gStatsDatabaseName = argv[1];

   else if(argv[0] == "stats_database_username" && argc > 1)
      gStatsDatabaseUsername = argv[1];

   else if(argv[0] == "stats_database_password" && argc > 1)
      gStatsDatabasePassword = argv[1];

   else if(argv[0] == "write_stats_to_database" && argc > 1)
      gWriteStatsToDatabase = (lcase(argv[1]) == "Yes");
}

enum {
   MaxArgc = 128,
   MaxArgLen = 100,
};

static string argv[MaxArgc];   // *argv[MaxArgc]
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
      argv[argc] += c;
      argLen++;
   }
}

inline void addArg()
{
   if(argc < MaxArgc)
   {
      //argv[argc][argLen] = 0;
      argc++;
      argLen = 0;
   }
}

inline void clearArgv()
{
   for(S32 i = 0; i < MaxArgc; i++)
      argv[i] = "";
}

int parseArgs(const char *string)
{
   int numObjects = 0;

   argc = 0;
   argLen = 0;
   clearArgv();

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
      processConfigLine(argc, argv);
   
   // Reset everything...
   argc = 0;
   argLen = 0;
   clearArgv();

   if(c)
      goto stateEatingWhitespace;

   return numObjects;
}

void readConfigFile()
{
   const char *configFile = "master.cfg";
   FILE *f = fopen(configFile, "r");
   if(!f)
   {
      logprintf(LogConsumer::LogError, "Unable to open config file \"%s\"", configFile);
      return;
   }

   MOTDStringVecOld.clear();
   MOTDTypeVecOld.clear();
   MOTDStringVec.clear();
   MOTDVersionVec.clear();


   char fileData[32768];

   size_t bytesRead = fread(fileData, 1, sizeof(fileData), f);
   fileData[bytesRead] = 0;

   parseArgs(fileData);

   if(gLatestReleasedCSProtocol == 0)
      logprintf(LogConsumer::LogError, "Unable to find a valid protocol line in config file... disabling update checks!");

   fclose(f);
}

