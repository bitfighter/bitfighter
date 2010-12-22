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

namespace Zap
{

// Constructor
Statistics::Statistics() : mShots(WeaponCount), mHits(WeaponCount)
{
   for(S32 i = 0; i < WeaponCount; i++)
   {
      mShots.push_back(0);
      mHits.push_back(0);
   }

   mKills = 0;
   mDeaths = 0;
   mSuicides = 0;
   mFratricides = 0;
}


void Statistics::countShot(WeaponType weaponType)
{
   mShots[(S32) weaponType]++;
}


void Statistics::countHit(WeaponType weaponType)
{
   mHits[(S32) weaponType]++;
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


Vector<U16> Statistics::getShotsVector()
{
   return mShots;
}
   

Vector<U16> Statistics::getHitsVector()
{
   return mHits;
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


// Player killed another player
void Statistics::addKill()
{
   mKills++;
}

// Report cumulated kills
U16 Statistics::getKills()
{
   return mKills;
}


// Player got killed
void Statistics::addDeath()
{
   mDeaths++;
}

// Report cumulated deaths
U16 Statistics::getDeaths()
{
   return mDeaths;
}


// Player killed self
void Statistics::addSuicide()
{
   mSuicides++;
}

// Report cumulated suicides
U16 Statistics::getSuicides()
{
   return mSuicides;
}



// Player killed teammate
void Statistics::addFratricide()
{
   mFratricides++;
}

// Report cumulated fratricides
U16 Statistics::getFratricides()
{
   return mFratricides;
}


}

