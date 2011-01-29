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
#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlBitStream.h"
#include "tnlNetStringTable.h"
#include "tnlString.h"
#include "database.h"     // For GameStats


using namespace TNL;



struct GameStatistics3
{
   U8 version;
   GameStats gameStats ;
};


namespace Types
{
   extern void read(TNL::BitStream &s, GameStatistics3 *val);
   extern void write(TNL::BitStream &s, GameStatistics3 &val);
}

/*
#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_VECTOR_H_
#include "tnlVector.h"
#endif

#ifndef _TNL_BITSTREAM_H_
#include "tnlBitStream.h"
#endif

#ifndef _TNL_NETSTRINGTABLE_H_
#include "tnlNetStringTable.h"
#endif

#ifndef _TNL_STRING_H_
#include "tnlString.h"
#endif

using namespace TNL;


struct GameStatistics3
{
StringTableEntry gameType;
bool teamGame;
StringTableEntry levelName;
Vector<StringTableEntry> teams;
Vector<S32> teamScores;
Vector<RangedU32<0,0xFFFFFF> > color;
U16 timeInSecs;
Vector<StringTableEntry> playerNames;
Vector<Vector<U8> > playerIDs;
Vector<bool> isBot;
Vector<bool> lastOnTeam;
Vector<S32> playerScores;
Vector<U16> playerKills;
Vector<U16> playerDeaths;
Vector<U16> playerSuicides;
Vector<Vector<U16> > shots;
Vector<Vector<U16> > hits;
Vector<U16> playerSwitchedTeamCount;
};

// If above struct is added, be sure to change masterInterface.cpp read and write so it can actually send data.
namespace Types
{
   extern void read(TNL::BitStream &s, GameStatistics3 *val);
   extern void write(TNL::BitStream &s, GameStatistics3 &val);
}
*/
