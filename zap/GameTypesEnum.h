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

#ifndef _GAMETYPESENUM_H_
#define _GAMETYPESENUM_H_

namespace Zap
{     
/**
 * @luaenum GameType(1,2)
 * The GameType enum represets the different types of game.
 */
//                 Enum               GameType             LuaEnum     GameType Name                      
#define GAME_TYPE_TABLE \
   GAME_TYPE_ITEM( BitmatchGame,    "GameType",            "Bitmatch", "Bitmatch"         ) \
   GAME_TYPE_ITEM( CTFGame,         "CTFGameType",         "CTF",      "Capture the Flag" ) \
   GAME_TYPE_ITEM( HTFGame,         "CoreGameType",        "HTF",      "Hold the Flag"    ) \
   GAME_TYPE_ITEM( NexusGame,       "HTFGameType",         "Nexus",    "Nexus"            ) \
   GAME_TYPE_ITEM( RabbitGame,      "NexusGameType",       "Rabbit",   "Rabbit"           ) \
   GAME_TYPE_ITEM( RetrieveGame,    "RabbitGameType",      "Retrieve", "Retrieve"         ) \
   GAME_TYPE_ITEM( SoccerGame,      "RetrieveGameType",    "Soccer",   "Soccer"           ) \
   GAME_TYPE_ITEM( ZoneControlGame, "SoccerGameType",      "ZC",       "Zone Control"     ) \
   GAME_TYPE_ITEM( CoreGame,        "ZoneControlGameType", "Core",     "Core"             ) \


   // Define an enum from the first column of GAME_TYPE_TABLE
   enum GameTypeId {
#  define GAME_TYPE_ITEM(enumValue, b, c, d) enumValue,
       GAME_TYPE_TABLE
#  undef GAME_TYPE_ITEM
       NoGameType,
       GameTypesCount
   };


};


#endif
