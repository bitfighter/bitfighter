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

#include "masterInterface.h"

#include "MasterServerConnection.h"
#include "DatabaseAccessThread.h"

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
};


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

   DatabaseAccessThread mDatabaseAccessThread;

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

   void idle(U32 timeDelta);
};

}

