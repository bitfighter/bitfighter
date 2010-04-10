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

#include "teamInfo.h"
#include "playerInfo.h"
#include "gameType.h"

namespace Zap
{

const char LuaTeamInfo::className[] = "TeamInfo";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<LuaTeamInfo>::RegType LuaTeamInfo::methods[] =
{
   method(LuaTeamInfo, getName),
   method(LuaTeamInfo, getIndex),
   method(LuaTeamInfo, getPlayerCount),
   method(LuaTeamInfo, getScore),
   method(LuaTeamInfo, getPlayers),

   {0,0}    // End method list 
};


// Lua constructor
LuaTeamInfo::LuaTeamInfo(lua_State *L)
{
   static const char *methodName = "TeamInfo constructor";
   checkArgCount(L, 1, methodName);

   // Lua thinks first team has index 1... we know better, but we need to play along...
   U32 teamIndx = (U32) getInt(L, 1, methodName, 1, gServerGame->getGameType()->mTeams.size()) - 1;

   mTeam = gServerGame->getGameType()->mTeams[teamIndx];
   mTeamIndex = teamIndx;
}


// C++ constructor
LuaTeamInfo::LuaTeamInfo(Team team)
{
   mTeam = team;

   const char *teamName = team.getName().getString();

   Vector<Team> teams = gServerGame->getGameType()->mTeams;

   for(S32 i = 0; i < teams.size(); i ++)
      if(!strcmp(teams[i].getName().getString(), teamName))
      {
         mTeamIndex = i;
         break;
      }
}

// Destructor
LuaTeamInfo::~LuaTeamInfo()
{
   logprintf("deleted LuaTeamInfo (%p)\n", this);     // Never gets run...
}


// We'll add 1 to the index to allow the first team in Lua to have index of 1, and the first team in C++ to have an index of 0
S32 LuaTeamInfo::getIndex(lua_State *L) { return returnInt(L, mTeamIndex + 1); }                  // getTeamIndex() ==> return team's index (returns int)
S32 LuaTeamInfo::getName(lua_State *L)  { return returnString(L, mTeam.getName().getString()); }  // getTeamName() ==> return team name (returns string)
S32 LuaTeamInfo::getScore(lua_State *L) { return returnInt(L, mTeam.getScore()); }                // getScore() ==> return team score (returns int)


S32 LuaTeamInfo::getPlayerCount(lua_State *L)         // number getPlayerCount() ==> return player count
{
   gServerGame->getGameType()->countTeamPlayers();    // Make sure player counts are up-to-date
   return returnInt(L, mTeam.numPlayers);
}


// Return a table listing all players on this team
S32 LuaTeamInfo::getPlayers(lua_State *L)
{
   TNLAssert(gServerGame->getPlayerCount() == gServerGame->getGameType()->mClientList.size(), "Mismatched player counts!");

   S32 pushed = 0;

   lua_newtable(L);    // Create a table, with no slots pre-allocated for our data

   for(S32 i = 0; i < gServerGame->getGameType()->mClientList.size(); i++)
   {
      ClientRef *clientRef = gServerGame->getGameType()->mClientList[i];

      if(clientRef->getTeam() == mTeamIndex)
      {
         clientRef->getPlayerInfo()->push(L);
         pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
         lua_rawseti(L, 1, pushed);
      }
   }

   for(S32 i = 0; i < gServerGame->getRobotCount(); i ++)
   {
      if(Robot::robots[i]->getTeam() == mTeamIndex)
      {
         Robot::robots[i]->getPlayerInfo()->push(L);
         pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
         lua_rawseti(L, 1, pushed);
      }
   }

   return 1;
}


};