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
#include "gameItems.h"        // For FlagSpawn def
#include "lineEditor.h"
#include "tnl.h"

namespace Zap
{

static const S32 MAX_NAME_LEN = 256;

class AbstractTeam
{
protected:
   LineEditor _name;
public:
   static const S32 MAX_TEAM_NAME_LENGTH = 32;

   Color color;
   S32 numPlayers;      // Number of human players --> Needs to be computed before use, not dynamically tracked (see countTeamPlayers())
   S32 numBots;         // Number of robot players --> Needs to be computed before use, not dynamically tracked

   virtual void setName(const char *name) { _name.setString(name); }
   virtual StringTableEntry getName() { return StringTableEntry(_name.c_str()); }  // Wrap in STE to make signatures match... lame.
   LineEditor *getLineEditor() { return &_name; }

   void readTeamFromLevelLine(S32 argc, const char **argv);          // Read team info from level line
};

////////////////////////////////////////
////////////////////////////////////////

// Class for managing teams in the game
class Team : public AbstractTeam
{  
private:
   //static U32 mNextId;     // The problem is it never goes back to zero, so it keeps counting up and up, to > 100 ?
                  // As with any statics, it can cause problems with (in the future) multiplayer split screen clients in one game

protected:
   S32 mScore;

public:
   S32 mId;                // Helps keep track of teams after they've been sorted
   StringTableEntry _name;
   Vector<Point> spawnPoints;
   Vector<FlagSpawn> flagSpawnPoints;   // List of places for team flags to spawn

   F32 rating; 

   Team() { numPlayers = 0; numBots = 0; mScore = 0; rating = 0;}// mId = mNextId++;}     // Quickie constructor

   void setName(const char *name) { _name = name; }
   void setName(StringTableEntry name) { _name = name; }
   StringTableEntry getName() { return _name.getString(); }

   S32 getScore() { return mScore; }
   S32 getId() { return mId; }
   void setScore(S32 score) { mScore = score; }
   void addScore(S32 score) { mScore += score; }
};


////////////////////////////////////////
////////////////////////////////////////

// Class for managing teams in the editor
class TeamEditor : public AbstractTeam
{
public:
   TeamEditor() { _name = LineEditor(MAX_TEAM_NAME_LENGTH); numPlayers = 0; }     // Quickie constructor
};


////////////////////////////////////////
////////////////////////////////////////

class LuaTeamInfo : public LuaObject
{

private:
   S32 mTeamIndex;                  // Robots could potentially be on neutral or hostile team; maybe observer player could too
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


