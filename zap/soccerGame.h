//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SOCCERGAME_H_
#define _SOCCERGAME_H_

#include "gameType.h"

namespace Zap
{

class Ship;
class SoccerBallItem;

class SoccerGameType : public GameType
{
   typedef GameType Parent;

private:
   SafePtr<SoccerBallItem> mBall;

   U32 mHatTrickCounter;
   ClientInfo *mPossibleHatTrickPlayer;

public:
   SoccerGameType();
   virtual ~SoccerGameType();

   void scoreGoal(Ship *ship, const StringTableEntry &lastPlayerTouchName, S32 lastPlayerTouchTeam, const Point &scorePos, S32 goalTeamIndex, S32 score);
   void updateSoccerScore(Ship *ship, S32 scoringTeam, ScoringEvent scoringEvent, S32 score);   // Helper function to make sure the two-arg version of updateScore doesn't get a null ship

   void onOvertimeStarted();

   void setBall(SoccerBallItem *theBall);
   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   // Messages
   enum {
      SoccerMsgScoreGoal,
      SoccerMsgScoreOwnGoal,
      SoccerMsgGameOverTeamWin,
      SoccerMsgGameOverTie,
   };

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;

   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;

   TNL_DECLARE_RPC(s2cSoccerScoreMessage, (U32 msgIndex, StringTableEntry clientName, RangedU32<0, GameType::gMaxTeamCount> teamIndex, Point scorePos));
   TNL_DECLARE_CLASS(SoccerGameType);
};


////////////////////////////////////////
////////////////////////////////////////

class SoccerBallItem : public MoveItem
{
   typedef MoveItem Parent;

private:
   Point mInitialPos;
   Timer mSendHomeTimer;
   S32 mLastPlayerTouchTeam;
   F32 mDragFactor;
   StringTableEntry mLastPlayerTouchName;
   SafePtr<Ship> mLastPlayerTouch;
   bool mLuaBall;

public:
   explicit SoccerBallItem(lua_State *L = NULL);      // Combined Lua / C++ default constructor
   virtual ~SoccerBallItem();                // Destructor

   SoccerBallItem *clone() const;

   static const F32 SOCCER_BALL_RADIUS;

   void renderItem(const Point &pos);
   void resetPlayerTouch();
   void sendHome();
   void damageObject(DamageInfo *theInfo);
   void idle(BfObject::IdleCallPath path);
   bool processArguments(S32 argc, const char **argv, Level *level);
   string toLevelCode() const;

   void onAddedToGame(Game *game);

   bool collide(BfObject *hitObject);

   TNL_DECLARE_CLASS(SoccerBallItem);

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   void renderDock();
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   const Color &getColor() const;


   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(SoccerBallItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

};

};


#endif


