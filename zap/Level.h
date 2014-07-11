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

class Level : public GridDatabase
{
   typedef GridDatabase Parent;

private:
   S32 mVersion;
   F32 mLegacyGridSize; 
   U32 mDatabaseId;
   SafePtr<GameType> mGameType;
   string mLevelHash;
   bool mAddedToGame;      // False until onAddedToGame() is called, then True

   U32 mLevelDatabaseId;

   Vector<string> mRobotLines;
   boost::shared_ptr<Vector<TeamInfo> > mTeamInfos;
   Vector<string> mTeamChangeLines;

   TeamManager mTeamManager;

   Vector<WallItem *>mWallItemList;    // Need to delete items that are removed from this list!
   Rect mWallItemExtents;

   void parseLevelLine(const string &line, const string &levelFileName);
   bool processLevelLoadLine(U32 argc, S32 id, const char **argv, string &errorMsg);  
   bool processLevelParam(S32 argc, const char **argv);

public:
   Level();             // Constructor
   virtual ~Level();    // Destructor

   static const U32 CurrentLevelFormat = 2;

   void onAddedToServerGame(Game *game);
   void onAddedToClientGame();

   void loadLevelFromString(const string &contents, const string &filename = "");
   bool loadLevelFromFile(const string &filename);
   void validateLevel();

   boost::shared_ptr<Vector<TeamInfo> > getTeamInfosClone() const;
   string toLevelCode() const;

   U32 getLevelDatabaseId() const;
   void setLevelDatabaseId(U32 id);


   const Vector<WallItem *> &getWallList() const;

   string getHash() const;
   F32 getLegacyGridSize() const;
   GameType *getGameType() const;
   void setGameType(GameType *gameType);

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