//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "GameJoltConnector.h"

#include "database.h"
#include "master.h"
#include "MasterServerConnection.h"

#include "../zap/HttpRequest.h"
#include "../zap/md5wrapper.h"
#include "../zap/stringUtils.h"

using namespace Zap;
using namespace DbWriter;
using namespace std;


// No GameJolt for Windows, or when phpbb is disabled -- can also disable GameJolt in the INI file
#if defined VERIFY_PHPBB3 && !defined TNL_OS_WIN32    
#  define GAME_JOLT
#endif

// Uncomment to test compiling on Windows
//#define GAME_JOLT
//#define fork() false;
//#define execl() 
//#ifndef TNL_OS_WIN32
//#  error -- need to comment this block out to build!
//#endif


namespace GameJolt
{

// Bitfighter's GameJolt id ==> does it make sense to put this in the INI?
static const string gameIdString = "game_id=20546";      

static md5wrapper md5;


// When using curl, this will never return
static void updateGameJolt(const MasterSettings *settings, const string &baseUrl, 
                           const string &secret,           const string &quotedNameList,
                           const string &otherParams = "")
{
#ifdef GAME_JOLT

   DatabaseWriter databaseWriter = DbWriter::getDatabaseWriter(settings);

   string databaseName = settings->getVal<string>("Phpbb3Database");
   Vector<string> credentialStrings = databaseWriter.getGameJoltCredentialStrings(databaseName, quotedNameList, 1);

   //HttpRequest request;

   string urlList = "";
   string otherParamString = otherParams + (otherParams != "" ? "&" : "");

   for(S32 i = 0; i < credentialStrings.size(); i++)
   {
      string url = baseUrl + "?" + gameIdString + "&" + otherParamString + credentialStrings[i];

      string signature = md5.getHashFromString(url + secret);

      url += "&signature=" + signature;

      urlList += url + " ";

      //request.setUrl(url);

      //if(!request.send())
      //   logprintf(LogConsumer::LogError, "Error sending GameJolt request! (msg=%s, url=%s)", 
      //                                     request.getError().c_str(), url.c_str());
   }

   if(urlList.length() > 0)
   {
      // This is a fallback because the request.send() was returning a "Socket not writable" error
      execl("/usr/bin/curl", "curl", urlList.c_str(), NULL);

      logprintf(LogConsumer::LogError, "Error running exec()");
      exit(1);
   }

   exit(0);

#endif
}


static void onPlayerAuthenticatedOrQuit(const MasterSettings *settings, const MasterServerConnection *client, const string &verb)
{
#ifdef GAME_JOLT  

   if(!settings->getVal<YesNo>("UseGameJolt"))
      return;

   string secret = settings->getVal<string>("GameJoltSecret");

   if(secret == "")
   {
      logprintf(LogConsumer::LogWarning, "Missing GameJolt secret key!");
      return;
   }

   S32 pid = fork();

   if(pid < 0)
   {
      logprintf(LogConsumer::LogError, "PANIC: Could not fork process! (%s)", verb.c_str());
      exit(1);    
   }

   if(pid > 0)
      return;        // Parent process, return and get on with life

   // From here on down is child process... we'll never return!
   string nameList = "'" + sanitizeForSql(client->mPlayerOrServerName.getString()) + "'";

   updateGameJolt(settings, "http://gamejolt.com/api/game/v1/sessions/" + verb, secret, nameList);

   exit(0);    // Bye bye!

#endif
}


void onPlayerAuthenticated(const MasterSettings *settings, const MasterServerConnection *client)
{
   onPlayerAuthenticatedOrQuit(settings, client, "open");

   // The above never returns
}


void onPlayerQuit(const MasterSettings *settings, const MasterServerConnection *client)
{
   onPlayerAuthenticatedOrQuit(settings, client, "close");

   // The above never returns
}


// Write a current count of clients/servers for display on a website, using JSON format
// This gets updated whenever we gain or lose a server, at most every 5 seconds (currently)
void ping(const MasterSettings *settings, const Vector<MasterServerConnection *> *clientList)
{
#ifdef GAME_JOLT    

   if(!settings->getVal<YesNo>("UseGameJolt"))
      return;

   string secret = settings->getVal<string>("GameJoltSecret");

   if(secret == "")
   {
      logprintf(LogConsumer::LogWarning, "Missing GameJolt secret key!");
      return;
   }

   S32 pid = fork();

   if(pid < 0)
   {
      logprintf(LogConsumer::LogError, "PANIC: Could not fork process! (ping)");
      exit(1);
   }

   if(pid > 0)
      return;        // Parent process, return and get on with life


   // From here on down is child process... we'll never return!

   
   // Assemble list of all connected and authenticated players

   string nameList = "";  // Comma seperated list of quoted, sanitized names ready to pass to a SQL IN() function
   S32 nameCount = 0;

   for(S32 i = 0; i < clientList->size(); i++)
   {
      if(clientList->get(i) && clientList->get(i)->mAuthenticated) 
      {
         string name = sanitizeForSql(clientList->get(i)->mPlayerOrServerName.getString());
         if(nameCount > 0)
            nameList += ", ";

         nameList += "'" + name + "'";
         nameCount++;
      }
   }

   updateGameJolt(settings, "http://gamejolt.com/api/game/v1/sessions/ping", secret, nameList);

   exit(0);    // Bye bye!

#endif
}


void onPlayerAwardedAchievement(const MasterSettings *settings, const MasterServerConnection *client, S32 achievementId)
{
#ifdef GAME_JOLT  

   if(!settings->getVal<YesNo>("UseGameJolt"))
      return;

   string secret = settings->getVal<string>("GameJoltSecret");

   if(secret == "")
   {
      logprintf(LogConsumer::LogWarning, "Missing GameJolt secret key!");
      return;
   }

   S32 pid = fork();
   wait();

   if(pid < 0)
   {
      logprintf(LogConsumer::LogError, "PANIC: Could not fork process! (achievement)");
      exit(1);    
   }

   if(pid > 0)
      return;        // Parent process, return and get on with life


   // From here on down is child process... we'll never return!
   string name = "'" + sanitizeForSql(client->mPlayerOrServerName.getString()) + "'";

   DatabaseWriter databaseWriter = DbWriter::getDatabaseWriter(settings);
   string trophyId = databaseWriter.getGameJoltTrophyId(achievementId);

   // Make sure we have the trophy in Game Jolt
   if(trophyId == "")
      exit(0);

   string trophyStr = "trophy_id=" + trophyId;

   updateGameJolt(settings, "http://gamejolt.com/api/game/v1/trophies/add-achieved", secret, name, trophyStr);

   exit(0);    // Bye bye!

#endif
}


} 
