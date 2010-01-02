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
#include "SweptEllipsoid.h"      // For centroid calculation for labeling

#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(HuntersGameType);


TNL_IMPLEMENT_NETOBJECT_RPC(HuntersGameType, s2cSetNexusTimer, (U32 nexusTime, bool isOpen), (nexusTime, isOpen), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mNexusReturnTimer.reset(nexusTime);
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
   if(msgIndex == HuntersMsgScore)
   {
      SFXObject::play(SFXFlagCapture);
      gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f),"%s returned %d flag%s to the Nexus for %d points!", clientName.getString(), flagCount, flagCount > 1 ? "s" : "", score);
   }
   else if(msgIndex == HuntersMsgYardSale)
   {
      SFXObject::play(SFXFlagSnatch);
      gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f),
                  "%s is having a YARD SALE!",
                  clientName.getString());
   }
   else if(msgIndex == HuntersMsgGameOverWin)
   {
      gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f),
                     "Player %s wins the game!",
                     clientName.getString());
      SFXObject::play(SFXFlagCapture);
   }
   else if(msgIndex == HuntersMsgGameOverTie)
   {
      gGameUserInterface.displayMessage(Color(0.6f, 1.0f, 0.8f),
                     "The game ended in a tie.");
      SFXObject::play(SFXFlagDrop);
   }
}

// Constructor
HuntersGameType::HuntersGameType() : GameType()
{
   mNexusIsOpen = false;
   mNexusReturnDelay = 60 * 1000;
   mNexusCapDelay = 15 * 1000;
   mNexusReturnTimer.reset(mNexusReturnDelay);
   mNexusCapTimer.reset(0);
}


void HuntersGameType::addNexus(HuntersNexusObject *nexus)
{
   mNexus.push_back(nexus);
}


bool HuntersGameType::processArguments(S32 argc, const char **argv)
{
   if(argc > 0)
   {
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));     // Game time
      if(argc > 1)
      {
         mNexusReturnDelay = atoi(argv[1]) * 60 * 1000;     // Time until nexus opens
         if(argc > 2)
         {
            mNexusCapDelay = atoi(argv[2]) * 1000;          // Time nexus remains open
            if(argc > 3)
               mWinningScore = atoi(argv[3]);               // Winning score
         }
      }
   }
   mNexusReturnTimer.reset(mNexusReturnDelay);

   return true;
}


bool HuntersGameType::isCarryingItems(Ship *ship)
{
   if(ship->mMountedItems.size() > 1)     // Currently impossible, but in future may be possible
      return true;
   if(ship->mMountedItems.size() == 0)    // Should never happen
      return false;

   Item *item = ship->mMountedItems[0];   // Currently, ship always has a NexusFlagItem... this is it
   return ( ((HuntersFlagItem *) item)->getFlagCount() > 0 );    
}


// Cycle through mounted items and find the first one (last one, actually) that's a HuntersFlagItem.
// Returns NULL if it can't find one.
HuntersFlagItem *findFirstNexusFlag(Ship *ship)
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


static StringTableEntry dropOneString( "%e0 dropped a flag!");
static StringTableEntry dropManyString( "%e0 dropped %e1 flags!");

void HuntersGameType::flagDropped(Ship *theShip, FlagItem *flag)
{
   HuntersFlagItem *theFlag = findFirstNexusFlag(theShip);
   U32 flagCount = theFlag ? theFlag->getFlagCount() : 0;

   if(flagCount == 0)
      return;

   Vector<StringTableEntry> e;

   e.push_back(theShip->getName());
   if(flagCount > 1)
      e.push_back(UserInterface::itos(flagCount).c_str());

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(flagCount > 1)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropManyString, e);
      else
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagDrop, dropOneString, e);
   }
}


// Describe the arguments processed above...
Vector<GameType::ParameterDescription> HuntersGameType::describeArguments()
{
   Vector<GameType::ParameterDescription> descr;

   GameType::ParameterDescription item;
   item.name = "Game Time:";
   item.help = "Time game will last";
   item.value = 10;
   item.units = "mins";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Time for Nexus to Open:";
   item.help = "Time it takes for the Nexus to open";
   item.value = 1;
   item.units = "mins";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Time Nexus Remains Open:";
   item.help = "Time that the Nexus will remain open";
   item.value = 30;
   item.units = "secs";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Score Limit:";
   item.help = "Game ends when one player or team gets this score";
   item.value = 5000;
   item.units = "points";
   item.minval = 100;
   item.maxval = 20000;
   descr.push_back(item);

   return descr;
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
   if(mNexusIsOpen)
      s2cSetNexusTimer(mNexusCapTimer.getCurrent(), mNexusIsOpen);
   else
      s2cSetNexusTimer(mNexusReturnTimer.getCurrent(), mNexusIsOpen);
   NetObject::setRPCDestConnection(NULL);
}


// Runs on the server
// If a flag is released from a ship, it will have underlying startVel, to which a random vector will be added
void releaseFlag(Game *game, Point pos, Point startVel)
{
   HuntersFlagItem *newFlag = new HuntersFlagItem(pos);
   newFlag->addToGame(game);

   F32 th = TNL::Random::readF() * Float2Pi;
   F32 f = (TNL::Random::readF() * 2 - 1) * 100;
   Point vel(cos(th) * f, sin(th) * f);
   vel += startVel;

   newFlag->setActualVel(vel);

   newFlag->mDroppedTimer.reset(newFlag->dropDelay);
}


void HuntersGameType::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);

   U32 deltaT = mCurrentMove.time;
   if(isGhost())     // i.e. on client
   {
      mNexusReturnTimer.update(deltaT);
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
   if(mNexusReturnTimer.update(deltaT))         // Nexus has opened
   {
      mNexusCapTimer.reset(mNexusCapDelay);
      mNexusIsOpen = true;
      s2cSetNexusTimer(mNexusCapTimer.getCurrent(), mNexusIsOpen);
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
   else if(mNexusCapTimer.update(deltaT))       // Nexus has closed
   {
      mNexusReturnTimer.reset(mNexusReturnDelay);
      mNexusIsOpen = false;
      s2cSetNexusTimer(mNexusReturnTimer.getCurrent(), mNexusIsOpen);
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


void HuntersGameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   Parent::renderInterfaceOverlay(scoreboardVisible);

   glColor(mNexusIsOpen ? gNexusOpenColor : gNexusClosedColor);      // Display timer in appropriate color

   U32 timeLeft = mNexusReturnTimer.getCurrent();
   U32 minsRemaining = timeLeft / (60000);
   U32 secsRemaining = (timeLeft - (minsRemaining * 60000)) / 1000;

   UserInterface::drawStringf(UserInterface::canvasWidth - UserInterface::horizMargin - 65 - UserInterface::getStringWidth(20, NEXUS_STR),
      UserInterface::canvasHeight - UserInterface::vertMargin - 45, 20, "%s%02d:%02d", NEXUS_STR, minsRemaining, secsRemaining);

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


// Special spawn function for Hunters games (runs only on server)
void HuntersGameType::spawnShip(GameConnection *theClient)
{
   Parent::spawnShip(theClient);

   HuntersFlagItem *newFlag = new HuntersFlagItem(theClient->getControlObject()->getActualPos());
   newFlag->addToGame(getGame());
   newFlag->mountToShip(dynamic_cast<Ship *>(theClient->getControlObject()));    // mountToShip() can handle NULL
   newFlag->changeFlagCount(0);
}

///////////////////

TNL_IMPLEMENT_NETOBJECT(HuntersFlagItem);

// C++ constructor
HuntersFlagItem::HuntersFlagItem(Point pos) : Item(pos, true, 30, 4)
{
   mObjectTypeMask |= CommandMapVisType;
   mNetFlags.set(Ghostable);
   mFlagCount = 0;
}


bool HuntersFlagItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 2)
      return false;

   // If this is a regular FlagItem that was "recast" as a HuntersFlagItem at load time, it will have 3 args.  The first, team,
   // does not apply to this object, so we'll ignore it.  If there's only 2 args, those will be the coords.
   S32 offset = 0;
   if(argc >= 3)
      offset++;

   if(!Parent::processArguments(argc - 1, argv + offset))
      return false;

   return true;
}


const char HuntersFlagItem::className[] = "HuntersFlagItem";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<HuntersFlagItem>::RegType HuntersFlagItem::methods[] =
{
   // Standard gameItem methods
   method(HuntersFlagItem, getClassID),
   method(HuntersFlagItem, getLoc),
   method(HuntersFlagItem, getRad),
   method(HuntersFlagItem, getVel),

   {0,0}    // End method list
};


extern Color gErrorMessageTextColor;

void HuntersFlagItem::renderItem(Point pos)
{
   if(mMount.isValid() && mMount->isModuleActive(ModuleCloak))
      return;

   Point offset = pos;

   if(mIsMounted)
      offset.set(pos.x + 15, pos.y - 15);

   Color c;
   GameType *gt = getGame()->getGameType();

   c = gt->mTeams[0].color;   // Maybe should always be rendered in player's team color?  Or unused color?

   renderFlag(offset, c);

   if(mIsMounted)
   {
      if(mFlagCount)
      {
         if(mFlagCount > 20)
            glColor3f(1, 0.5, 0.5);
         else if(mFlagCount > 10)
            glColor3f(1, 1, 0);
         else
            glColor3f(1, 1, 1);
         UserInterface::drawStringf(offset.x - 5, offset.y - 30, 12, "%d", mFlagCount);
      }
   }
}


// Private helper function
void HuntersFlagItem::dropFlags(U32 flags)
{
   if(!mMount.isValid())
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


void HuntersFlagItem::onItemDropped(Ship *ship)
{
   getGame()->getGameType()->flagDropped(mMount, NULL);
   dropFlags(mFlagCount);        // Only dropping the flags we're carrying, not the "extra" one that comes when we die
}


void HuntersFlagItem::setActualVel(Point v)
{
   mMoveState[ActualState].vel = v;
   setMaskBits(WarpPositionMask | PositionMask);
}


bool HuntersFlagItem::collide(GameObject *hitObject)
{
   if(mIsMounted || !mIsCollideable)
      return false;

   if(hitObject->getObjectTypeMask() & BarrierType)
      return true;

   if(isGhost() || ! (hitObject->getObjectTypeMask() & ShipType || hitObject->getObjectTypeMask() & RobotType))
      return false;

   if(mDroppedTimer.getCurrent())    // Dropped flag not ready to be picked up! 
      return false;

   Ship *theShip = static_cast<Ship *>(hitObject);
   if(!theShip)
      return false;

   if(theShip->hasExploded)
      return false;

   // Don't mount to ship, instead increase current mounted HuntersFlag
   //    flagCount, and remove collided flag from game
   for(S32 i = theShip->mMountedItems.size() - 1; i >= 0; i--)
   {
      Item *theItem = theShip->mMountedItems[i];
      HuntersFlagItem *theFlag = dynamic_cast<HuntersFlagItem *>(theItem);
      if(theFlag)
      {
         theFlag->changeFlagCount(theFlag->getFlagCount() + 1);
         break;
      }
   }

   mIsCollideable = false;
   removeFromDatabase();
   deleteObject();
   return true;
}

U32 HuntersFlagItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);
   if(stream->writeFlag(updateMask & FlagCountMask))
      stream->write(mFlagCount);

   return retMask;
}

void HuntersFlagItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag())
      stream->read(&mFlagCount);
}


void HuntersFlagItem::idle(GameObject::IdleCallPath path)
{
   Parent::idle(path);

   if(isGhost()) 
      return;
   
   U32 deltaT = mCurrentMove.time;
   mDroppedTimer.update(deltaT);
}


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
bool HuntersNexusObject::processArguments(S32 argc, const char **argv)
{
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
      processPolyBounds(argc, argv, 0, mPolyBounds, getGame()->getGridSize());

   computeExtent();

   return true;
}

void HuntersNexusObject::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);
   setExtent(extent);
}

void HuntersNexusObject::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();    // Always visible!

   dynamic_cast<HuntersGameType *>( getGame()->getGameType() )->addNexus(this);
   getGame()->mObjectsLoaded++;
}

void HuntersNexusObject::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
}


void HuntersNexusObject::render()
{
   HuntersGameType *theGameType = dynamic_cast<HuntersGameType *>(getGame()->getGameType());
   renderNexus(mPolyBounds, mPolyFill, mCentroid, mLabelAngle, (theGameType && theGameType->mNexusIsOpen), theGameType->mZoneGlowTimer.getFraction());
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
   packPolygonUpdate(connection, stream);

   return 0;
}


void HuntersNexusObject::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(unpackPolygonUpdate(connection, stream))
      computeExtent();
}


};

