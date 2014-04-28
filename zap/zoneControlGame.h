//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ZONECONTROLGAMETYPE_H_
#define _ZONECONTROLGAMETYPE_H_

#include "gameType.h"      // Parent

namespace Zap
{
class Ship;

class ZoneControlGameType : public GameType
{
private:
   typedef GameType Parent;

   // Zone Controller badge
   bool mZcBadgeAchievable;
   ClientInfo *mPossibleZcBadgeAchiever;

public:
   ZoneControlGameType();  // Constructor
   virtual ~ZoneControlGameType();

   void shipTouchFlag(Ship *ship, FlagItem *flag);
   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);

   void shipTouchZone(Ship *ship, GoalZone *zone);

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;

   bool isFlagGame() const;
   bool isTeamGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;

   void onFlagMounted(S32 teamIndex);

   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;
   bool teamHasFlag(S32 teamIndex) const;

   void performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo);
   void majorScoringEventOcurred(S32 team);    // Gets run when a touchdown is scored

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   bool onGameOver();
   void onOvertimeStarted();

   TNL_DECLARE_CLASS(ZoneControlGameType);
};

};
#endif
