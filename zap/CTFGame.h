//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CTFGAME_H_
#define _CTFGAME_H_

#include "gameType.h"

namespace Zap
{

class Ship;
class FlagItem;

class CTFGameType : public GameType
{
private:
   typedef GameType Parent;

   bool mLastWinBadgeAchievable;
   ClientInfo *mPossibleLastWinBadgeAchiever;

public:
   CTFGameType();
   virtual ~CTFGameType();

   void addFlag(FlagItem *flag);
   void shipTouchFlag(Ship *ship, FlagItem *flag);
   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);
   void performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo);
   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;

   bool teamHasFlag(S32 teamIndex) const;
   void onFlagMounted(S32 teamIndex);

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;
   
   bool isFlagGame() const; 
   bool isTeamGame() const;
   bool canBeTeamGame()  const;
   bool canBeIndividualGame() const;

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   void onGameOver();

   TNL_DECLARE_CLASS(CTFGameType);
};

};


#endif

