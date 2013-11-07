//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "CTFGame.h"
#include "flagItem.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#endif

#include <stdio.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(CTFGameType);


// Destructor
CTFGameType::~CTFGameType()
{
   // Do nothing
}


void CTFGameType::addFlag(FlagItem *flag)
{
   Parent::addFlag(flag);

   if(!isGhost())
      addItemOfInterest(flag);      // Server only
}


void CTFGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   ClientInfo *clientInfo = theShip->getClientInfo();

   if(!clientInfo)
      return;

   if(theShip->getTeam() == theFlag->getTeam())      // Touch own flag
   {
      if(!theFlag->isAtHome())
      {
         static StringTableEntry returnString("%e0 returned the %e1 flag.");

         Vector<StringTableEntry> e;
         e.push_back(clientInfo->getName());
         e.push_back(getGame()->getTeamName(theFlag->getTeam()));
         
         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagReturn, returnString, e);

         theFlag->sendHome();

         updateScore(theShip, ReturnTeamFlag);

         clientInfo->getStatistics()->mFlagReturn++;
      }
      else     // Flag is at home
      {
         // Check if this client has an enemy flag mounted
         for(S32 i = 0; i < theShip->getMountedItemCount(); i++)
         {
            MountableItem *mountedItem = theShip->getMountedItem(i);

            if(mountedItem && mountedItem->getObjectTypeNumber() == FlagTypeNumber)
            {
               FlagItem *mountedFlag = static_cast<FlagItem *>(mountedItem);
               static StringTableEntry capString("%e0 captured the %e1 flag!");

               Vector<StringTableEntry> e;
               e.push_back(theShip->getClientInfo()->getName());
               e.push_back(getGame()->getTeamName(mountedFlag->getTeam()));

               broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, capString, e);

               mountedFlag->dismount(DISMOUNT_SILENT);
               mountedFlag->sendHome();

               updateScore(theShip, CaptureFlag);

               clientInfo->getStatistics()->mFlagScore++;
            }
         }
      }
   }
   else     // Touched enemy flag
   {
      // Make sure we don't already have a flag mounted... this will never happen in an ordinary game
      // But you can only carry one flag in CTF
      if(!theShip->isCarryingItem(FlagTypeNumber))
      {
         // Alert the clients
         static StringTableEntry takeString("%e0 took the %e1 flag!");
         Vector<StringTableEntry> e;
         e.push_back(clientInfo->getName());
         e.push_back(getGame()->getTeamName(theFlag->getTeam()));

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, takeString, e);

         theFlag->mountToShip(theShip);

         clientInfo->getStatistics()->mFlagPickup++;
      }
   }
}


// Identical to HTF, Retrieve
void CTFGameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
{
   GameConnection *connection = clientInfo->getConnection();

   Parent::performProxyScopeQuery(scopeObject, clientInfo);
   S32 uTeam = scopeObject->getTeam();

   // Scan all the flags and mark any that are at home or parked in a zone as being in scope; for those that are mounted,
   // if the mount is on our team, mark both the mount and the flag as being in scope

   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));

      if(flag->isAtHome() || flag->getZone())
         connection->objectInScope(flag);
      else
      {
         Ship *mount = flag->getMount();
         if(mount && mount->getTeam() == uTeam)
         {
            connection->objectInScope(mount);
            connection->objectInScope(flag);
         }
      }
   }
}


void CTFGameType::renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const
{
#ifndef ZAP_DEDICATED
   // Render basic overlay
   Parent::renderInterfaceOverlay(canvasWidth, canvasHeight);

   // Add some objective arrows...
   // ...but rendering objective arrows makes no sense if there is no ship at the moment
   Ship *ship = getGame()->getLocalPlayerShip();

   if(!ship)
      return;

   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));

      if(flag->isMounted())
      {
         Ship *mount = flag->getMount();
         if(mount)
            renderObjectiveArrow(mount, canvasWidth, canvasHeight);
      }
      else
         renderObjectiveArrow(flag, canvasWidth, canvasHeight);
   }
#endif
}


bool CTFGameType::teamHasFlag(S32 teamIndex) const
{
   return doTeamHasFlag(teamIndex);
}


void CTFGameType::onFlagMounted(S32 teamIndex)
{
   getGame()->setTeamHasFlag(teamIndex, true);
   notifyClientsWhoHasTheFlag();
}


class FlagItem;


void CTFGameType::itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode)
{
   Parent::itemDropped(ship, item, dismountMode);

   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {
      if(dismountMode != DISMOUNT_SILENT)
      {
         FlagItem *flag = static_cast<FlagItem *>(item);
         static StringTableEntry dropString("%e0 dropped the %e1 flag!");

         Vector<StringTableEntry> e;
         e.push_back(ship->getClientInfo()->getName());
         e.push_back(getGame()->getTeamName(flag->getTeam()));

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropString, e);
      }
   }
}


// What does a particular scoring event score?
S32 CTFGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   if(scoreGroup == TeamScore)
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 0;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return 0;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         case ReturnTeamFlag:
            return 0;
         case CaptureFlag:
            return 1;
         default:
            return naScore;
      }
   }
   else  // scoreGroup == IndividualScore
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 1;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -1;
         case KillTeammate:
            return 0;
         case KillEnemyTurret:
            return 1;
         case KillOwnTurret:
            return -1;
         case ReturnTeamFlag:
            return 1;
         case CaptureFlag:
            return 5;
         default:
            return naScore;
      }
   }
}


GameTypeId CTFGameType::getGameTypeId() const { return CTFGame; }


const char *CTFGameType::getShortName() const { return "CTF"; }

static const char *instructions[] = { "Take the enemy flag",  "and touch it to yours!" };
const char **CTFGameType::getInstructionString() const { return instructions; }
HelpItem CTFGameType::getGameStartInlineHelpItem() const { return CTFGameStartItem; }


bool CTFGameType::isFlagGame()          const { return true; }
bool CTFGameType::isTeamGame()          const { return true; }
bool CTFGameType::canBeTeamGame()       const { return true; }
bool CTFGameType::canBeIndividualGame() const { return false; }


};


