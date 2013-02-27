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

#include "statistics.h"
#include "gameWeapons.h"

namespace Zap
{

// Constructor
Statistics::Statistics()
{
   mTotalKills = 0;      
   mTotalFratricides = 0;
   mTotalDeaths = 0;     
   mTotalSuicides = 0; 
   mGamesPlayed = 0;

   resetStatistics();
}


void Statistics::countShot(WeaponType weaponType)
{
   TNLAssert(weaponType < WeaponCount, "Out of range");
   mShots[(S32) weaponType]++;
}


void Statistics::countHit(WeaponType weaponType)
{
   TNLAssert(weaponType < WeaponCount, "Out of range");
   mHits[(S32) weaponType]++;
}


void Statistics::countHitBy(WeaponType weaponType)
{
   TNLAssert(weaponType < WeaponCount, "Out of range");
   mHitBy[(S32) weaponType]++;
}


S32 Statistics::getShots()
{
   S32 totalShots = 0;

   for(S32 i = 0; i < WeaponCount; i++)
      totalShots += mShots[i];

   return totalShots;
}


S32 Statistics::getShots(WeaponType weaponType)
{
   return mShots[(S32)weaponType];
}


Vector<U32> Statistics::getShotsVector()
{
   Vector<U32>(shots);
   shots.resize(WeaponCount);
   for(S32 i = 0; i < WeaponCount; i++)
      shots[i] = mShots[i];
   return shots;
}


Vector<U32> Statistics::getHitsVector()
{
   Vector<U32>(hits);
   hits.resize(WeaponCount);
   for(S32 i = 0; i < WeaponCount; i++)
      hits[i] = mHits[i];
   return hits;
}


S32 Statistics::getHits()
{
   S32 totalHits = 0;

   for(S32 i = 0; i < WeaponCount; i++)
      totalHits += mHits[i];

   return totalHits;
}


S32 Statistics::getHits(WeaponType weaponType)
{
   return mHits[(S32)weaponType];
}


// Report overall hit rate
F32 Statistics::getHitRate()
{
   return (F32)getHits() / (F32)getShots();
}


// Report hit rate for specified weapon
F32 Statistics::getHitRate(WeaponType weaponType)
{
   return (F32)mHits[(S32)weaponType] / (F32)mShots[(S32)weaponType];
}


S32 Statistics::getHitBy(WeaponType weaponType)
{
   return mHitBy[(S32)weaponType];
}

void Statistics::addModuleUsed(ShipModule module, U32 milliseconds)
{
   TNLAssert(U32(module) < U32(ModuleCount), "ShipModule out of range");
   mModuleUsedTime[(S32)module] += milliseconds;
}


U32 Statistics::getModuleUsed(ShipModule module)
{
   TNLAssert(U32(module) < U32(ModuleCount), "ShipModule out of range");
   return mModuleUsedTime[(S32)module];
}


void Statistics::addGamePlayed()
{
   mGamesPlayed++;
}


// Player killed another player
void Statistics::addKill()
{
   mKills++;
   mTotalKills++;
}


// Report cumulated kills
U32 Statistics::getKills()
{
   return mKills;
}


// Player got killed
void Statistics::addDeath()
{
   mDeaths++;
   mTotalDeaths++;
}

// Report cumulated deaths
U32 Statistics::getDeaths()
{
   return mDeaths;
}


// Player killed self
void Statistics::addSuicide()
{
   mSuicides++;
   mTotalSuicides++;
}


// Report cumulated suicides
U32 Statistics::getSuicides()
{
   return mSuicides;
}


// Player killed teammate
void Statistics::addFratricide()
{
   mFratricides++;
   mTotalFratricides++;
}


// Report cumulated fratricides
U32 Statistics::getFratricides()
{
   return mFratricides;
}


void Statistics::addLoadout(U32 loadoutHash)
{
   mLoadouts.push_back(loadoutHash);
}


Vector<U32> Statistics::getLoadouts()
{
   return mLoadouts;
}


// Return a measure of a player's strength.
// Right now this is roughly a kill - death / kill + death ratio
// Better might be: https://secure.wikimedia.org/wikipedia/en/wiki/Elo_rating_system
F32 Statistics::getCalculatedRating()
{
   // Total kills = mKills + mFratricides (but we won't count mFratricides)
   // Counted deaths = mDeaths - mSuicides (mSuicides are included in mDeaths and we want to ignore them)
   // Use F32 here so we don't underflow with U32 math; probably not necessary
   F32 deathsDueToEnemyAction   = F32(mTotalDeaths) - F32(mTotalSuicides);
   F32 totalTotalKillsAndDeaths = F32(mTotalKills) + deathsDueToEnemyAction;

   // Initial case: you haven't killed or died -- go out and prove yourself, lad!
   if(totalTotalKillsAndDeaths == 0)
      return 0;

   // Standard case
   else   
      return ((F32)mTotalKills - deathsDueToEnemyAction) / totalTotalKillsAndDeaths;
}


static const U32 DIST_MULTIPLIER = 10000;

void Statistics::accumulateDistance(F32 dist)
{
   // We'll track distance as an integer to avoid data loss due to a small float being added to a big one.
   // Not that precision is important; it isn't... but rather to avoid appearance of "going no further" once
   // you've gone a certain distance.
   mDist += dist * DIST_MULTIPLIER;
}


U32 Statistics::getDistanceTraveled()
{
   if(mDist / DIST_MULTIPLIER > (U64)U32_MAX)
      return U32_MAX;
   else
      return mDist / DIST_MULTIPLIER;
}


// Gets called at beginning of each game -- stats listed here do not persist
void Statistics::resetStatistics()
{
   mKills = 0;
   mDeaths = 0;
   mSuicides = 0;
   mFratricides = 0;
   mDist = 0;

   for(S32 i = 0; i < WeaponCount; i++)
   {
      mShots[i] = 0;
      mHits[i] = 0;
      mHitBy[i] = 0;
   }

   for(S32 i = 0; i < ModuleCount; i++)
      mModuleUsedTime[i] = 0;

   mLoadouts.clear();

   mFlagPickup = 0;
   mFlagReturn = 0;
   mFlagScore = 0;
   mFlagDrop = 0;
   mCrashedIntoAsteroid = 0;
   mChangedLoadout = 0;
   mTeleport = 0;
   mPlayTime = 0;
}

}

