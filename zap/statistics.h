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

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include "gameWeapons.h"      // For WeaponType enum
#include "shipItems.h"        // for ModuleCount

#include "tnlTypes.h"
#include "tnlVector.h"


using namespace TNL;

namespace Zap
{

// Class to keep track of player statistics
class Statistics
{
private:
   U32 mShots[WeaponCount];
   U32 mHits[WeaponCount];
   U32 mHitBy[WeaponCount];
   U32 mModuleUsedTime[ModuleCount];

   U32 mKills;             // Enemy kills
   U32 mDeaths;            // Times died
   U32 mSuicides;          // Self kills
   U32 mFratricides;       // Teammate kills
   U64 mDist;              // Total distance traveled -- note that this number is mulitplied by 10,000

   // Long term score tracking
   U32 mTotalKills;        // Total kills over the lifetime of this connection
   U32 mTotalFratricides;  // Total kills of teammates over the lifetime of this connection
   U32 mTotalDeaths;       // Total deaths over the lifetime of this connection
   U32 mTotalSuicides;     // Total suicides over the lifetime of this connection
   U32 mGamesPlayed;       // Number of games played, obviously

   Vector<U32> mLoadouts;

public:
   U32 mFlagPickup;
   U32 mFlagDrop;
   U32 mTurretsKilled;
   U32 mFFsKilled;
   U32 mFlagReturn;  // used as Flag steal in HTF and retrieve
   U32 mFlagScore;
   U32 mCrashedIntoAsteroid;
   U32 mChangedLoadout;
   U32 mTeleport;
   U32 mPlayTime;

   Statistics();        // Constructor

   void countShot(WeaponType weaponType);    // Record a shot
   void countHit(WeaponType weaponType);     // Record a hit
   void countHitBy(WeaponType weaponType);   // got hit by which weapon?

   S32 getShots();
   S32 getShots(WeaponType weaponType);

   S32 getHits();
   S32 getHits(WeaponType weaponType);

   F32 getHitRate();                  // Report overall hit rate
   F32 getHitRate(WeaponType weaponType);    // Report hit rate for specified weapon

   S32 getHitBy(WeaponType weaponType);

   void addModuleUsed(ShipModule, U32 milliseconds);
   U32 getModuleUsed(ShipModule);

   void addGamePlayed();

   void addKill();      // Player killed another player
   U32 getKills();      // Report cumulated kills

   void addDeath();     // Player got killed
   U32 getDeaths();     // Report cumulated deaths

   void addSuicide();   // Player killed self
   U32 getSuicides();   // Report cumulated suicides

   void addFratricide();   // Player killed teammate
   U32 getFratricides();   // Report cumulated fratricides

   void addLoadout(U32 loadoutHash);  // Add loadout hash
   Vector<U32> getLoadouts();         // Return all loadouts

   F32 getCalculatedRating();

   Vector<U32> getShotsVector();
   Vector<U32> getHitsVector();

   U32 getDistanceTraveled();

   void accumulateDistance(F32 dist);

   void resetStatistics();   // Reset Player Statistics (used at end of match)
};

};

#endif

