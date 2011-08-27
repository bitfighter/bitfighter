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

#include "luaGameInfo.h"      // For LuaObject def
#include "Spawn.h"            // For FlagSpawn def
#include "lineEditor.h"
#include "tnl.h"


namespace Zap
{

static const S32 MAX_NAME_LEN = 256;

class AbstractTeam
{
private:
   Color mColor;

public:
   static const S32 MAX_TEAM_NAME_LENGTH = 32;

   void setColor(F32 r, F32 g, F32 b) { mColor.set(r,g,b); } 
   const Color *getColor() const { return &mColor; }

   virtual void setName(const char *name) = 0;
   virtual StringTableEntry getName() = 0;

   bool processArguments(S32 argc, const char **argv);          // Read team info from level line
   string toString();

   void alterRed(F32 amt);
   void alterGreen(F32 amt);
   void alterBlue(F32 amt);
};


////////////////////////////////////////
////////////////////////////////////////

struct TeamInfo
{
   Color color;
   string name;
};


////////////////////////////////////////
////////////////////////////////////////

// Class for managing teams in the game
class Team : public AbstractTeam
{  
private:
   StringTableEntry mName;

   S32 mPlayerCount;      // Number of human players --> Needs to be computed before use, not dynamically tracked (see countTeamPlayers())
   S32 mBotCount;         // Number of robot players --> Needs to be computed before use, not dynamically tracked

   S32 mScore;
   F32 mRating; 

   Vector<Point> mItemSpawnPoints;
   Vector<FlagSpawn> mFlagSpawns;       // List of places for team flags to spawn

public:
   S32 mId;                             // Helps keep track of teams after they've been sorted

   Team();              // Constructor
   virtual ~Team();     // Destructor

   void setName(const char *name) { mName.set(name); }
   void setName(StringTableEntry name) { mName = name; }

   S32 getSpawnPointCount() const { return mItemSpawnPoints.size(); }
   Point getSpawnPoint(S32 index) const { return mItemSpawnPoints[index]; }
   void addSpawnPoint(Point point) { mItemSpawnPoints.push_back(point); }

   void addFlagSpawn(FlagSpawn flagSpawn) { mFlagSpawns.push_back(flagSpawn); }
   const Vector<FlagSpawn> *getFlagSpawns() const { return &mFlagSpawns; }
  

   StringTableEntry getName() { return mName; }  

   S32 getId() { return mId; }
   
   S32 getScore() { return mScore; }
   void setScore(S32 score) { mScore = score; }
   void addScore(S32 score) { mScore += score; }

   F32 getRating() { return mRating; }
   void addRating(F32 rating) { mRating += rating; }

   void clearStats();

   // Note that these values need to be precalulated before they are ready for use; they are not dynamically updated!
   S32 getPlayerCount() { return mPlayerCount; }                  // Get number of human players on team
   S32 getBotCount() { return mBotCount; }                        // Get number of bots on team
   S32 getPlayerBotCount() { return mPlayerCount + mBotCount; }   // Get total number of players/bots on team

   void incrementPlayerCount() { mPlayerCount++; }
   void incrementBotCount() { mBotCount++; }
};


////////////////////////////////////////
////////////////////////////////////////

// Class for managing teams in the editor
class TeamEditor : public AbstractTeam
{
private:
   LineEditor mNameEditor;

public:
   TeamEditor() { mNameEditor = LineEditor(MAX_TEAM_NAME_LENGTH); }              // Quickie constructor
   virtual ~TeamEditor() {}

   LineEditor *getLineEditor() { return &mNameEditor; }
   void setName(const char *name) { mNameEditor.setString(name); }
   StringTableEntry getName() { return StringTableEntry(mNameEditor.c_str()); }  // Wrap in STE to make signatures match
};


////////////////////////////////////////
////////////////////////////////////////

class LuaTeamInfo : public LuaObject
{

private:
   S32 mTeamIndex;                // Robots could potentially be on neutral or hostile team; maybe observer player could too
   Team *mTeam;

public:
   LuaTeamInfo(lua_State *L);     // Lua constructor
   LuaTeamInfo(Team *team);       // C++ constructor

   ~LuaTeamInfo();                // Destructor

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


