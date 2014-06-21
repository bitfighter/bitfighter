//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
#ifndef _HTFGAMETYPE_H_
#define _HTFGAMETYPE_H_

#include "gameType.h"


namespace Zap
{

class HTFGameType : public GameType
{
   typedef GameType Parent;
   static StringTableEntry aString;
   static StringTableEntry theString;

   U32 mFlagScoreTime;     // Time flag is in your zone to get points for your team

public:
   HTFGameType();    // Constructor
   virtual ~HTFGameType();

   bool processArguments(S32 argc, const char **argv, Level *level);
   string toLevelCode() const;

#ifndef ZAP_DEDICATED
   // Editor menu
   Vector<string> getGameParameterMenuKeys();
   boost::shared_ptr<MenuItem> getMenuItem(const string &key);
   bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   void setFlagScore(S32 pointsPerMinute);
   S32 getFlagScore() const;

   bool isFlagGame() const;

   // Note -- neutral or enemy-to-all robots can't pick up the flag!!!  When we add robots, this may be important!!!
   void shipTouchFlag(Ship *theShip, FlagItem *theFlag);

   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);

   void shipTouchZone(Ship *s, GoalZone *z);


   void idle(BfObject::IdleCallPath path, U32 deltaT);

   // Same code as in retrieveGame, CTF
   void performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo);

   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;
   bool isTeamGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;

   // What does a particular scoring event score?
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   TNL_DECLARE_CLASS(HTFGameType);
};

};

#endif
