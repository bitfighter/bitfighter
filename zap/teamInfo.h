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

#ifndef _TEAM_INFO_H_
#define _TEAM_INFO_H_

#include "luaGameInfo.h"
#include "flagItem.h"
#include "tnl.h"
#include "UIEditor.h"   // For nameLen

namespace Zap
{

class Team
{  
protected:
   S32 mScore;

public:
   StringTableEntry _name;
   Color color;
   Vector<Point> spawnPoints;
   Vector<FlagSpawn> flagSpawnPoints;   // List of places for team flags to spawn

   S32 numPlayers;                      // Needs to be computed before use, not dynamically tracked
   F32 rating;

   Team() {numPlayers = 0; mScore = 0; rating = 0; }     // Quickie constructor

   void setName(const char *name) { _name = name; }
   void setName(StringTableEntry name) { _name = name; }
   StringTableEntry getName() { return _name.getString(); }

   S32 getScore() { return mScore; }
   void setScore(S32 score) { mScore = score; }
   void addScore(S32 score) { mScore += score; }
};


////////////////////////////////////////
////////////////////////////////////////

// Represents teams in the editor

class TeamEditor : public Team
{
public:
   LineEditor _name;   

   TeamEditor() {  _name = LineEditor(nameLen); numPlayers = 0; mScore = 0; rating = 0; }     // Quickie constructor

   void setName(const char *name) { _name.setString(name); }
   const char *getName() { return _name.c_str(); }
};


////////////////////////////////////////
////////////////////////////////////////


class LuaTeamInfo : public LuaObject
{

private:
   U32 mTeamIndex;
   Team mTeam;

public:
   LuaTeamInfo(lua_State *L);      // Lua constructor
   LuaTeamInfo(Team team);         // C++ constructor

   ~LuaTeamInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaTeamInfo>::RegType methods[];

   S32 getName(lua_State *L);
   S32 getIndex(lua_State *L);
   S32 getPlayerCount(lua_State *L);
   S32 getScore(lua_State *L);
   S32 getPlayers(lua_State *L);
};


};

#endif