//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _RABBITGAME_H_
#define _RABBITGAME_H_

#include "gameType.h"

namespace Zap
{

class Ship;
class FlagItem;

class RabbitGameType : public GameType
{
   typedef GameType Parent;

private:
   U32 mFlagReturnTimer;
   U32 mFlagScoreTimer;

   Vector<string> makeParameterMenuKeys() const;

public:
   enum
   {
      RabbitMsgGrab,
      RabbitMsgRabbitKill,
      RabbitMsgRabbitDead,
      RabbitMsgDrop,
      RabbitMsgReturn,
      RabbitMsgGameOverWin,
      RabbitMsgGameOverTie
   };

   RabbitGameType();  // Constructor
   virtual ~RabbitGameType();

   bool processArguments(S32 argc, const char **argv, Level *level);
   string toLevelCode() const;

#ifndef ZAP_DEDICATED
   const Vector<string> *getGameParameterMenuKeys() const;
   boost::shared_ptr<MenuItem> getMenuItem(const string &key) const;
   bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   void idle(BfObject::IdleCallPath path, U32 deltaT);

   void addFlag(FlagItem *flag);
   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);
   void shipTouchFlag(Ship *ship, FlagItem *flag);

   bool objectCanDamageObject(BfObject *damager, BfObject *victim);
   void controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject);
   bool shipHasFlag(const Ship *ship) const;

   bool teamHasFlag(S32 teamIndex) const;
   void onFlagMounted(S32 teamIndex);

   const Color &getTeamColor(const BfObject *object) const;

   void onFlagHeld(Ship *ship);
   void onFlaggerDead(Ship *killerShip);
   void onFlaggerKill(Ship *rabbitShip);
   void onFlagReturned();

   void setFlagScore(S32 pointsPerMinute);
   S32 getFlagScore() const;

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;

   bool isFlagGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


   bool isSpawnWithLoadoutGame();

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   TNL_DECLARE_RPC(s2cRabbitMessage, (U32 msgIndex, StringTableEntry clientName));
   TNL_DECLARE_CLASS(RabbitGameType);
};

};


#endif


