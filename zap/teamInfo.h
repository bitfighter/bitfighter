//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEAM_INFO_H_
#define _TEAM_INFO_H_

#include "LuaBase.h"          // Parent class

#include "TeamPreset.h"       // For TeamPreset def
#include "LuaWrapper.h"
#include "TeamConstants.h"    // For TEAM_NEUTRAL et. al.
#include "Color.h"

#include "tnlNetStringTable.h"
#include "tnlNetBase.h"

#include <string>

namespace Zap
{

static const S32 MAX_NAME_LEN = 256;

class TeamInfo
{
protected:
   StringTableEntry mName;
   Color mColor;

public:
   TeamInfo();                            // Constructor
   TeamInfo(const TeamPreset &preset);    // Constructor
   virtual ~TeamInfo();                   // Destructor

   bool processArguments(S32 argc, const char **argv);     // Read team info from level line

   virtual void setName(const char *name);
   void setName(const StringTableEntry &name);
   virtual StringTableEntry getName() const;

   string toLevelCode() const;

   virtual void setColor(const Color &color); 
   virtual void setColor(const Color *color);
   virtual void setColor(F32 r, F32 g, F32 b);

   const Color &getColor() const;
};


////////////////////////////////////////
////////////////////////////////////////

class AbstractTeam : public RefPtrData, public TeamInfo
{
   typedef TeamInfo Parent;

protected:
   S32 mTeamIndex;           // Team index of this team according to the level file and game

public:
   AbstractTeam();           // Constructor
   virtual ~AbstractTeam();  // Destructor

   static const S32 MAX_TEAM_NAME_LENGTH = 32;

   void setTeamIndex(S32 index);

   void alterRed(F32 amt);
   void alterGreen(F32 amt);
   void alterBlue(F32 amt);

   void set(const TeamInfo &teamInfo);

   virtual S32 getPlayerBotCount() const; 
   virtual S32 getPlayerCount() const;      
   virtual S32 getBotCount() const;

   virtual S32 getScore() const;
   virtual void setScore(S32 score);
   virtual void addScore(S32 score);
};


////////////////////////////////////////
////////////////////////////////////////

class FlagSpawn;

// Class for managing teams in the game
class Team : public AbstractTeam
{  
   typedef AbstractTeam Parent;

private:
   S32 mPlayerCount;      // Number of human players --> Needs to be computed before use, not dynamically tracked (see countTeamPlayers())
   S32 mBotCount;         // Number of robot players --> Needs to be computed before use, not dynamically tracked

   S32 mScore;
   F32 mRating; 

   Vector<Point> mItemSpawnPoints;
   Vector<FlagSpawn *> mFlagSpawns;    // List of places for team flags to spawn

   void initialize();

public:
   // Constructors
   Team();                                                
   Team(const char *name, const Color &color);            
   Team(const char *name, F32 r, F32 g, F32 b, S32 score);
   Team(const TeamInfo &teamInfo);

   virtual ~Team();     // Destructor

   S32 getScore() const;
   void setScore(S32 score);
   void addScore(S32 score);

   F32 getRating();
   void addRating(F32 rating);     // For summing ratings of all players on a team

   void clearStats();

   // Players & bots on each team:
   // Note that these values need to be precalulated before they are ready for use;
   // they are not dynamically updated!
   S32 getPlayerCount() const;      // Get number of human players on team
   S32 getBotCount() const;         // Get number of bots on team
   S32 getPlayerBotCount() const;   // Get total number of players/bots on team

   void incrementPlayerCount();
   void incrementBotCount();

   ///// Lua interface
   LUAW_DECLARE_CLASS(Team);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_getName(lua_State *L);
   S32 lua_getIndex(lua_State *L);
   S32 lua_getPlayerCount(lua_State *L);
   S32 lua_getScore(lua_State *L);
   S32 lua_getPlayers(lua_State *L);
};


////////////////////////////////////////
////////////////////////////////////////

class TeamManager
{
private:
   Vector<RefPtr<AbstractTeam> > mTeams;
   Vector<S32> mTeamHasFlagList;      // Track which team (or teams) have the flag

public:
   virtual ~TeamManager();      // Destructor

   const Color &getTeamColor(S32 index) const;
   StringTableEntry getTeamName(S32 index) const;

   S32 getTeamCount() const;

   AbstractTeam *getTeam(S32 teamIndex) const;

   void removeTeam(S32 teamIndex);
   void addTeam(AbstractTeam *team);
   void addTeam(AbstractTeam *team, S32 index);
   void replaceTeam(AbstractTeam *team, S32 index);
   void clearTeams();

   S32 getBotCount() const;

   // Access to mTeamHasFlagList
   bool getTeamHasFlag(S32 teamIndex) const;
   void setTeamHasFlag(S32 teamIndex, bool hasFlag);
};


};

#endif


