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

#include "huntersGame.h"
#include "flagItem.h"
#include "SoundSystem.h"
#include "gameNetInterface.h"
#include "ship.h"
#include "GeomUtils.h"        // For centroid calculation for labeling
#include "stringUtils.h"      // For itos
#include "game.h"
#include "gameConnection.h"

// Things I think should not be on server side
#include "Colors.h"
#include "ScreenInfo.h"
#include "gameObjectRender.h"


#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#include "UIGame.h"
#include "UIMenuItems.h"
#include "SDL/SDL_opengl.h"
#endif


#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(HuntersGameType);



TNL_IMPLEMENT_NETOBJECT_RPC(HuntersGameType, s2cSetNexusTimer, (U32 nexusTime, bool isOpen), (nexusTime, isOpen), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mNexusTimer.reset(nexusTime);
   mNexusIsOpen = isOpen;
}

GAMETYPE_RPC_S2C(HuntersGameType, s2cAddYardSaleWaypoint, (F32 x, F32 y), (x, y))
{
   YardSaleWaypoint w;
   w.timeLeft.reset(YardSaleWaypointTime);
   w.pos.set(x,y);
   mYardSaleWaypoints.push_back(w);
}

TNL_IMPLEMENT_NETOBJECT_RPC(HuntersGameType, s2cHuntersMessage,
   (U32 msgIndex, StringTableEntry clientName, U32 flagCount, U32 score), (msgIndex, clientName, flagCount, score),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED

   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   if(msgIndex == HuntersMsgScore)
   {
      SoundSystem::playSoundEffect(SFXFlagCapture);
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f),"%s returned %d flag%s to the Nexus for %d points!", clientName.getString(), flagCount, flagCount > 1 ? "s" : "", score);
   }
   else if(msgIndex == HuntersMsgYardSale)
   {
      SoundSystem::playSoundEffect(SFXFlagSnatch);
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f),
                  "%s is having a YARD SALE!",
                  clientName.getString());
   }
   else if(msgIndex == HuntersMsgGameOverWin)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f),
                     "Player %s wins the game!",
                     clientName.getString());
      SoundSystem::playSoundEffect(SFXFlagCapture);
   }
   else if(msgIndex == HuntersMsgGameOverTie)
   {
      clientGame->displayMessage(Color(0.6f, 1.0f, 0.8f), "The game ended in a tie.");
      SoundSystem::playSoundEffect(SFXFlagDrop);
   }
#endif
}


// Constructor
HuntersGameType::HuntersGameType() : GameType(100)
{
   mNexusClosedTime = 60 * 1000;
   mNexusOpenTime = 15 * 1000;
   mNexusTimer.reset(mNexusClosedTime);
   mNexusIsOpen = false;
}


bool HuntersGameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc > 0)
   {
      setGameTime((F32)atof(argv[0]) * 60);                       // Game time, stored in minutes in level file
      if(argc > 1)
      {
         mNexusClosedTime = S32(atof(argv[1]) * 60 * 1000);  // Time until nexus opens, specified in minutes
         if(argc > 2)
         {
            mNexusOpenTime = S32(atof(argv[2]) * 1000);      // Time nexus remains open, specified in seconds
            if(argc > 3)
               setWinningScore(atoi(argv[3]));               // Winning score
         }
      }
   }
   mNexusTimer.reset(mNexusClosedTime);

   return true;
}


string HuntersGameType::toString() const
{
   return string(getClassName()) + " " + ftos(F32(getTotalGameTime()) / 60 , 3) + " " + ftos(F32(mNexusClosedTime) / 60 / 1000, 3) + " " + 
                                         ftos(F32(mNexusOpenTime) / 1000, 3) + " " + itos(getWinningScore());
}


void HuntersGameType::addNexus(HuntersNexusObject *nexus)
{
   mNexus.push_back(nexus);
}


bool HuntersGameType::isCarryingItems(Ship *ship)
{
   if(ship->mMountedItems.size() > 1)     // Currently impossible, but in future may be possible
      return true;
   if(ship->mMountedItems.size() == 0)    // Should never happen
      return false;

   MoveItem *item = ship->mMountedItems[0];   // Currently, ship always has a NexusFlagItem... this is it
   if(!item)                                  // Null when a player drop flag and get destroyed at the same time
      return false;  

   return ( ((HuntersFlagItem *) item)->getFlagCount() > 0 );    
}


// Cycle through mounted items and find the first one (last one, actually) that's a HuntersFlagItem.
// Returns NULL if it can't find one.
static HuntersFlagItem *findFirstNexusFlag(Ship *ship)
{
   for(S32 i = ship->mMountedItems.size() - 1; i >= 0; i--)
   {
      MoveItem *item = ship->mMountedItems[i];
      HuntersFlagItem *flag = dynamic_cast<HuntersFlagItem *>(item);

      if(flag)
         return flag;
   }

   return NULL;
}


// The flag will come from ship->mount.  *item is used as it is posssible to carry and drop multiple items
void HuntersGameType::itemDropped(Ship *ship, MoveItem *item)
{
   //HuntersFlagItem *flag = findFirstNexusFlag(ship);  //  This line causes multiple "Drop Flag" messages when ship carry multiple items.
   HuntersFlagItem *flag = dynamic_cast<HuntersFlagItem *>(item);
   if(!flag)
      return;

   U32 flagCount = flag->getFlagCount();

   if(flagCount == 0)  // This is needed if you drop your flags, then pick up a different item type (like resource item), and drop it
      return;

   Vector<StringTableEntry> e;

   e.push_back(ship->getName());
   if(flagCount > 1)
      e.push_back(itos(flagCount).c_str());


   static StringTableEntry dropOneString( "%e0 dropped a flag!");
   static StringTableEntry dropManyString( "%e0 dropped %e1 flags!");

   StringTableEntry *ste = (flagCount > 1) ? &dropManyString : &dropOneString;

   for(S32 i = 0; i < getClientCount(); i++)
      getClient(i)->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, *ste, e);
}


#ifndef ZAP_DEDICATED
// Any unique items defined here must be handled in both getMenuItem() and saveMenuItem() below!
const char **HuntersGameType::getGameParameterMenuKeys()
{
    static const char *items[] = {
      "Level Name",
      "Level Descr",
      "Level Credits",
      "Levelgen Script",
      "Game Time",
      "Nexus Time to Open",      // <=== defined here
      "Nexus Time Remain Open",  // <=== defined here
      "Nexus Win Score",         // <=== defined here
      "Grid Size",
      "Min Players",
      "Max Players",
      "Allow Engr",
      "" };

      return items;
}


// Definitions for those items
boost::shared_ptr<MenuItem> HuntersGameType::getMenuItem(ClientGame *game, const char *key)
{
   if(!strcmp(key, "Nexus Time to Open"))
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItem(game, "Time for Nexus to Open:", mNexusClosedTime / 1000, 99*60, "Never", "Time it takes for the Nexus to open"));
   else if(!strcmp(key, "Nexus Time Remain Open"))
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItemSeconds(game, "Time Nexus Remains Open:", mNexusOpenTime / 1000, 99*60, "Always", "Time that the Nexus will remain open"));
   else if(!strcmp(key, "Nexus Win Score"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem(game, "Score to Win:", getWinningScore(), 100, 100, 20000, "points", "", "Game ends when one player or team gets this score"));
   else return Parent::getMenuItem(game, key);
}


bool HuntersGameType::saveMenuItem(const MenuItem *menuItem, const char *key)
{
   if(!strcmp(key, "Nexus Time to Open"))
      mNexusOpenTime = menuItem->getIntValue();
   else if(!strcmp(key, "Nexus Time Remain Open"))
      mNexusClosedTime = menuItem->getIntValue();
   else if(!strcmp(key, "Nexus Win Score"))
      setWinningScore(menuItem->getIntValue());
   else return Parent::saveMenuItem(menuItem, key);

   return true;
}
#endif


TNL_IMPLEMENT_NETOBJECT(HuntersNexusObject);


TNL_IMPLEMENT_NETOBJECT_RPC(HuntersNexusObject, s2cFlagsReturned, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   getGame()->getGameType()->mZoneGlowTimer.reset();
}


// The nexus is open.  A ship has entered it.  Now what?
// Runs on server only
void HuntersGameType::shipTouchNexus(Ship *theShip, HuntersNexusObject *theNexus)
{
   HuntersFlagItem *theFlag = findFirstNexusFlag(theShip);

   if(!theFlag)      // Just in case!
      return;

   updateScore(theShip, ReturnFlagsToNexus, theFlag->getFlagCount());

   if(theFlag->getFlagCount() > 0)
   {
      s2cHuntersMessage(HuntersMsgScore, theShip->getName().getString(), theFlag->getFlagCount(), getEventScore(TeamScore, ReturnFlagsToNexus, theFlag->getFlagCount()) );
      theNexus->s2cFlagsReturned();    // Alert the Nexus that someone has returned flags to it
   }
   theFlag->changeFlagCount(0);
}


// Runs on the server
void HuntersGameType::onGhostAvailable(GhostConnection *theConnection)
{
   Parent::onGhostAvailable(theConnection);

   NetObject::setRPCDestConnection(theConnection);
   s2cSetNexusTimer(mNexusTimer.getCurrent(), mNexusIsOpen);
   NetObject::setRPCDestConnection(NULL);
}


// Runs on the server
// If a flag is released from a ship, it will have underlying startVel, to which a random vector will be added
void releaseFlag(Game *game, Point pos, Point startVel)
{
   F32 th = TNL::Random::readF() * Float2Pi;
   F32 f = (TNL::Random::readF() * 2 - 1) * 100;
   Point vel(cos(th) * f, sin(th) * f);
   vel += startVel;

   FlagItem *newFlag = new FlagItem(pos, vel, true);
   newFlag->addToGame(game, game->getGameObjDatabase());
}


// Runs on client and server
void HuntersGameType::idle(GameObject::IdleCallPath path, U32 deltaT)
{
   Parent::idle(path, deltaT);

   if(isGhost())     // i.e. on client
   {
      mNexusTimer.update(deltaT);
      for(S32 i = 0; i < mYardSaleWaypoints.size();)
      {
         if(mYardSaleWaypoints[i].timeLeft.update(deltaT))
            mYardSaleWaypoints.erase_fast(i);
         else
            i++;
      }
      return;
   }

   // The following only runs on the server
   if(!mNexusIsOpen && mNexusTimer.update(deltaT))         // Nexus has just opened
   {
      mNexusTimer.reset(mNexusOpenTime);
      mNexusIsOpen = true;
      s2cSetNexusTimer(mNexusTimer.getCurrent(), mNexusIsOpen);
      static StringTableEntry msg("The Nexus is now OPEN!");

      // Broadcast a message
      for(S32 i = 0; i < getClientCount(); i++)
        getClient(i)->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, msg);

      // Check if anyone is already in the Nexus, examining each client's ship in turn...
      for(S32 i = 0; i < getClientCount(); i++)
      {
         Ship *client_ship = dynamic_cast<Ship *>(getClient(i)->clientConnection->getControlObject());

         if(!client_ship)
            continue;

         HuntersNexusObject *nexus = dynamic_cast<HuntersNexusObject *>(client_ship->isInZone(NexusTypeNumber));
         if(nexus)
            shipTouchNexus(client_ship, nexus);
      }
   }
   else if(mNexusIsOpen && mNexusTimer.update(deltaT))       // Nexus has just closed
   {
      mNexusTimer.reset(mNexusClosedTime);
      mNexusIsOpen = false;
      s2cSetNexusTimer(mNexusTimer.getCurrent(), mNexusIsOpen);

      static StringTableEntry msg("The Nexus is now CLOSED.");
      for(S32 i = 0; i < getClientCount(); i++)
         getClient(i)->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, msg);
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
S32 HuntersGameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 flags)
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


//////////  Client only code:

extern Color gNexusOpenColor;
extern Color gNexusClosedColor;

#define NEXUS_STR mNexusIsOpen ?  "Nexus closes: " : "Nexus opens: "
#define NEXUS_NEVER_STR mNexusIsOpen ? "Nexus never closes" : "Nexus never opens"

#ifndef ZAP_DEDICATED
void HuntersGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);

   glColor(mNexusIsOpen ? gNexusOpenColor : gNexusClosedColor);      // Display timer in appropriate color

   U32 timeLeft = mNexusTimer.getCurrent();
   U32 minsRemaining = timeLeft / (60000);
   U32 secsRemaining = (timeLeft - (minsRemaining * 60000)) / 1000;

   const S32 y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 45;
   const S32 size = 20;

   if(mNexusTimer.getPeriod() == 0)
   {
      S32 x =  gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - UserInterface::getStringWidth(size, NEXUS_NEVER_STR);
      UserInterface::drawStringf(x, y, size, NEXUS_NEVER_STR);
   }
   else
   {
      S32 x =  gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 65 - UserInterface::getStringWidth(size, NEXUS_STR);
      UserInterface::drawStringf(x, y, size, "%s%02d:%02d", NEXUS_STR, minsRemaining, secsRemaining);
   }

   for(S32 i = 0; i < mYardSaleWaypoints.size(); i++)
      renderObjectiveArrow(&mYardSaleWaypoints[i].pos, &Colors::white);

   for(S32 i = 0; i < mNexus.size(); i++)
      renderObjectiveArrow(dynamic_cast<GameObject *>(mNexus[i].getPointer()), mNexusIsOpen ? &gNexusOpenColor : &gNexusClosedColor);
}
#endif

#undef NEXUS_STR


//////////  END Client only code



void HuntersGameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *theShip = dynamic_cast<Ship *>(clientObject);
   if(!theShip)
      return;

   // Check for yard sale  (is this when the flags a player is carrying go drifting about??)
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      MoveItem *item = theShip->mMountedItems[i];
      HuntersFlagItem *flag = dynamic_cast<HuntersFlagItem *>(item);

      if(flag)
      {
         if(flag->getFlagCount() >= YardSaleCount)
         {
            Point pos = flag->getActualPos();
            s2cAddYardSaleWaypoint(pos.x, pos.y);
            s2cHuntersMessage(HuntersMsgYardSale, theShip->getName().getString(), 0, 0);
         }

         return;
      }
   }
}


void HuntersGameType::shipTouchFlag(Ship *theShip, FlagItem *theFlag)
{
   // Don't mount to ship, instead increase current mounted HuntersFlag
   //    flagCount, and remove collided flag from game
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      HuntersFlagItem *theFlag = dynamic_cast<HuntersFlagItem *>(theShip->mMountedItems[i].getPointer());
      if(theFlag)
      {
         theFlag->changeFlagCount(theFlag->getFlagCount() + 1);
         break;
      }
   }

   theFlag->setCollideable(false);
   theFlag->removeFromDatabase();
   theFlag->deleteObject();
}


// Special spawn function for Hunters games (runs only on server)
void HuntersGameType::spawnShip(GameConnection *theClient)
{
   Parent::spawnShip(theClient);

   HuntersFlagItem *newFlag = new HuntersFlagItem(theClient->getControlObject()->getActualPos());
   newFlag->addToGame(getGame(), getGame()->getGameObjDatabase());
   newFlag->mountToShip(dynamic_cast<Ship *>(theClient->getControlObject()));    // mountToShip() can handle NULL
   newFlag->changeFlagCount(0);
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(HuntersFlagItem);

// C++ constructor
HuntersFlagItem::HuntersFlagItem(Point pos, Point vel, bool useDropDelay) : FlagItem(pos, true, (F32)Ship::CollisionRadius, 4)  // radius was 30, which had problem with sticking to wall when drop too close to walls
{
   mFlagCount = 0;

   setActualVel(vel);
   if(useDropDelay)
      mDroppedTimer.reset(DROP_DELAY);
}


//////////  Client only code:

void HuntersFlagItem::renderItem(const Point &pos)
{
#ifndef ZAP_DEDICATED
   // Don't render flags on cloaked ships
   if(mMount.isValid() && mMount->isModuleActive(ModuleCloak))
      return;

   Parent::renderItem(pos);

   if(mIsMounted && mFlagCount > 0)
   {
      if(mFlagCount >= 40)
         glColor3f(1, 0.5, 0.5);
      else if(mFlagCount >= 20)
         glColor3f(1, 1, 0);
      else if(mFlagCount >= 10)
         glColor3f(0, 1, 0);
      else
         glColor3f(1, 1, 1);

      UserInterface::drawStringf(pos.x + 10, pos.y - 46, 12, "%d", mFlagCount);
   }
#endif
}

//////////  END Client only code



// Private helper function
void HuntersFlagItem::dropFlags(U32 flags)
{
   if(!mMount.isValid())
      return;

   if(isGhost())  //avoid problem with adding flag to client, when it doesn't really exist on server.
      return;

   for(U32 i = 0; i < flags; i++)
      releaseFlag(getGame(), mMount->getActualPos(), mMount->getActualVel());

   changeFlagCount(0);
}


void HuntersFlagItem::onMountDestroyed()
{
   if(mMount->getOwner())
      mMount->getOwner()->mStatistics.mFlagDrop += mFlagCount + 1;

   dropFlags(mFlagCount + 1);    // Drop at least one flag plus as many as the ship carries

   // Now delete the flag itself
   dismount();
   removeFromDatabase();
   deleteObject();
}


void HuntersFlagItem::onItemDropped()
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


U32 HuntersFlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & FlagCountMask))
      stream->write(mFlagCount);

   return Parent::packUpdate(connection, updateMask, stream);
}

void HuntersFlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      stream->read(&mFlagCount);

   Parent::unpackUpdate(connection, stream);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
HuntersNexusObject::HuntersNexusObject()
{
   mObjectTypeNumber = NexusTypeNumber;
   mNetFlags.set(Ghostable);
}


HuntersNexusObject *HuntersNexusObject::clone() const
{
   return new HuntersNexusObject(*this);
}


// The nexus object itself
// If there are 2 or 4 params, this is an Zap! rectangular format object
// If there are more, this is a Bitfighter polygonal format object
// Note parallel code in EditorUserInterface::processLevelLoadLine
bool HuntersNexusObject::processArguments(S32 argc2, const char **argv2, Game *game)
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

   if(argc <= 4)     // Zap! format
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
   }
   else              // Bitfighter format
      readGeom(argc, argv, 0, game->getGridSize());

   setExtent();      // Sets object's extent database

   return true;
}


string HuntersNexusObject::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + geomToString(gridSize);
}


void HuntersNexusObject::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();    // Always visible!

   HuntersGameType *gt = dynamic_cast<HuntersGameType *>( getGame()->getGameType() );
   if(gt) gt->addNexus(this);
}

void HuntersNexusObject::idle(GameObject::IdleCallPath path)
{
   //U32 deltaT = mCurrentMove.time;
}


void HuntersNexusObject::render()
{
   GameType *gt = getGame()->getGameType();
   HuntersGameType *theGameType = dynamic_cast<HuntersGameType *>(gt);
   renderNexus(getOutline(), getFill(), getCentroid(), getLabelAngle(), 
              (theGameType && theGameType->mNexusIsOpen), gt ? gt->mZoneGlowTimer.getFraction() : 0);
}


void HuntersNexusObject::renderDock()
{
  renderNexus(getOutline(), getFill(), false, 0);
}




void HuntersNexusObject::renderEditor(F32 currentScale)
{
   render();
   EditorPolygon::renderEditor(currentScale);
}


bool HuntersNexusObject::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints = *getOutline();
   return true;
}


bool HuntersNexusObject::collide(GameObject *hitObject)
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

   HuntersGameType *theGameType = dynamic_cast<HuntersGameType *>(getGame()->getGameType());
   if(theGameType && theGameType->mNexusIsOpen)          // Is the nexus open?
      theGameType->shipTouchNexus(theShip, this);

   return false;
}


U32 HuntersNexusObject::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   packGeom(connection, stream);

   return 0;
}


void HuntersNexusObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   unpackGeom(connection, stream);      
   setExtent();     // Sets object's extent database
}

const char HuntersNexusObject::className[] = "HuntersNexusObject";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<HuntersNexusObject>::RegType HuntersNexusObject::methods[] =
{
   // Standard gameItem methods
   method(HuntersNexusObject, getClassID),
   method(HuntersNexusObject, getLoc),
   method(HuntersNexusObject, getRad),
   method(HuntersNexusObject, getVel),
   method(HuntersNexusObject, getTeamIndx),

   {0,0}    // End method list
};

};

