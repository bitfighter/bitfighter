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
#include "ServerGame.h"
#include "robot.h"
#include "Colors.h"

#include "stringUtils.h"

namespace Zap
{

//U32 Team::mNextId = 0; // ???  when does it go back to zero?

// Constructor
AbstractTeam::AbstractTeam()
{
   mTeamIndex = -1;
}


// Destructor
AbstractTeam::~AbstractTeam()
{
   // Do nothing
}


void AbstractTeam::setColor(F32 r, F32 g, F32 b)
{
   mColor.set(r,g,b);
}


void AbstractTeam::setColor(const Color &color)
{
   mColor.set(color);
}


const Color *AbstractTeam::getColor() const
{
   return &mColor;
}


void AbstractTeam::setTeamIndex(S32 index)
{
   mTeamIndex = index;
}


// Read team from level file params
bool AbstractTeam::processArguments(S32 argc, const char **argv)
{
   if(argc < 5)               // Not enough arguments!
      return false;

   setName(argv[1]);
   mColor.read(argv + 2);

   return true;
}


string AbstractTeam::toLevelCode() const
{
   return string("Team ") + writeLevelString(getName().getString()) + " " + mColor.toRGBString();
}


void AbstractTeam::alterRed(F32 amt) 
{
   mColor.r += amt;

   if(mColor.r < 0)
      mColor.r = 0;
   else if(mColor.r > 1)
      mColor.r = 1;
}


void AbstractTeam::alterGreen(F32 amt) 
{
   mColor.g += amt;

   if(mColor.g < 0)
      mColor.g = 0;
   else if(mColor.g > 1)
      mColor.g = 1;
}


void AbstractTeam::alterBlue(F32 amt) 
{
   mColor.b += amt;

   if(mColor.b < 0)
      mColor.b = 0;
   else if(mColor.b > 1)
      mColor.b = 1;
}



////////////////////////////////////////
////////////////////////////////////////


// Constructor
Team::Team()
{
   mPlayerCount = 0;
   mBotCount = 0;
   mScore = 0;
   mRating = 0;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
Team::~Team()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


void Team::clearStats()
{
   mPlayerCount = 0;
   mBotCount = 0;
   mRating = 0;
}


void Team::setName(const char *name)
{
   mName.set(name);
}


void Team::setName(StringTableEntry name)
{
   mName = name;
}


StringTableEntry Team::getName() const
{
   return mName;
}


S32 Team::getScore()
{
   return mScore;
}


void Team::setScore(S32 score)
{
   mScore = score;
}


void Team::addScore(S32 score)
{
   mScore += score;
}


F32 Team::getRating()
{
   return mRating;
}


void Team::addRating(F32 rating)
{
   mRating += rating;
}


S32 Team::getPlayerCount()
{
   return mPlayerCount;
}


S32 Team::getBotCount()
{
   return mBotCount;
}


S32 Team::getPlayerBotCount()
{
   return mPlayerCount + mBotCount;
}


void Team::incrementPlayerCount()
{
   mPlayerCount++;
}


void Team::incrementBotCount()
{
   mBotCount++;
}


/**
 *  @luaclass Team
 *  @brief    Get information about a team in the current game
 *  @descr    The %Team object contains data about each team in a game.
 *
 */
//                Fn name                  Param profiles            Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getIndex,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getName,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getScore,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayerCount,    ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayers,        ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_FUNARGS_TABLE(Team, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(Team, LUA_METHODS);

#undef LUA_METHODS


const char *Team::luaClassName = "Team";  // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(Team);



// We'll add 1 to the index to allow the first team in Lua to have index of 1, and the first team in C++ to have an index of 0
S32 Team::lua_getIndex(lua_State *L)
{
   return returnInt(L, mTeamIndex + 1);
}


S32 Team::lua_getName(lua_State *L)
{
   return returnString(L, mName.getString());
}


S32 Team::lua_getScore(lua_State *L)
{
   return returnInt(L, mScore);
}


S32 Team::lua_getPlayerCount(lua_State *L)
{
   gServerGame->countTeamPlayers();    // Make sure player counts are up-to-date
   return returnInt(L, mPlayerCount);
}


// Return a table listing all players on this team.  Is there a better way to do this?
S32 Team::lua_getPlayers(lua_State *L)
{
   ServerGame *game = gServerGame;

   TNLAssert(game->getPlayerCount() == game->getClientCount(), "Mismatched player counts!");

   S32 pushed = 0;

   lua_newtable(L);    // Create a table, with no slots pre-allocated for our data

   for(S32 i = 0; i < game->getClientCount(); i++)
   {
      ClientInfo *clientInfo = game->getClientInfo(i);

      if(clientInfo->getTeamIndex() == mTeamIndex)
      {
         clientInfo->getPlayerInfo()->push(L);
         pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
         lua_rawseti(L, 1, pushed);
      }
   }

   for(S32 i = 0; i < game->getRobotCount(); i ++)
   {
      if(game->getBot(i)->getTeam() == mTeamIndex)
      {
         game->getBot(i)->getPlayerInfo()->push(L);
         pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
         lua_rawseti(L, 1, pushed);
      }
   }

   return 1;
}


////////////////////////////////////////
////////////////////////////////////////

// Destructor
TeamManager::~TeamManager()
{
   clearTeams();
}


S32 TeamManager::getTeamCount()
{
   return mTeams.size();
}


const Color *TeamManager::getTeamColor(S32 index) const
{
   if(index == TEAM_NEUTRAL)
      return &Colors::NeutralTeamColor;
   else if(index == TEAM_HOSTILE)
      return &Colors::HostileTeamColor;
   else if((U32)index < (U32)mTeams.size())     // Using U32 lets us handle goofball negative team numbers without explicitly checking for them
      return mTeams[index]->getColor();
   else
      return &Colors::magenta;                  // Use a rare color to let user know an object has an out of range team number
}


AbstractTeam *TeamManager::getTeam(S32 teamIndex)
{
   return mTeams[teamIndex];
}


void TeamManager::removeTeam(S32 teamIndex)
{
   mTeams.erase(teamIndex);
   mTeamHasFlagList.resize(mTeams.size());
}


void TeamManager::addTeam(AbstractTeam *team)
{
   mTeams.push_back(team);
   mTeamHasFlagList.resize(mTeams.size());
   mTeamHasFlagList[mTeamHasFlagList.size() - 1] = false;

   team->setTeamIndex(mTeams.size() - 1);  // Size of mTeams - 1 should be the index
}


void TeamManager::addTeam(AbstractTeam *team, S32 index)
{
   mTeams.insert(index);
   mTeams[index] = team;
   mTeamHasFlagList.resize(mTeams.size());
   mTeamHasFlagList[index] = false;

   team->setTeamIndex(index);
}


bool TeamManager::getTeamHasFlag(S32 teamIndex) const
{
   return mTeamHasFlagList[teamIndex] != 0;
}


void TeamManager::setTeamHasFlag(S32 teamIndex, bool hasFlag)
{
   mTeamHasFlagList[teamIndex] = hasFlag ? 1 : 0;
}


void TeamManager::clearTeamHasFlagList()
{
   for(S32 i = 0; i < mTeamHasFlagList.size(); i++)
      mTeamHasFlagList[i] = 0;
}


void TeamManager::replaceTeam(AbstractTeam *team, S32 index)
{
   mTeams[index] = team;

   team->setTeamIndex(index);
}


void TeamManager::clearTeams()
{
   mTeams.clear();
   mTeamHasFlagList.clear();
}


};


