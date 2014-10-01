//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_H_
#define _LEVEL_H_

#include "gridDB.h"

#include "teamInfo.h"

#include "tnlTypes.h"
#include "tnlNetBase.h"

#include "boost/smart_ptr/shared_ptr.hpp"

#include <string>

using namespace std;
using namespace TNL;


namespace Zap
{

class Game;
class GameType;
class WallItem;
class PolyWall;
class BotNavMeshZone;

class Level : public GridDatabase
{
   typedef GridDatabase Parent;

private:
   S32 mVersion;
   F32 mLegacyGridSize; 
   U32 mDatabaseId;
   RefPtr<GameType> mGameType;
   string mLevelHash;

   Game *mGame;             // Game we've been added to -- NULL until onAddedToGame() is called

   U32 mLevelDatabaseId;

   Vector<string> mRobotLines;
   boost::shared_ptr<Vector<TeamInfo> > mTeamInfos;
   Vector<string> mTeamChangeLines;

   TeamManager mTeamManager;

   Vector<WallItem *>mWallItemList;    // Need to delete items that are removed from this list!

   // Zone-related
   GridDatabase mBotZoneDatabase;
   Vector<BotNavMeshZone *> mAllZones;

   void initialize();
   void parseLevelLine(const string &line, const string &levelFileName);
   bool processLevelLoadLine(U32 argc, S32 id, const char **argv, string &errorMsg);  
   bool processLevelParam(S32 argc, const char **argv);

public:
   Level();                         // Constructor
   Level(const string &levelCode);  // Constructor with passed levelcode, primarily used for testing
   virtual ~Level();                // Destructor

   Level *clone();      // Duplicate level

   static const U32 CurrentLevelFormat = 2;

   void onAddedToGame(Game *game);

   void loadLevelFromString(const string &contents, const string &filename = "");
   bool loadLevelFromFile(const string &filename);
   void validateLevel();

   void buildWallEdgeGeometry();
   void snapAllEngineeredItems(bool onlyUnsnapped);

   boost::shared_ptr<Vector<TeamInfo> > getTeamInfosClone() const;
   string toLevelCode() const;

   U32 getLevelDatabaseId() const;
   void setLevelDatabaseId(U32 id);

   void addWallItem(WallItem *wall, Game *game = NULL);

   const Vector<WallItem *> &getWallList() const;

   string getHash() const;
   F32 getLegacyGridSize() const;
   GameType *getGameType() const;
   void setGameType(GameType *gameType);

   // Note that these return modifiable copies!
   GridDatabase &getBotZoneDatabase();
   Vector<BotNavMeshZone *> &getBotZoneList();


   bool getAddedToGame() const;

   void addBots(Game *game);

   // These methods for giving a brain transplant to mDockItems
   void setTeamInfosPtr(const boost::shared_ptr<Vector<TeamInfo> > &teamInfos);
   boost::shared_ptr<Vector<TeamInfo> > getTeamInfosPtr() const;


   // Level metadata
   string getLevelName() const;
   string getLevelCredits() const;
   S32 getWinningScore() const;


   // Team methods
   S32 getTeamCount() const;
   const TeamInfo &getTeamInfo(S32 index) const;

   StringTableEntry getTeamName(S32 index) const;
   const Color &getTeamColor(S32 index) const;
   void removeTeam(S32 teamIndex); 

   void addTeam(AbstractTeam *team);
   void addTeam(const TeamInfo &teamInfo);
   void addTeam(AbstractTeam *team, S32 index);
   void addTeam(const TeamInfo &teamInfo, S32 index);    

   AbstractTeam *getTeam(S32 teamIndex) const;
   void setTeamHasFlag(S32 teamIndex, bool hasFlag);
   bool getTeamHasFlag(S32 teamIndex) const;

   void replaceTeam(AbstractTeam *team, S32 index);
   void clearTeams();             
   string getTeamLevelCode(S32 index) const;

   bool makeSureTeamCountIsNotZero();        // Because zero teams can cause crashiness


   S32 getBotCount() const;

   friend class ObjectTest;
};


}

#endif