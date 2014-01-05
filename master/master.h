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

namespace Zap {
   struct VersionedGameStats;       // gameStats.h
   struct GameStats;
}


namespace Master 
{


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
   T getVal(const string &name) const
   {
      return mSettings.getVal<T>(name);
   }

   Settings mSettings;
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
   T getSetting(const string &name) const
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

