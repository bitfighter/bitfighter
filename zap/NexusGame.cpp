//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "NexusGame.h"
#include "EventManager.h"      

#include "stringUtils.h"      // For ftos et al
#include "masterConnection.h" // For master connection details

#include "ClientInfo.h"

#ifndef ZAP_DEDICATED
#   include "gameObjectRender.h"
#   include "ScreenInfo.h"
#   include "ClientGame.h"
#   include "UIGame.h"
#   include "UIMenuItems.h"
#   include "OpenglUtils.h"
#endif


#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(NexusGameType);


TNL_IMPLEMENT_NETOBJECT_RPC(NexusGameType, s2cSetNexusTimer, (S32 nextChangeTime, bool isOpen), (nextChangeTime, isOpen), 
                            NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mNexusChangeAtTime = nextChangeTime;
   mNexusIsOpen = isOpen;
}


TNL_IMPLEMENT_NETOBJECT_RPC(NexusGameType, s2cSendNexusTimes, (S32 nexusClosedTime, S32 nexusOpenTime), (nexusClosedTime, nexusOpenTime),                                            NetClassGroupGameMask, RPCGuaranteed, RPCToGhost, 0)
{
   mNexusClosedTime = nexusClosedTime;
   mNexusOpenTime = nexusOpenTime;
}


GAMETYPE_RPC_S2C(NexusGameType, s2cAddYardSaleWaypoint, (F32 x, F32 y), (x, y))
{
   YardSaleWaypoint w;
   w.timeLeft.reset(YardSaleWaypointTime);
   w.pos.set(x,y);
   mYardSaleWaypoints.push_back(w);
}


TNL_IMPLEMENT_NETOBJECT_RPC(NexusGameType, s2cNexusMessage,
   (U32 msgIndex, StringTableEntry clientName, U32 flagCount, U32 score), (msgIndex, clientName, flagCount, score),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED

   ClientGame *clientGame = static_cast<ClientGame *>(getGame());

   if(msgIndex == NexusMsgScore)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), "%s returned %d flag%s to the Nexus for %d points!", 
                                 clientName.getString(), flagCount, flagCount > 1 ? "s" : "", score);
      SoundSystem::playSoundEffect(SFXFlagCapture);

      Ship *ship = clientGame->findShip(clientName);
      if(ship && score >= 100)
         clientGame->emitTextEffect(itos(score) + " POINTS!", Colors::red80, ship->getRenderPos());
   }
   else if(msgIndex == NexusMsgYardSale)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), "%s is having a YARD SALE!", clientName.getString());
      SoundSystem::playSoundEffect(SFXFlagSnatch);

      Ship *ship = clientGame->findShip(clientName);
      if(ship)
         clientGame->emitTextEffect("YARD SALE!", Colors::red80, ship->getRenderPos());
   }
   else if(msgIndex == NexusMsgGameOverWin)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), "Player %s wins the game!", clientName.getString());
      SoundSystem::playSoundEffect(SFXFlagCapture);
   }
   else if(msgIndex == NexusMsgGameOverTie)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), "The game ended in a tie.");
      SoundSystem::playSoundEffect(SFXFlagDrop);
   }
#endif
}


// Constructor
NexusGameType::NexusGameType() : GameType(100)
{
   mNexusClosedTime = 60;
   mNexusOpenTime = 15;
   mNexusIsOpen = false;
   mNexusChangeAtTime = -1;
}


bool NexusGameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc > 0)
   {
      setGameTime(F32(atof(argv[0]) * 60.0));                 // Game time, stored in minutes in level file

      if(argc > 1)
      {
         mNexusClosedTime = S32(atof(argv[1]) * 60.f + 0.5);  // Time until nexus opens, specified in minutes (0.5 converts truncation into rounding)

         if(argc > 2)
         {
            mNexusOpenTime = S32(atof(argv[2]));              // Time nexus remains open, specified in seconds

            if(argc > 3)
               setWinningScore(atoi(argv[3]));                // Winning score
         }
      }
   }

   if(mGameTimer.isUnlimited())
      mNexusChangeAtTime = S32_MAX / 1000 - mNexusClosedTime;
   else
      mNexusChangeAtTime = mGameTimer.getTotalGameTime() / 1000 - mNexusClosedTime + 1;      // + 1 fixes quirk when setting times initially
   return true;
}


string NexusGameType::toLevelCode() const
{
   return string(getClassName()) + " " + mGameTimer.toString_minutes() + " " + ftos(F32(mNexusClosedTime) / 60, 3) + " " + 
                                         ftos(F32(mNexusOpenTime), 3)  + " " + itos(getWinningScore());
}


// Returns time left in current Nexus cycle -- if we're open, this will be the time until Nexus closes; if we're closed,
// it will return the time until Nexus opens
// Client only
S32 NexusGameType::getNexusTimeLeft()
{
   return mGameTimer.getCurrent() / 1000 - mNexusChangeAtTime;
}


// Here we need to update the game clock as well as change the time we expect the Nexus will next change state
// This version runs only on the client 
void NexusGameType::setTimeRemaining(U32 timeLeft, bool isUnlimited, S32 renderingOffset)
{
   U32 oldDisplayTime = mGameTimer.getCurrent() / 1000;           // Time displayed before remaining time changed

   Parent::setTimeRemaining(timeLeft, isUnlimited, renderingOffset);

   U32 newDisplayTime = mGameTimer.getCurrent() / 1000;           // Time displayed after remaining time changed

   if(mNexusChangeAtTime == -1)     // Initial visit to this function, will happen on client when they first join a level
      mNexusChangeAtTime = newDisplayTime - mNexusClosedTime;
   else
      mNexusChangeAtTime = newDisplayTime - (oldDisplayTime - mNexusChangeAtTime);
}


// Game time has changed -- need to do an update
// This version runs only on the server
void NexusGameType::setTimeRemaining(U32 timeLeft, bool isUnlimited)
{
   U32 oldDisplayTime = mGameTimer.getCurrent() / 1000;           // Time displayed before remaining time changed

   Parent::setTimeRemaining(timeLeft, isUnlimited);

   U32 newDisplayTime = mGameTimer.getCurrent() / 1000;           // Time displayed after remaining time changed

   if(mNexusChangeAtTime == -1)     // Initial visit to this function, will happen on client when they first join a level
      mNexusChangeAtTime = newDisplayTime - mNexusClosedTime;
   else
      mNexusChangeAtTime = newDisplayTime - (oldDisplayTime - mNexusChangeAtTime);
}


bool NexusGameType::nexusShouldChange()
{
   if(mNexusChangeAtTime == -1)     
      return false;

   return mNexusChangeAtTime * 1000 > (S32)mGameTimer.getCurrent();
}


bool NexusGameType::isSpawnWithLoadoutGame()
{
   return true;
}


void NexusGameType::addNexus(NexusZone *nexus)
{
   mNexus.push_back(nexus);
}

// Count flags on a ship.  This function assumes that all carried flags are NexusFlags, each of which can represent multiple flags
// (see getFlagCount()).  This code will support a ship having several flags, but in practice, each ship will have exactly one.
static S32 getMountedFlagCount(Ship *ship)
{
   S32 flagCount = 0;
   S32 itemCount = ship->getMountedItemCount();

   for(S32 i = 0; i < itemCount; i++)
   {
      MountableItem *mountedItem = ship->getMountedItem(i);

      if(mountedItem->getObjectTypeNumber() == FlagTypeNumber)      // All flags are NexusFlags here!
      {
         NexusFlagItem *flag = static_cast<NexusFlagItem *>(mountedItem);
         flagCount += flag->getFlagCount();
      }
   }

   return flagCount;
}


// Currently only used when determining if there is something to drop
bool NexusGameType::isCarryingItems(Ship *ship)
{
   S32 itemCount = ship->getMountedItemCount();

   for(S32 i = 0; i < itemCount; i++)
   {
      MountableItem *mountedItem = ship->getMountedItem(i);
      if(!mountedItem)        // Could be null when a player drop their flags and gets destroyed at the same time
         continue;

      if(mountedItem->getObjectTypeNumber() == FlagTypeNumber)      
      {
         FlagItem *flag = static_cast<FlagItem *>(mountedItem);
         if(flag->getFlagCount() > 0)
            return true;
      }
      else     // Must be carrying something other than a flag.  Maybe we could drop that!
         return true;
   }

   return false;
}


// Cycle through mounted items and find the first one that's a FlagItem.
// In practice, this will always be a NexusFlagItem... I think...
// Returns NULL if it can't find a flag.
static FlagItem *findFirstFlag(Ship *ship)
{
   return static_cast<FlagItem *>(ship->getMountedItem(ship->getFlagIndex()));
}


// The flag will come from ship->mount.  *item is used as it is posssible to carry and drop multiple items.
// This method doesn't actually do any dropping; it only sends out an appropriate flag-drop message.
void NexusGameType::itemDropped(Ship *ship, MoveItem *item, MountableItem::DismountMode dismountMode)
{
   Parent::itemDropped(ship, item, dismountMode);

   if(item->getObjectTypeNumber() == FlagTypeNumber)
   {
      if(dismountMode != MountableItem::DISMOUNT_SILENT)
      {
         FlagItem *flag = static_cast<FlagItem *>(item);

         U32 flagCount = flag->getFlagCount();

         if(flagCount == 0)  // Needed if you drop your flags, then pick up a different item type (like resource item), and drop it
            return;

         if(!ship->getClientInfo())
            return;

         Vector<StringTableEntry> e;
         e.push_back(ship->getClientInfo()->getName());

         static StringTableEntry dropOneString(  "%e0 dropped a flag!");
         static StringTableEntry dropManyString( "%e0 dropped %e1 flags!");

         StringTableEntry *ste;

         if(flagCount == 1)
            ste = &dropOneString;
         else
         {
            ste = &dropManyString;
            e.push_back(itos(flagCount).c_str());
         }
      
         broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, *ste, e);
      }
   }
}


#ifndef ZAP_DEDICATED
// Any unique items defined here must be handled in both getMenuItem() and saveMenuItem() below!
Vector<string> NexusGameType::getGameParameterMenuKeys()
{
   Vector<string> items = Parent::getGameParameterMenuKeys();
   
   // Remove Win Score, replace it with some Nexus specific items
   for(S32 i = 0; i < items.size(); i++)
      if(items[i] == "Win Score")
      {
         items.erase(i);      // Delete "Win Score"

         // Create slots for 3 new items, and fill them with our Nexus specific items
         items.insert(i,     "Nexus Time to Open");
         items.insert(i + 1, "Nexus Time Remain Open");
         items.insert(i + 2, "Nexus Win Score");

         break;
      }

   return items;
}


// Definitions for those items
boost::shared_ptr<MenuItem> NexusGameType::getMenuItem(const string &key)
{
   if(key == "Nexus Time to Open")
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItem("Time for Nexus to Open:", mNexusClosedTime, 99*60, "Never", 
                                                                 "Time it takes for the Nexus to open"));
   else if(key == "Nexus Time Remain Open")
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItemSeconds("Time Nexus Remains Open:", mNexusOpenTime, 99*60, "Always", 
                                                                        "Time that the Nexus will remain open"));
   else if(key == "Nexus Win Score")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Score to Win:", getWinningScore(), 100, 100, 20000, "points", "", 
                                                             "Game ends when one player or team gets this score"));
   else return Parent::getMenuItem(key);
}


bool NexusGameType::saveMenuItem(const MenuItem *menuItem, const string &key)
{
   if(key == "Nexus Time to Open")
      mNexusClosedTime = menuItem->getIntValue();
   else if(key == "Nexus Time Remain Open")
      mNexusOpenTime = menuItem->getIntValue();
   else if(key == "Nexus Win Score")
      setWinningScore(menuItem->getIntValue());
   else 
      return Parent::saveMenuItem(menuItem, key);

   return true;
}
#endif


TNL_IMPLEMENT_NETOBJECT(NexusZone);


TNL_IMPLEMENT_NETOBJECT_RPC(NexusZone, s2cFlagsReturned, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   getGame()->getGameType()->mZoneGlowTimer.reset();
}


// The nexus is open.  A ship has entered it.  Now what?
// Runs on server only
void NexusGameType::shipTouchNexus(Ship *ship, NexusZone *theNexus)
{
   FlagItem *flag = findFirstFlag(ship);

   if(!flag)      // findFirstFlag can return NULL
      return;

   updateScore(ship, ReturnFlagsToNexus, flag->getFlagCount());

   S32 flagsReturned = flag->getFlagCount();
   ClientInfo *scorer = ship->getClientInfo();

   if(flagsReturned > 0 && scorer)
   {
      if(!isGameOver())  // Avoid flooding messages on game over.
         s2cNexusMessage(NexusMsgScore, scorer->getName().getString(), flag->getFlagCount(), 
                      getEventScore(TeamScore, ReturnFlagsToNexus, flag->getFlagCount()) );
      theNexus->s2cFlagsReturned();    // Alert the Nexus that someone has returned flags to it

      // See if this event qualifies for an achievement
      if(flagsReturned >= 25 &&                                   // Return 25+ flags
         scorer && scorer->isAuthenticated() &&                   // Player must be authenticated
         getGame()->getPlayerCount() >= 4 &&                      // Game must have 4+ human players
         getGame()->getAuthenticatedPlayerCount() >= 2 &&         // Two of whom must be authenticated
         !hasFlagSpawns() && !hasPredeployedFlags() &&            // Level can have no flag spawns, nor any predeployed flags
         !scorer->hasBadge(BADGE_TWENTY_FIVE_FLAGS))              // Player doesn't already have the badge
      {
         achievementAchieved(BADGE_TWENTY_FIVE_FLAGS, scorer->getName());
      }
   }

   flag->changeFlagCount(0);
}


// Runs on the server
void NexusGameType::onGhostAvailable(GhostConnection *theConnection)
{
   Parent::onGhostAvailable(theConnection);

   NetObject::setRPCDestConnection(theConnection);

   s2cSendNexusTimes(mNexusClosedTime, mNexusOpenTime);     // Send info about Nexus hours of business
   s2cSetNexusTimer(mNexusChangeAtTime, mNexusIsOpen);      // Send info about current state of Nexus
   
   NetObject::setRPCDestConnection(NULL);
}


// Emit a flag in a random direction at a random speed
// Server only, static method.  Only called from dropFlags().
// If a flag is released from a ship, it will have underlying startVel, to which a random vector will be added
void NexusGameType::releaseFlag(Game *game, const Point &pos, const Point &startVel, S32 count)
{
   static const S32 MAX_SPEED = 100;

   F32 th = TNL::Random::readF() * FloatTau;
   F32 f = (TNL::Random::readF() * 2 - 1) * MAX_SPEED;

   Point vel(cos(th) * f, sin(th) * f);
   vel += startVel;

   NexusFlagItem *newFlag = new NexusFlagItem(pos, vel, count, true);
   newFlag->addToGame(game, game->getGameObjDatabase());
}


// Runs on client and server
void NexusGameType::idle(BfObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);

   if(isGhost()) 
      idle_client(deltaT);
   else
      idle_server(deltaT);
}


static U32 getNextChangeTime(U32 changeTime, S32 duration)
{
   if(duration == 0)    // Handle special case of never opening/closing nexus
      return -1;

   return changeTime - duration;
}


void NexusGameType::idle_client(U32 deltaT)
{
#ifndef ZAP_DEDICATED
   if(!mNexusIsOpen && nexusShouldChange())         // Nexus has just opened
   {
      if(!isGameOver())
      {
         static_cast<ClientGame *>(getGame())->displayMessage(Color(0.6f, 1, 0.8f), "The Nexus is now OPEN!");
         SoundSystem::playSoundEffect(SFXFlagSnatch);
      }

      mNexusIsOpen = true;
      mNexusChangeAtTime = getNextChangeTime(mNexusChangeAtTime, mNexusOpenTime);
   }

   else if(mNexusIsOpen && nexusShouldChange())       // Nexus has just closed
   {
      if(!isGameOver())
      {
         static_cast<ClientGame *>(getGame())->displayMessage(Color(0.6f, 1, 0.8f), "The Nexus is now CLOSED!");
         SoundSystem::playSoundEffect(SFXFlagDrop);
      }

      mNexusIsOpen = false;
      mNexusChangeAtTime = getNextChangeTime(mNexusChangeAtTime, mNexusClosedTime);
   }

   for(S32 i = 0; i < mYardSaleWaypoints.size();)
   {
      if(mYardSaleWaypoints[i].timeLeft.update(deltaT))
         mYardSaleWaypoints.erase_fast(i);
      else
         i++;
   }
#endif
}


void NexusGameType::idle_server(U32 deltaT)
{
   if(nexusShouldChange())
   {
      if(mNexusIsOpen) 
         closeNexus(mNexusChangeAtTime);
      else
         openNexus(mNexusChangeAtTime);
   }
}


// Server only
void NexusGameType::openNexus(S32 timeNexusOpened)
{
   mNexusIsOpen = true;
   mNexusChangeAtTime = getNextChangeTime(timeNexusOpened, mNexusOpenTime);

   // Check if anyone is already in the Nexus, examining each client's ship in turn...
   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      Ship *client_ship = getGame()->getClientInfo(i)->getShip();

      if(!client_ship)
         continue;

      BfObject *zone = client_ship->isInZone(NexusTypeNumber);

      if(zone)
         shipTouchNexus(client_ship, static_cast<NexusZone *>(zone));
   }

   // Fire an event
   EventManager::get()->fireEvent(EventManager::NexusOpenedEvent);
}


// Server only
void NexusGameType::closeNexus(S32 timeNexusClosed)
{
   mNexusIsOpen = false;
   mNexusChangeAtTime = getNextChangeTime(timeNexusClosed, mNexusClosedTime);

   // Fire an event
   EventManager::get()->fireEvent(EventManager::NexusClosedEvent);
}


// Server only -- only called by scripts
void NexusGameType::setNexusState(bool open)
{
   if(open)
      openNexus(getRemainingGameTime());
   else
      closeNexus(getRemainingGameTime());

   s2cSetNexusTimer(mNexusChangeAtTime, open);      // Broacast new Nexus opening hours
}


// Server only -- only called by scripts
void NexusGameType::setNewOpenTime(S32 timeInSeconds)
{
   mNexusOpenTime = timeInSeconds;
   s2cSendNexusTimes(mNexusClosedTime, mNexusOpenTime);

   // Trigger update of new opening time if we are currently open
   if(mNexusIsOpen)
      setNexusState(true);
}


// Server only -- only called by scripts
void NexusGameType::setNewClosedTime(S32 timeInSeconds)
{
   mNexusClosedTime = timeInSeconds;
   s2cSendNexusTimes(mNexusClosedTime, mNexusOpenTime);

   // Trigger update of new closing time if we are currently closed
   if(!mNexusIsOpen)
      setNexusState(false);
}


// What does a particular scoring event score?
S32 NexusGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 flags)
{
   S32 score = 0;
   for(S32 count = 1; count <= flags; count++)
      score += (count * 10);

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
         case ReturnFlagsToNexus:
            return score;
         default:
            return naScore;
      }
   }
   else  // scoreGroup == IndividualScore
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
         case ReturnFlagsToNexus:
            return score;
         default:
            return naScore;
      }
   }
}


GameTypeId NexusGameType::getGameTypeId() const { return NexusGame; }

const char *NexusGameType::getShortName()         const { return "N"; }
const char *NexusGameType::getInstructionString() const { return "Collect flags from opposing players and bring them to the Nexus!"; }

bool NexusGameType::isFlagGame()          const { return true;  } // Well, technically not, but we'll pervert flags as we load the level
bool NexusGameType::canBeTeamGame()       const { return true;  }
bool NexusGameType::canBeIndividualGame() const { return true;  }


U32 NexusGameType::getLowerRightCornerScoreboardOffsetFromBottom() const
{
   return 88;
}


//////////  Client only code:

extern Color gNexusOpenColor;
extern Color gNexusClosedColor;

#ifndef ZAP_DEDICATED
void NexusGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);

   const S32 x = gScreenInfo.getGameCanvasWidth()  - UserInterface::horizMargin;
   const S32 y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 25;
   const S32 size = 20;

   glColor(mNexusIsOpen ? gNexusOpenColor : gNexusClosedColor);      // Display timer in appropriate color

   if(mNexusIsOpen && mNexusOpenTime == 0)
      drawStringfr(x, y - size, size, "Nexus never closes");
   else if(!mNexusIsOpen && mNexusClosedTime == 0)
      drawStringfr(x, y - size, size, "Nexus never opens");
   else if(!mNexusIsOpen && mNexusChangeAtTime <= 0)
      drawStringfr(x, y - size, size, "Nexus closed until end of game");
   else if(!isGameOver())
   {
      static const U32 w00     = getStringWidth(size, "00:00");
      static const U32 wCloses = getStringWidth(size, "Nexus closes: ");
      static const U32 wOpens  = getStringWidth(size, "Nexus opens: ");

      S32 w = w00 + (mNexusIsOpen ? wCloses : wOpens);

      S32 timeLeft = min(getNexusTimeLeft() * 1000, (S32)mGameTimer.getCurrent());

      drawTime(x - w, y - size, size, timeLeft, mNexusIsOpen ? "Nexus closes: " : "Nexus opens: ");
   }

   for(S32 i = 0; i < mYardSaleWaypoints.size(); i++)
      renderObjectiveArrow(mYardSaleWaypoints[i].pos, &Colors::white);

   for(S32 i = 0; i < mNexus.size(); i++)
      renderObjectiveArrow(mNexus[i].getPointer(), mNexusIsOpen ? &gNexusOpenColor : &gNexusClosedColor);
}
#endif


//////////  END Client only code

// Server only
void NexusGameType::controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject)
{
   if(isGameOver())  // Avoid flooding messages when game is over
      return;

   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   if(!clientObject || !isShipType(clientObject->getObjectTypeNumber()))
      return;

   Ship *ship = static_cast<Ship *>(clientObject);

   // Check for yard sale  (i.e. tons of flags released at same time)
   S32 flagCount = getMountedFlagCount(ship);

   static const S32 YARD_SALE_THRESHOLD = 8;

   if(flagCount >= YARD_SALE_THRESHOLD)
   {
      Point pos = ship->getActualPos();

      // Notify the clients
      s2cAddYardSaleWaypoint(pos.x, pos.y);
      s2cNexusMessage(NexusMsgYardSale, ship->getClientInfo()->getName().getString(), 0, 0);
   }
}


void NexusGameType::shipTouchFlag(Ship *ship, FlagItem *touchedFlag)
{
   // Don't mount to ship, instead increase current mounted NexusFlag
   //    flagCount, and remove collided flag from game

   FlagItem *shipFlag = findFirstFlag(ship);

   TNLAssert(shipFlag, "Expected to find a flag on this ship!");

   if(!shipFlag)      // findFirstFlag can return NULL... but probably won't
      return;

   U32 shipFlagCount = shipFlag->getFlagCount();

   if(touchedFlag)
      shipFlagCount += touchedFlag->getFlagCount();
   else
      shipFlagCount++;

   shipFlag->changeFlagCount(shipFlagCount);


   // Now that the touchedFlag has been absorbed into the ship, remove it from the game.  Be sure to use deleteObject, as having the database
   // delete the object directly leads to memory corruption errors.
   touchedFlag->removeFromDatabase(false);
   touchedFlag->setCollideable(false);
   touchedFlag->deleteObject();

   if(mNexusIsOpen)
   {
      // Check if ship is sitting on an open Nexus (can use static_cast because we already know the type, even though it could be NULL)
      NexusZone *nexus = static_cast<NexusZone *>(ship->isInZone(NexusTypeNumber));

      if(nexus)         
         shipTouchNexus(ship, nexus);
   }   
}


// Special spawn function for Nexus games (runs only on server)
bool NexusGameType::spawnShip(ClientInfo *clientInfo)
{
   if(!Parent::spawnShip(clientInfo))
      return false;

   Ship *ship = clientInfo->getShip();

   TNLAssert(ship, "Why NULL ship??");

   NexusFlagItem *newFlag = new NexusFlagItem(ship->getActualPos());
   newFlag->addToGame(getGame(), getGame()->getGameObjDatabase());
   newFlag->mountToShip(ship);    // mountToShip() can handle NULL
   newFlag->changeFlagCount(0);

   return true;
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(NexusFlagItem);

// C++ constructor
NexusFlagItem::NexusFlagItem(Point pos, Point vel, S32 count, bool useDropDelay) : Parent(pos, vel, useDropDelay)
{
   mFlagCount = count;
}


// Destructor
NexusFlagItem::~NexusFlagItem()
{
   // Do nothing
}


//////////  Client only code:

void NexusFlagItem::renderItem(const Point &pos)
{
   renderItemAlpha(pos, 1.0f);
}


void NexusFlagItem::renderItemAlpha(const Point &pos, F32 alpha)
{
#ifndef ZAP_DEDICATED
   Point offset;

   if(mIsMounted)
      offset.set(15, -15);

   renderFlag(pos + offset, getColor(), NULL, alpha);

   if(mIsMounted && mFlagCount > 0)
   {
      if     (mFlagCount >= 40) glColor(Colors::paleRed, alpha);   // like, rad!
      else if(mFlagCount >= 20) glColor(Colors::yellow,  alpha);   // cool!
      else if(mFlagCount >= 10) glColor(Colors::green,   alpha);   // ok, I guess
      else                      glColor(Colors::white,   alpha);   // lame

      drawStringf(pos.x + 10, pos.y - 46, 12, "%d", mFlagCount);
   }
#endif
}

//////////  END Client only code



// Private helper function
void NexusFlagItem::dropFlags(U32 flags)
{
   if(!mMount.isValid())
      return;

   // This is server only, folks -- avoids problem with adding flag on client when it doesn't really exist on server
   if(isGhost())
      return;

   static const U32 MAX_DROP_FLAGS = 200;    // If we drop too many flags, things just get bogged down.  This limit is rarely hit.

   if(flags > MAX_DROP_FLAGS)
   {
      for(U32 i = MAX_DROP_FLAGS; i > 0; i--)
      {
         // By dividing and subtracting, it works by using integer divide, subtracting from "flags" left, 
         // and the last loop is (i == 1), dropping exact amount using only limited FlagItems
         U32 flagValue = flags / i;

         NexusGameType::releaseFlag(getGame(), mMount->getActualPos(), mMount->getActualVel(), flagValue);

         flags -= flagValue;
      }
   }
   else     // Normal situation
      for(U32 i = 0; i < flags; i++)
         NexusGameType::releaseFlag(getGame(), mMount->getActualPos(), mMount->getActualVel());

   changeFlagCount(0);
}


void NexusFlagItem::dismount(DismountMode dismountMode)
{
   if(isGhost())      // Server only
      return;
  
   if(dismountMode == DISMOUNT_MOUNT_WAS_KILLED)
   {
      // Should getting shot up count as a flag drop event for statistics purposes?
      if(mMount && mMount->getClientInfo())
         mMount->getClientInfo()->getStatistics()->mFlagDrop += mFlagCount + 1;

      dropFlags(mFlagCount + 1);    // Drop at least one flag plus as many as the ship carries

      // Now delete the flag itself
      removeFromDatabase(false);   
      deleteObject();
   }
   else
   {
      GameType *gameType = getGame()->getGameType();
      if(!gameType)        // Crashed here once, don't know why, so I added the check
         return;

      gameType->itemDropped(mMount, this, dismountMode); // Sends messages; no flags actually dropped here; server only method
      dropFlags(mFlagCount);                             // Only dropping the flags we're carrying, not the "extra" one that comes when we die
   }
}


U32 NexusFlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & FlagCountMask))
      stream->write(mFlagCount);

   return Parent::packUpdate(connection, updateMask, stream);
}


void NexusFlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      stream->read(&mFlagCount);

   Parent::unpackUpdate(connection, stream);
}


bool NexusFlagItem::isItemThatMakesYouVisibleWhileCloaked()
{
   return false;
}


void NexusFlagItem::changeFlagCount(U32 change)
{
   mFlagCount = change;
   setMaskBits(FlagCountMask);
}


U32 NexusFlagItem::getFlagCount()
{
   return mFlagCount;
}


bool NexusFlagItem::isAtHome()
{
   return false;
}


void NexusFlagItem::sendHome()
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

/**
 *  @luaconst NexusZone::NexusZone()
 *  @luaconst NexusZone::NexusZone(geom)
 */
// Combined Lua / C++ constructor)
NexusZone::NexusZone(lua_State *L)
{
   mObjectTypeNumber = NexusTypeNumber;
   mNetFlags.set(Ghostable);
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "NexusZone", "constructor");
         
      if(profile == 1)
         setGeom(L, 1);
   }

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
NexusZone::~NexusZone()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


NexusZone *NexusZone::clone() const
{
   return new NexusZone(*this);
}


// The Nexus object itself
// If there are 2 or 4 params, this is an Zap! rectangular format object
// If there are more, this is a Bitfighter polygonal format object
// Note parallel code in EditorUserInterface::processLevelLoadLine
bool NexusZone::processArguments(S32 argc2, const char **argv2, Game *game)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[64]; // 32 * 2 = 64
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 65)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 2)
      return false;

   if(argc <= 4)     // Archaic Zap! format
      processArguments_ArchaicZapFormat(argc, argv, game->getGridSize());
   else              // Sleek, modern Bitfighter format
      Parent::processArguments(argc, argv, game);

   return true;
}


// Read and process NexusZone format used in Zap -- we need this for backwards compatibility
void NexusZone::processArguments_ArchaicZapFormat(S32 argc, const char **argv, F32 gridSize)
{
   Point pos;
   pos.read(argv);
   pos *= gridSize;

   Point ext(50, 50);

   if(argc == 4)
      ext.set(atoi(argv[2]), atoi(argv[3]));

   addVert(Point(pos.x - ext.x, pos.y - ext.y));   // UL corner
   addVert(Point(pos.x + ext.x, pos.y - ext.y));   // UR corner
   addVert(Point(pos.x + ext.x, pos.y + ext.y));   // LR corner
   addVert(Point(pos.x - ext.x, pos.y + ext.y));   // LL corner
   
   updateExtentInDatabase(); 
}


const char *NexusZone::getOnScreenName()     { return "Nexus"; }
const char *NexusZone::getOnDockName()       { return "Nexus"; }
const char *NexusZone::getPrettyNamePlural() { return "Nexii"; }
const char *NexusZone::getEditorHelpString() { return "Area to bring flags in Hunter game.  Cannot be used in other games."; }


bool NexusZone::hasTeam()      { return false; }
bool NexusZone::canBeHostile() { return false; }
bool NexusZone::canBeNeutral() { return false; }


string NexusZone::toLevelCode(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + geomToLevelCode(gridSize);
}


void NexusZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always visible!

   GameType *gameType = getGame()->getGameType();

   if(gameType && gameType->getGameTypeId() == NexusGame)
      static_cast<NexusGameType *>(gameType)->addNexus(this);
}


void NexusZone::idle(BfObject::IdleCallPath path)
{
   // Do nothing
}


void NexusZone::render()
{
#ifndef ZAP_DEDICATED
   GameType *gameType = getGame()->getGameType();
   NexusGameType *nexusGameType = NULL;

   if(gameType && gameType->getGameTypeId() == NexusGame)
      nexusGameType = static_cast<NexusGameType *>(gameType);

   bool isOpen = nexusGameType && nexusGameType->mNexusIsOpen;
   F32 glowFraction = gameType ? gameType->mZoneGlowTimer.getFraction() : 0;

   renderNexus(getOutline(), getFill(), getCentroid(), getLabelAngle(), isOpen, glowFraction);
#endif
}


void NexusZone::renderDock()
{
#ifndef ZAP_DEDICATED
  renderNexus(getOutline(), getFill(), false, 0);
#endif
}


void NexusZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


const Vector<Point> *NexusZone::getCollisionPoly() const
{
   return getOutline();
}


bool NexusZone::collide(BfObject *hitObject)
{
   if(isGhost())
      return false;

   // From here on out, runs on server only

   if( ! (isShipType(hitObject->getObjectTypeNumber())) )
      return false;

   Ship *theShip = static_cast<Ship *>(hitObject);

   if(theShip->hasExploded)                              // Ignore collisions with exploded ships
      return false;

   GameType *gameType = getGame()->getGameType();
   NexusGameType *nexusGameType = NULL;

   if(gameType && gameType->getGameTypeId() == NexusGame)
      nexusGameType = static_cast<NexusGameType *>(getGame()->getGameType());

   if(nexusGameType && nexusGameType->mNexusIsOpen)      // Is the nexus open?
      nexusGameType->shipTouchNexus(theShip, this);

   return false;
}


/////
// Lua interface
/**
  *  @luaclass NexusZone
  *  @brief Players return flags to a %NexusZone in a Nexus game.
  *  @descr %NexusZone represents a flag return area in a Nexus game.  It plays no role in any other game type.
  *         Nexus opening and closing times are actually game parameters, so these methods serve only as a convenient and
  *         intuitive way to access those parameters.  Therefore, modifying the opening/closing schedule or status of any
  *         %NexusZone will have the same effect on all NexusZones in the game.
  */
//               Fn name         Param profiles     Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, setOpen,       ARRAYDEF({{ BOOL,    END }}), 1 ) \
   METHOD(CLASS, isOpen,        ARRAYDEF({{          END }}), 1 ) \
   METHOD(CLASS, setOpenTime,   ARRAYDEF({{ INT_GE0, END }}), 1 ) \
   METHOD(CLASS, setClosedTime, ARRAYDEF({{ INT_GE0, END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(NexusZone, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(NexusZone, LUA_METHODS);

#undef LUA_METHODS


const char *NexusZone::luaClassName = "NexusZone";
REGISTER_LUA_SUBCLASS(NexusZone, Zone);


// All these methods are really just passthroughs to the underlying game object.  They will have no effect if the
// NexusZone has not been added to the game.  Perhaps this is bad design...

/**
 * @luafunc  bool NexusZone::isOpen()
 * @brief    Get the current state of the Nexus (open or closed).
 * @return   isOpen - Boolean specifying the current state of the Nexus -- true for open, false for closed.
 * @note     Since all Nexus items open and close together, this method will affect all Nexus items in a game.
*/
S32 NexusZone::isOpen(lua_State *L) 
{ 
   if(!mGame)
      return returnBool(L, false);

   GameType *gameType = mGame->getGameType();

   if(gameType->getGameTypeId() == NexusGame)
      return returnBool(L, static_cast<NexusGameType *>(gameType)->mNexusIsOpen);
   else
      return returnBool(L, false);     // If not a Nexus game, Nexus will never be open
}  


/**
 * @luafunc  NexusZone::setOpen(isOpen)
 * @brief    Tells the Nexus to open or close.
 * @param    isOpen - Pass true to open the Nexus, false to close it.
 * @note     Since all Nexus items open and close together, this method will affect all Nexus items in a game.
*/
S32 NexusZone::setOpen(lua_State *L) 
{ 
   checkArgList(L, functionArgs, "NexusZone", "setOpen");

   if(!mGame)
      return 0;

   GameType *gameType = mGame->getGameType();

   if(gameType->getGameTypeId() != NexusGame)       // Do nothing if this is not a Nexus game
      return 0;
  
   static_cast<NexusGameType *>(gameType)->setNexusState(getBool(L, 1));

   return 0;
}  


/**
 * @luafunc  NexusZone::setOpenTime(time)
 * @brief    Set the time (in seconds) that the Nexus should remain open. 
 * @descr    Pass 0 if the Nexus should never close, causing the Nexus to remain open permanently.  Passing a negative time will generate an error.
 * @param    time - Time in seconds that the Nexus should remain open.
 * @note     Since all Nexus items open and close together, this method will affect all Nexus items in a game.
 */
S32 NexusZone::setOpenTime(lua_State *L)
{
   checkArgList(L, functionArgs, "NexusZone", "setOpenTime");

   if(!mGame)
      return 0;

   GameType *gameType = mGame->getGameType();

   if(gameType->getGameTypeId() != NexusGame)       // Do nothing if this is not a Nexus game
      return 0;
  
   static_cast<NexusGameType *>(gameType)->setNewOpenTime(getInt(L, 1));

   return 0;
}


/**
 * @luafunc  NexusZone::setClosedTime(time)
 * @brief    Set the time (in seconds) that the Nexus will remain closed. 
 * @descr    Pass 0 if the Nexus should never open, causing the Nexus to remain closed permanently.   Passing a negative time will generate an error.
 * @param    time - Time in seconds that the Nexus should remain closed.
 * @note     Since all Nexus items open and close together, this method will affect all Nexus items in a game.
 * @note     Note that in a level file, closing times are specified in fractions of minutes.  This method works with seconds.
 */
S32 NexusZone::setClosedTime(lua_State *L)
{
   checkArgList(L, functionArgs, "NexusZone", "setCloseTime");

   if(!mGame)
      return 0;

   GameType *gameType = mGame->getGameType();

   if(gameType->getGameTypeId() != NexusGame)       // Do nothing if this is not a Nexus game
      return 0;
  
   static_cast<NexusGameType *>(gameType)->setNewClosedTime(getInt(L, 1));

   return 0;
}


};
