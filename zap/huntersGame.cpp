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
#include "UIGame.h"
#include "sfx.h"
#include "gameNetInterface.h"
#include "ship.h"
#include "gameObjectRender.h"
#include "GeomUtils.h"      // For centroid calculation for labeling
#include "stringUtils.h"         // For itos

#include "../glut/glutInclude.h"

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
   ClientGame *clientGame = dynamic_cast<ClientGame *>(getGame());
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   if(msgIndex == HuntersMsgScore)
   {
      SFXObject::play(SFXFlagCapture);
      clientGame->mGameUserInterface->displayMessage(Color(0.6f, 1.0f, 0.8f),"%s returned %d flag%s to the Nexus for %d points!", clientName.getString(), flagCount, flagCount > 1 ? "s" : "", score);
   }
   else if(msgIndex == HuntersMsgYardSale)
   {
      SFXObject::play(SFXFlagSnatch);
      clientGame->mGameUserInterface->displayMessage(Color(0.6f, 1.0f, 0.8f),
                  "%s is having a YARD SALE!",
                  clientName.getString());
   }
   else if(msgIndex == HuntersMsgGameOverWin)
   {
      clientGame->mGameUserInterface->displayMessage(Color(0.6f, 1.0f, 0.8f),
                     "Player %s wins the game!",
                     clientName.getString());
      SFXObject::play(SFXFlagCapture);
   }
   else if(msgIndex == HuntersMsgGameOverTie)
   {
      clientGame->mGameUserInterface->displayMessage(Color(0.6f, 1.0f, 0.8f), "The game ended in a tie.");
      SFXObject::play(SFXFlagDrop);
   }
}

// Constructor
HuntersGameType::HuntersGameType() : GameType()
{
   mNexusClosedTime = 60 * 1000;
   mNexusOpenTime = 15 * 1000;
   mNexusTimer.reset(mNexusClosedTime);
   mNexusIsOpen = false;
}



bool HuntersGameType::processArguments(S32 argc, const char **argv)
{
   if(argc > 0)
   {
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));       // Game time
      if(argc > 1)
      {
         mNexusClosedTime = S32(atof(argv[1]) * 60 * 1000);  // Time until nexus opens, specified in minutes
         if(argc > 2)
         {
            mNexusOpenTime = S32(atof(argv[2]) * 1000);       // Time nexus remains open, specified in seconds
            if(argc > 3)
               mWinningScore = atoi(argv[3]);                 // Winning score
         }
      }
   }
   mNexusTimer.reset(mNexusClosedTime);

   return true;
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

   Item *item = ship->mMountedItems[0];   // Currently, ship always has a NexusFlagItem... this is it
   if(! item) return false;               // Null when a player drop flag and get destroyed at the same time.
   return ( ((HuntersFlagItem *) item)->getFlagCount() > 0 );    
}


// Cycle through mounted items and find the first one (last one, actually) that's a HuntersFlagItem.
// Returns NULL if it can't find one.
static HuntersFlagItem *findFirstNexusFlag(Ship *ship)
{
   HuntersFlagItem *theFlag = NULL;

   for(S32 i = ship->mMountedItems.size() - 1; i >= 0; i--)
   {
      Item *theItem = ship->mMountedItems[i];
      theFlag = dynamic_cast<HuntersFlagItem *>(theItem);

      if(theFlag)
         return theFlag;
   }

   return NULL;
}


// The flag will come from ship->mount.  *item is used as it is posssible to carry and drop multiple items
void HuntersGameType::itemDropped(Ship *ship, Item *item)
{
   //HuntersFlagItem *flag = findFirstNexusFlag(ship);  //  This line causes multiple "Drop Flag" messages when ship carry multiple items.
   HuntersFlagItem *flag = dynamic_cast<HuntersFlagItem *>(item);
   if(! flag)
      return;

   U32 flagCount = flag ? flag->getFlagCount() : 0;
   if(flagCount == 0)
      return;

   Vector<StringTableEntry> e;

   e.push_back(ship->getName());
   if(flagCount > 1)
      e.push_back(itos(flagCount).c_str());


   static StringTableEntry dropOneString( "%e0 dropped a flag!");
   static StringTableEntry dropManyString( "%e0 dropped %e1 flags!");

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(flagCount > 1)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropManyString, e);
      else
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropOneString, e);
   }
}


// Create some game-specific menu items for the GameParameters menu from the arguments processed above...
void HuntersGameType::addGameSpecificParameterMenuItems(Vector<MenuItem *> &menuItems)
{
   menuItems.push_back(new TimeCounterMenuItem("Game Time:", 8 * 60, 99*60, "Unlimited", "Time game will last"));
   menuItems.push_back(new TimeCounterMenuItem("Time for Nexus to Open:", 60, 99*60, "Never", "Time it takes for the Nexus to open"));
   menuItems.push_back(new TimeCounterMenuItemSeconds("Time Nexus Remains Open:", 30, 99*60, "Always", "Time that the Nexus will remain open"));
   menuItems.push_back(new CounterMenuItem("Score to Win:", 5000, 100, 100, 20000, "points", "", "Game ends when one player or team gets this score"));
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

   HuntersFlagItem *newFlag = new HuntersFlagItem(pos, vel, true);
   newFlag->addToGame(game);
}


void HuntersGameType::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);

   U32 deltaT = mCurrentMove.time;
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
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagSnatch, msg);

      // Check if anyone is already in the Nexus, examining each client's ship in turn...
      for(S32 i = 0; i < mClientList.size(); i++)
      {
         Ship *client_ship = dynamic_cast<Ship *>(mClientList[i]->clientConnection->getControlObject());

         if(!client_ship)
            continue;

         HuntersNexusObject *nexus = dynamic_cast<HuntersNexusObject *>(client_ship->isInZone(NexusType));
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
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, msg);
   }

   // Advance all flagSpawn timers and see if it's time for a new flag
   for(S32 i = 0; i < mFlagSpawnPoints.size(); i++)
   {
      if(mFlagSpawnPoints[i].timer.update(deltaT))
      {
         releaseFlag(getGame(), mFlagSpawnPoints[i].getPos(), Point(0,0));   // Release a flag
         mFlagSpawnPoints[i].timer.reset();                                  // Reset the timer
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


extern Color gNexusOpenColor;
extern Color gNexusClosedColor;

#define NEXUS_STR mNexusIsOpen ?  "Nexus closes: " : "Nexus opens: "
#define NEXUS_NEVER_STR mNexusIsOpen ? "Nexus never closes" : "Nexus never opens"

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
      renderObjectiveArrow(mYardSaleWaypoints[i].pos, Color(1,1,1));

   for(S32 i = 0; i < mNexus.size(); i++)
      renderObjectiveArrow(mNexus[i], mNexusIsOpen ? gNexusOpenColor : gNexusClosedColor);
}

#undef NEXUS_STR


void HuntersGameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   Parent::controlObjectForClientKilled(theClient, clientObject, killerObject);

   Ship *theShip = dynamic_cast<Ship *>(clientObject);
   if(!theShip)
      return;

   // Check for yard sale  (is this when the flags a player is carrying go drifting about??)
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      Item *theItem = theShip->mMountedItems[i];
      HuntersFlagItem *theFlag = dynamic_cast<HuntersFlagItem *>(theItem);
      if(theFlag)
      {
         if(theFlag->getFlagCount() >= YardSaleCount)
         {
            Point pos = theFlag->getActualPos();
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
   newFlag->addToGame(getGame());
   newFlag->mountToShip(dynamic_cast<Ship *>(theClient->getControlObject()));    // mountToShip() can handle NULL
   newFlag->changeFlagCount(0);
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(HuntersFlagItem);

// C++ constructor
HuntersFlagItem::HuntersFlagItem(Point pos, Point vel, bool useDropDelay) : FlagItem(pos, true, Ship::CollisionRadius, 4)  // radius was 30, which had problem with sticking to wall when drop too close to walls
{
   mFlagCount = 0;

   setActualVel(vel);
   if(useDropDelay)
      mDroppedTimer.reset(DROP_DELAY);
}


//const char HuntersFlagItem::className[] = "HuntersFlagItem";      // Class name as it appears to Lua scripts

//// Define the methods we will expose to Lua
//Lunar<HuntersFlagItem>::RegType HuntersFlagItem::methods[] =
//{
//   // Standard gameItem methods
//   method(HuntersFlagItem, getClassID),
//   method(HuntersFlagItem, getLoc),
//   method(HuntersFlagItem, getRad),
//   method(HuntersFlagItem, getVel),
//
//   {0,0}    // End method list
//};


void HuntersFlagItem::renderItem(Point pos)
{
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
}


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

   return Parent::Parent::packUpdate(connection, updateMask, stream);
}

void HuntersFlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
      stream->read(&mFlagCount);

   Parent::Parent::unpackUpdate(connection, stream);
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(HuntersNexusObject);


TNL_IMPLEMENT_NETOBJECT_RPC(HuntersNexusObject, s2cFlagsReturned, (), (), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   gClientGame->getGameType()->mZoneGlowTimer.reset();
}

// Constructor
HuntersNexusObject::HuntersNexusObject()
{
   mObjectTypeMask |= NexusType | CommandMapVisType;
   mNetFlags.set(Ghostable);
}

extern S32 gMaxPolygonPoints;

// The nexus object itself
// If there are 2 or 4 params, this is an Zap! rectangular format object
// If there are more, this is a Bitfighter polygonal format object
// Note parallel code in EditorUserInterface::processLevelLoadLine
bool HuntersNexusObject::processArguments(S32 argc2, const char **argv2)
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
      pos *= getGame()->getGridSize();

      Point ext(50, 50);
      if(argc == 4)
         ext.set(atoi(argv[2]), atoi(argv[3]));

      Point p;
      p.set(pos.x - ext.x, pos.y - ext.y);   // UL corner
      mPolyBounds.push_back(p);
      p.set(pos.x + ext.x, pos.y - ext.y);   // UR corner
      mPolyBounds.push_back(p);
      p.set(pos.x + ext.x, pos.y + ext.y);   // LR corner
      mPolyBounds.push_back(p);
      p.set(pos.x - ext.x, pos.y + ext.y);   // LL corner
      mPolyBounds.push_back(p);
   }
   else           // Bitfighter format
      processPolyBounds(argc, argv, 0, getGame()->getGridSize());

   computeExtent();

   return true;
}


string HuntersNexusObject::toString()
{
   return string(getClassName()) + " " + boundsToString(getGame()->getGridSize());
}


void HuntersNexusObject::computeExtent()
{
   setExtent(computePolyExtents());
}


void HuntersNexusObject::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();    // Always visible!

   HuntersGameType *gt = dynamic_cast<HuntersGameType *>( getGame()->getGameType() );
   if(gt) gt->addNexus(this);
   getGame()->mObjectsLoaded++;
}

void HuntersNexusObject::idle(GameObject::IdleCallPath path)
{
   //U32 deltaT = mCurrentMove.time;
}


void HuntersNexusObject::render()
{
   GameType *gt = getGame()->getGameType();
   HuntersGameType *theGameType = dynamic_cast<HuntersGameType *>(gt);
   renderNexus(mPolyBounds, mPolyFill, mCentroid, mLabelAngle, (theGameType && theGameType->mNexusIsOpen), gt ? gt->mZoneGlowTimer.getFraction() : 0);
}


bool HuntersNexusObject::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}


bool HuntersNexusObject::collide(GameObject *hitObject)
{
   if(isGhost())
      return false;

   // From here on out, runs on server only

   if( ! (hitObject->getObjectTypeMask() & (ShipType | RobotType)))
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
   EditorPolygon::packUpdate(connection, stream);

   return 0;
}


void HuntersNexusObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   EditorPolygon::unpackUpdate(connection, stream);      
   computeExtent();
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

