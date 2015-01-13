//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _RETRIEVEGAMETYPE_H_
#define _RETRIEVEGAMETYPE_H_

#include "gameType.h"


namespace Zap
{

class RetrieveGameType : public GameType
{
private:
   typedef GameType Parent;

public:
   RetrieveGameType();
   virtual ~RetrieveGameType();

   bool isFlagGame() const;

   // Note -- neutral or enemy-to-all robots can't pick up the flag!!!
   void shipTouchFlag(Ship *theShip, FlagItem *theFlag);

   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);

   // The ship has entered a drop zone, either friend or foe
   void shipTouchZone(Ship *s, GoalZone *z);


   // A major scoring event has ocurred -- in this case, it's all flags being collected by one team
   void majorScoringEventOcurred(S32 team);
   void onOvertimeStarted();

   // Same code as in HTF, CTF
   void performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo);

   // Runs on client
   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;

   // What does a particular scoring event score?
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;
   bool isTeamGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


   TNL_DECLARE_CLASS(RetrieveGameType);
};


};

#endif
