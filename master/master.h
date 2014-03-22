//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "masterInterface.h"

#include "MasterServerConnection.h"

#include "../zap/IniFile.h"

#include "tnlNetInterface.h"
#include "tnlVector.h"

#include "../zap/Settings.h"    // For Settings def
#include "../zap/Timer.h"

#include <map>

using namespace TNL;
using namespace Zap;


// Note that on the master, our settings are read-only, so there is no need to specify a comment
//            Data type  Setting name              INI Section    INI Key                                Default value            INI Comment
#define MASTER_SETTINGS_TABLE   \
   SETTINGS_ITEM(string,    ServerName,                 "host",     "name",                                 "Bitfighter Master Server", "" ) \
   SETTINGS_ITEM(string,    JsonOutfile,                "host",     "json_file",                            "server.json",              "" ) \
   SETTINGS_ITEM(U32,       Port,                       "host",     "port",                                 25955,                      "" ) \
   SETTINGS_ITEM(U32,       LatestReleasedCSProtocol,   "host",     "latest_released_cs_protocol",          0,                          "" ) \
   SETTINGS_ITEM(U32,       LatestReleasedBuildVersion, "host",     "latest_released_client_build_version", 0,                          "" ) \
                                                                                                                                             \
   /* Variables for managing access to MySQL */                                                                                              \
   SETTINGS_ITEM(string,    MySqlAddress,               "phpbb",    "phpbb_database_address",               "",                         "" ) \
   SETTINGS_ITEM(string,    DbUsername,                 "phpbb",    "phpbb3_database_username",             "",                         "" ) \
   SETTINGS_ITEM(string,    DbPassword,                 "phpbb",    "phpbb3_database_password",             "",                         "" ) \
                                                                                                                                             \
   /* Variables for verifying usernames/passwords in PHPBB3 */                                                                               \
   SETTINGS_ITEM(string,    Phpbb3Database,             "phpbb",    "phpbb3_database_name",                 "",                         "" ) \
   SETTINGS_ITEM(string,    Phpbb3TablePrefix,          "phpbb",    "phpbb3_table_prefix",                  "",                         "" ) \
                                                                                                                                             \
   /* Stats database credentials */                                                                                                          \
   SETTINGS_ITEM(YesNo,     WriteStatsToMySql,          "stats",    "write_stats_to_mysql",                 No,                         "" ) \
   SETTINGS_ITEM(string,    StatsDatabaseAddress,       "stats",    "stats_database_addr",                  "",                         "" ) \
   SETTINGS_ITEM(string,    StatsDatabaseName,          "stats",    "stats_database_name",                  "",                         "" ) \
   SETTINGS_ITEM(string,    StatsDatabaseUsername,      "stats",    "stats_database_username",              "",                         "" ) \
   SETTINGS_ITEM(string,    StatsDatabasePassword,      "stats",    "stats_database_password",              "",                         "" ) \
                                                                                                                                             \
   /* GameJolt settings */                                                                                                                   \
   SETTINGS_ITEM(YesNo,     UseGameJolt,                "GameJolt", "UseGameJolt",                          Yes,                        "" ) \
   SETTINGS_ITEM(string,    GameJoltSecret,             "GameJolt", "GameJoltSecret",                       "",                         "" ) \


namespace Zap {
   struct VersionedGameStats;       // gameStats.h
   struct GameStats;
}


// No GameJolt for Windows, or when phpbb is disabled -- can also disable GameJolt in the INI file
#if defined VERIFY_PHPBB3 && !defined TNL_OS_WIN32    
#  define GAME_JOLT
#endif

namespace Master 
{

namespace IniKey
{
enum SettingsItem {
#define SETTINGS_ITEM(a, enumVal, c, d, e, f) enumVal,
    MASTER_SETTINGS_TABLE
#undef SETTINGS_ITEM
};
}



class MasterSettings
{
private:
   void loadSettingsFromINI();
   string getCurrentMOTDFromFile(const string &filename) const;

   map<U32, string> motdClientMap;

public:
   MasterSettings(const string &iniFile);     // Constructor --> here all the keys, vals, and defaults are defined

   void readConfigFile();

   // Simplify access
   template <typename T>
   T getVal(IniKey::SettingsItem name) const
   {
      return mSettings.getVal<T>(name);
   }

   Settings<IniKey::SettingsItem> mSettings;
   CIniFile ini;

   string getMotd(U32 clientBuildVersion = U32_MAX) const;
};


class DatabaseAccessThread;

class MasterServer 
{
private:
   U32 mStartTime;
   MasterSettings *mSettings;
   NetInterface *mNetInterface;

   Timer mCleanupTimer;
   Timer mReadConfigTimer;

   Timer mJsonWriteTimer;
   bool mJsonWritingSuspended;

   Timer mPingGameJoltTimer;

   DatabaseAccessThread *mDatabaseAccessThread;

   Vector<MasterServerConnection *> mServerList;
   Vector<MasterServerConnection *> mClientList;

   NetInterface *createNetInterface() const;

public:
   MasterServer(MasterSettings *settings);      // Constructor
   ~MasterServer();                             // Destructor

   U32 getStartTime() const;

   const MasterSettings *getSettings() const;

   template <typename T>
   T getSetting(IniKey::SettingsItem name) const
   {
      return mSettings->getVal<T>(name);
   }


   NetInterface *getNetInterface() const;
   DatabaseAccessThread *getDatabaseAccessThread();
   void writeJsonDelayed();
   void writeJsonNow();

   const Vector<MasterServerConnection *> *getServerList() const;
   const Vector<MasterServerConnection *> *getClientList() const;

   void addServer(MasterServerConnection *server);
   void addClient(MasterServerConnection *client);

   void removeServer(S32 index);
   void removeClient(S32 index);

   void idle(const U32 timeDelta);
};

}

