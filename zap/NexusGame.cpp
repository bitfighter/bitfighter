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

const U32 MAX_DROP_FLAGS = 200;

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
   //mNexusTimer.reset(mNexusClosedTime * 1000);
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
      mNexusChangeAtTime = mGameTimer.getTotalGameTime() / 1000 - mNexusClosedTime;

   return true;
}


string NexusGameType::toString() const
{
   return string(getClassName()) + " " + mGameTimer.toString_minutes() + " " + ftos(F32(mNexusClosedTime) / 60, 3) + " " + 
                                         ftos(F32(mNexusOpenTime), 3) + " " + itos(getWinningScore());
}


// Returns time left in current Nexus cycle -- if we're open, this will be the time until Nexus closes; if we're closed,
// it will return the time until Nexus opens
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


void NexusGameType::addNexus(NexusObject *nexus)
{
   mNexus.push_back(nexus);
}


bool NexusGameType::isCarryingItems(Ship *ship)
{
   if(ship->mMountedItems.size() > 1)     // Currently impossible, but in future may be possible
      return true;
   if(ship->mMountedItems.size() == 0)    // Should never happen
      return false;

   MoveItem *item = ship->mMountedItems[0];   // Currently, ship always has a NexusFlagItem... this is it
   if(!item)                                  // Null when a player drop flag and get destroyed at the same time
      return false;  

   return ( ((NexusFlagItem *) item)->getFlagCount() > 0 );    
}


// Cycle through mounted items and find the first one (last one, actually) that's a NexusFlagItem.
// Returns NULL if it can't find one.
static NexusFlagItem *findFirstNexusFlag(Ship *ship)
{
   for(S32 i = ship->mMountedItems.size() - 1; i >= 0; i--)
   {
      MoveItem *item = ship->mMountedItems[i];
      NexusFlagItem *flag = dynamic_cast<NexusFlagItem *>(item);

      if(flag)
         return flag;
   }

   return NULL;
}


// The flag will come from ship->mount.  *item is used as it is posssible to carry and drop multiple items
void NexusGameType::itemDropped(Ship *ship, MoveItem *item)
{
   NexusFlagItem *flag = dynamic_cast<NexusFlagItem *>(item);
   if(!flag)
      return;

   U32 flagCount = flag->getFlagCount();

   if(flagCount == 0)  // This is needed if you drop your flags, then pick up a different item type (like resource item), and drop it
      return;

   if(!ship->getClientInfo())
      return;

   Vector<StringTableEntry> e;

   e.push_back(ship->getClientInfo()->getName());
   if(flagCount > 1)
      e.push_back(itos(flagCount).c_str());


   static StringTableEntry dropOneString( "%e0 dropped a flag!");
   static StringTableEntry dropManyString( "%e0 dropped %e1 flags!");

   StringTableEntry *ste = (flagCount > 1) ? &dropManyString : &dropOneString;

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, *ste, e);
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
         items.insert(i, "Nexus Time to Open");
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


TNL_IMPLEMENT_NETOBJECT(NexusObject);


TNL_IMPLEMENT_NETOBJECT_RPC(NexusObject, s2cFlagsReturned, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   getGame()->getGameType()->mZoneGlowTimer.reset();
}


// The nexus is open.  A ship has entered it.  Now what?
// Runs on server only
void NexusGameType::shipTouchNexus(Ship *theShip, NexusObject *theNexus)
{
   NexusFlagItem *theFlag = findFirstNexusFlag(theShip);

   if(!theFlag)      // Just in case!
      return;

   updateScore(theShip, ReturnFlagsToNexus, theFlag->getFlagCount());

   S32 flagsReturned = theFlag->getFlagCount();
   ClientInfo *scorer = theShip->getClientInfo();

   if(flagsReturned > 0 && scorer)
   {
      s2cNexusMessage(NexusMsgScore, scorer->getName().getString(), theFlag->getFlagCount(), 
                      getEventScore(TeamScore, ReturnFlagsToNexus, theFlag->getFlagCount()) );
      theNexus->s2cFlagsReturned();    // Alert the Nexus that someone has returned flags to it

      // See if this event qualifies for an achievement
      if(flagsReturned >= 25 &&                                   // Return 25+ flags
         scorer && scorer->isAuthenticated() &&                   // Player must be authenticated
         getGame()->getPlayerCount() >= 4 &&                      // Game must have 4+ human players
         getGame()->getAuthenticatedPlayerCount() >= 2 &&         // Two of whom must be authenticated
         !hasFlagSpawns() && !hasPredeployedFlags() &&            // Level can have no flag spawns, nor any predeployed flags
         !scorer->hasBadge(BADGE_TWENTY_FIVE_FLAGS))              // Player doesn't already have the badge
      {
         MasterServerConnection *masterConn = getGame()->getConnectionToMaster();
         if(masterConn && masterConn->isEstablished())
         {
            masterConn->s2mAcheivementAchieved(BADGE_TWENTY_FIVE_FLAGS, scorer->getName());     // Notify the master
            s2cAchievementMessage(BADGE_TWENTY_FIVE_FLAGS, scorer->getName());                  // Alert other players
         } 
      }
   }

   theFlag->changeFlagCount(0);
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


// Runs on the server
// If a flag is released from a ship, it will have underlying startVel, to which a random vector will be added
void releaseFlag(Game *game, Point pos, Point startVel, S32 count = 0)
{
   F32 th = TNL::Random::readF() * Float2Pi;
   F32 f = (TNL::Random::readF() * 2 - 1) * 100;
   Point vel(cos(th) * f, sin(th) * f);
   vel += startVel;

   if(count > 1)
   {
      NexusFlagItem *newFlag = new NexusFlagItem(pos, vel, true);
      newFlag->changeFlagCount(count);
      newFlag->addToGame(game, game->getGameObjDatabase());
   }
   else
   {
      FlagItem *newFlag = new FlagItem(pos, vel, true);
      newFlag->addToGame(game, game->getGameObjDatabase());
   }
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
   return changeTime - duration;
}


void NexusGameType::idle_client(U32 deltaT)
{
#ifndef ZAP_DEDICATED
   //mNexusTimer.update(deltaT);


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
   if(!mNexusIsOpen && nexusShouldChange())         // Nexus has just opened
   {
      mNexusIsOpen = true;
      mNexusChangeAtTime = getNextChangeTime(mNexusChangeAtTime, mNexusOpenTime);
      // Check if anyone is already in the Nexus, examining each client's ship in turn...
      for(S32 i = 0; i < getGame()->getClientCount(); i++)
      {
         Ship *client_ship = getGame()->getClientInfo(i)->getShip();

         if(!client_ship)
            continue;

         NexusObject *nexus = dynamic_cast<NexusObject *>(client_ship->isInZone(NexusTypeNumber));

         if(nexus)
            shipTouchNexus(client_ship, nexus);
      }

      // Fire an event
      EventManager::get()->fireEvent(EventManager::NexusOpenedEvent);
   }
   else if(mNexusIsOpen && nexusShouldChange())       // Nexus has just closed
   {
      mNexusIsOpen = false;
      mNexusChangeAtTime = getNextChangeTime(mNexusChangeAtTime, mNexusClosedTime);

      // Fire an event
      EventManager::get()->fireEvent(EventManager::NexusClosedEvent);
   }

   // Advance all flagSpawn timers and see if it's time for a new flag
   for(S32 i = 0; i < getFlagSpawnCount(); i++)
   {
      FlagSpawn *flagSpawn = const_cast<FlagSpawn *>(getFlagSpawn(i));    // We need to get a modifiable pointer so we can update the timer

      if(flagSpawn->updateTimer(deltaT))
      {
         releaseFlag(getGame(), getFlagSpawn(i)->getPos(), Point(0,0));   // Release a flag
         flagSpawn->resetTimer();                                         // Reset the timer
      }
   }
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
bool NexusGameType::isTeamFlagGame()      const { return false; } // Ditto... team info will be ignored... no need to show warning in editor
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
      UserInterface::drawStringfr(x, y - size, size, "Nexus never closes");
   else if(!mNexusIsOpen && mNexusClosedTime == 0)
      UserInterface::drawStringfr(x, y - size, size, "Nexus never opens");
   else if(!mNexusIsOpen && mNexusChangeAtTime <= 0)
      UserInterface::drawStringfr(x, y - size, size, "Nexus closed until end of game");
   else if(!isGameOver())
   {
      static const U32 w00     = UserInterface::getStringWidth(size, "00:00");
      static const U32 wCloses = UserInterface::getStringWidth(size, "Nexus closes: ");
      static const U32 wOpens  = UserInterface::getStringWidth(size, "Nexus opens: ");

      S32 w = w00 + (mNexusIsOpen ? wCloses : wOpens);

      S32 timeLeft = min(getNexusTimeLeft() * 1000, (S32)mGameTimer.getCurrent());

      UserInterface::drawTime(x - w, y - size, size, timeLeft, mNexusIsOpen ? "Nexus closes: " : "Nexus opens: ");
   }

   for(S32 i = 0; i < mYardSaleWaypoints.size(); i++)
      renderObjectiveArrow(&mYardSaleWaypoints[i].pos, &Colors::white);

   for(S32 i = 0; i < mNexus.size(); i++)
      renderObjectiveArrow(dynamic_cast<BfObject *>(mNexus[i].getPointer()), mNexusIsOpen ? &gNexusOpenColor : &gNexusClosedColor);
}
#endif


//////////  END Client only code



void NexusGameType::controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *theShip = dynamic_cast<Ship *>(clientObject);
   if(!theShip)
      return;

   // Check for yard sale  (is this when the flags a player is carrying go drifting about??)
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      MoveItem *item = theShip->mMountedItems[i];
      NexusFlagItem *flag = dynamic_cast<NexusFlagItem *>(item);

      if(flag)
      {
         if(flag->getFlagCount() >= YardSaleCount)
         {
            Point pos = flag->getActualPos();
            s2cAddYardSaleWaypoint(pos.x, pos.y);
            s2cNexusMessage(NexusMsgYardSale, theShip->getClientInfo()->getName().getString(), 0, 0);
         }

         return;
      }
   }
}


void NexusGameType::shipTouchFlag(Ship *theShip, FlagItem *theOtherFlag)
{
   // Don't mount to ship, instead increase current mounted NexusFlag
   //    flagCount, and remove collided flag from game
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      NexusFlagItem *shipFlag = dynamic_cast<NexusFlagItem *>(theShip->mMountedItems[i].getPointer());
      if(shipFlag)
      {
         U32 flagCount = shipFlag->getFlagCount();
         NexusFlagItem *theOtherNexusFlag = dynamic_cast<NexusFlagItem *>(theOtherFlag);
         if(theOtherNexusFlag)
            flagCount += theOtherNexusFlag->getFlagCount();
         else
            flagCount += 1;
         shipFlag->changeFlagCount(flagCount);

         if(mNexusIsOpen)
         {
            // Check if ship is sitting on an open Nexus (can use static_cast because we already know the type, even though it could be NULL)
            NexusObject *nexus = static_cast<NexusObject *>(theShip->isInZone(NexusTypeNumber));

            if(nexus)         
               shipTouchNexus(theShip, nexus);
         }

         break;
      }
   }

   theOtherFlag->setCollideable(false);
   theOtherFlag->removeFromDatabase();
   theOtherFlag->deleteObject();
}


// Special spawn function for Nexus games (runs only on server)
bool NexusGameType::spawnShip(ClientInfo *clientInfo)
{
   if(!Parent::spawnShip(clientInfo))
      return false;

   Ship *ship = clientInfo->getShip();

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
NexusFlagItem::NexusFlagItem(Point pos, Point vel, bool useDropDelay) : FlagItem(pos, true, (F32)Ship::CollisionRadius, 4)  // radius was 30, which had problem with sticking to wall when drop too close to walls
{
   mFlagCount = 0;

   setActualVel(vel);
   if(useDropDelay)
      mDroppedTimer.reset(DROP_DELAY);
}


// Destructor
NexusFlagItem::~NexusFlagItem()
{
   // Do nothing
}


//////////  Client only code:

void NexusFlagItem::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   // Don't render flags on cloaked ships
   if(mMount.isValid() && mMount->isModulePrimaryActive(ModuleCloak))
      return;

   Parent::renderItem(pos);

   if(mIsMounted && mFlagCount > 0)
   {
      if     (mFlagCount >= 40) glColor(Colors::paleRed);   // like, rad!
      else if(mFlagCount >= 20) glColor(Colors::yellow);    // cool!
      else if(mFlagCount >= 10) glColor(Colors::green);     // ok, I guess
      else                      glColor(Colors::white);     // lame

      UserInterface::drawStringf(pos.x + 10, pos.y - 46, 12, "%d", mFlagCount);
   }
#endif
}

//////////  END Client only code



// Private helper function
void NexusFlagItem::dropFlags(U32 flags)
{
   if(!mMount.isValid())
      return;

   if(isGhost())  //avoid problem with adding flag to client, when it doesn't really exist on server.
      return;

   if(flags > MAX_DROP_FLAGS)
   {
      for(U32 i = MAX_DROP_FLAGS; i > 0; i--)
      {
         // By dividing and subtracting, it works by using integer divide, subtracting from "flags" left, and the last loop is (i == 1), dropping exact amount using only limited FlagItems
         U32 thisFlagDropped = flags / i;
         flags -= thisFlagDropped;
         releaseFlag(getGame(), mMount->getActualPos(), mMount->getActualVel(), thisFlagDropped);
      }
   }
   else
      for(U32 i = 0; i < flags; i++)
         releaseFlag(getGame(), mMount->getActualPos(), mMount->getActualVel());

   changeFlagCount(0);
}


void NexusFlagItem::onMountDestroyed()
{
   if(mMount && mMount->getClientInfo())
      mMount->getClientInfo()->getStatistics()->mFlagDrop += mFlagCount + 1;

   dropFlags(mFlagCount + 1);    // Drop at least one flag plus as many as the ship carries

   // Now delete the flag itself
   dismount();
   removeFromDatabase();
   deleteObject();
}


void NexusFlagItem::onItemDropped()
{
   if(!isGhost())
   {
      GameType *gameType = getGame()->getGameType();
      if(!gameType)                 // Crashed here once, don't know why, so I added the check
         return;

      gameType->itemDropped(mMount, this);
   }
   dropFlags(mFlagCount);          // Only dropping the flags we're carrying, not the "extra" one that comes when we die
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


// Constructor
NexusObject::NexusObject()
{
   mObjectTypeNumber = NexusTypeNumber;
   mNetFlags.set(Ghostable);

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
NexusObject::~NexusObject()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


NexusObject *NexusObject::clone() const
{
   return new NexusObject(*this);
}


// The nexus object itself
// If there are 2 or 4 params, this is an Zap! rectangular format object
// If there are more, this is a Bitfighter polygonal format object
// Note parallel code in EditorUserInterface::processLevelLoadLine
bool NexusObject::processArguments(S32 argc2, const char **argv2, Game *game)
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
   {
      Point pos;
      pos.read(argv);
      pos *= game->getGridSize();

      Point ext(50, 50);
      if(argc == 4)
         ext.set(atoi(argv[2]), atoi(argv[3]));

      addVert(Point(pos.x - ext.x, pos.y - ext.y));   // UL corner
      addVert(Point(pos.x + ext.x, pos.y - ext.y));   // UR corner
      addVert(Point(pos.x + ext.x, pos.y + ext.y));   // LR corner
      addVert(Point(pos.x - ext.x, pos.y + ext.y));   // LL corner

      updateExtentInDatabase();  
   }
   else              // Sleek, modern Bitfighter format
      Parent::processArguments(argc, argv, game);

   return true;
}


const char *NexusObject::getOnScreenName()     { return "Nexus"; }
const char *NexusObject::getOnDockName()       { return "Nexus"; }
const char *NexusObject::getPrettyNamePlural() { return "Nexii"; }
const char *NexusObject::getEditorHelpString() { return "Area to bring flags in Hunter game.  Cannot be used in other games."; }


bool NexusObject::hasTeam()      { return false; }
bool NexusObject::canBeHostile() { return false; }
bool NexusObject::canBeNeutral() { return false; }


string NexusObject::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + geomToString(gridSize);
}


void NexusObject::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always visible!

   NexusGameType *gt = dynamic_cast<NexusGameType *>( getGame()->getGameType() );
   if(gt) 
      gt->addNexus(this);
}


void NexusObject::idle(BfObject::IdleCallPath path)
{
   // Do nothing
}


void NexusObject::render()
{
#ifndef ZAP_DEDICATED
   GameType *gt = getGame()->getGameType();
   NexusGameType *theGameType = dynamic_cast<NexusGameType *>(gt);
   renderNexus(getOutline(), getFill(), getCentroid(), getLabelAngle(), 
              (theGameType && theGameType->mNexusIsOpen), gt ? gt->mZoneGlowTimer.getFraction() : 0);
#endif
}


void NexusObject::renderDock()
{
#ifndef ZAP_DEDICATED
  renderNexus(getOutline(), getFill(), false, 0);
#endif
}


void NexusObject::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   render();
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


bool NexusObject::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints = *getOutline();
   return true;
}


bool NexusObject::collide(BfObject *hitObject)
{
   if(isGhost())
      return false;

   // From here on out, runs on server only

   if( ! (isShipType(hitObject->getObjectTypeNumber())) )
      return false;

   Ship *theShip = dynamic_cast<Ship *>(hitObject);
   if(!theShip)
      return false;

   if(theShip->hasExploded)                              // Ignore collisions with exploded ships
      return false;

   NexusGameType *theGameType = dynamic_cast<NexusGameType *>(getGame()->getGameType());
   if(theGameType && theGameType->mNexusIsOpen)          // Is the nexus open?
      theGameType->shipTouchNexus(theShip, this);

   return false;
}


U32 NexusObject::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   packGeom(connection, stream);

   return 0;
}


void NexusObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   unpackGeom(connection, stream);      
}


/////
// Lua interface

const luaL_reg           NexusObject::luaMethods[]   = { { NULL, NULL } };
const LuaFunctionProfile NexusObject::functionArgs[] = { { NULL, { }, 0 } };


const char *NexusObject::luaClassName = "NexusObject";
REGISTER_LUA_SUBCLASS(NexusObject, Zone);

};
