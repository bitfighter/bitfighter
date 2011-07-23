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
#include "tnlTypes.h"
#include "tnlVector.h"


using namespace TNL;

namespace Zap
{

// Class to keep track of player statistics
class Statistics
{
private:
   U16 mShots[WeaponCount];
   U16 mHits[WeaponCount];

   U16 mKills;          // Enemy kills
   U16 mDeaths;         // Times died
   U16 mSuicides;       // Self kills
   U16 mFratricides;    // Tammate kills

public:
   Statistics();        // Constructor

   void countShot(WeaponType weaponType);    // Record a shot
   void countHit(WeaponType weaponType);     // Record a hit

   S32 getShots();
   S32 getShots(WeaponType weaponType);

   S32 getHits();
   S32 getHits(WeaponType weaponType);

   F32 getHitRate();                  // Report overall hit rate
   F32 getHitRate(WeaponType weaponType);    // Report hit rate for specified weapon

   void addKill();      // Player killed another player
   U16 getKills();      // Report cumulated kills

   void addDeath();     // Player got killed
   U16 getDeaths();     // Report cumulated deaths

   void addSuicide();   // Player killed self
   U16 getSuicides();   // Report cumulated suicides

   void addFratricide();   // Player killed teammate
   U16 getFratricides();   // Report cumulated fratricides


   Vector<U16> getShotsVector();
   Vector<U16> getHitsVector();

   void resetStatistics();   // Reset Player Statistics (used at end of match)
};

};

#endif

