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

#include "item.h"
#include "ship.h"
#include "goalZone.h"
#include "gameType.h"
#include "flagItem.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{

static U32 sItemId = 1;

// Constructor
Item::Item(Point p, bool collideable, float radius, float mass) : MoveObject(p, radius, mass)
{
   mIsMounted = false;
   mIsCollideable = collideable;
   mObjectTypeMask = MoveableType | CommandMapVisType;
   mInitial = false;

   //if(getGame()->isServer())
   //{
      mItemId = sItemId;
      sItemId++;
   //}
   //else  // is client
   //   mItemId = 0;
   updateTimer = 0;
}


// Server only
bool Item::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= game->getGridSize();
   setVert(pos, 0);

   for(U32 i = 0; i < MoveStateCount; i++)
      mMoveState[i].pos = pos;

   updateExtent();

   return true;
}


////////////////////////////////////////
////////////////////////////////////////

// TODO: merge with simpleLine values, put in editor
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;
static const Color INSTRUCTION_TEXTCOLOR(1,1,1);      // TODO: Put in editor


 // Constructor
EditorPointObject::EditorPointObject(GameObjectType objectType) : EditorObject(objectType) 
{ 
   mGeometry = boost::shared_ptr<Geometry>(new PointGeometry); 
}     


// Copy constructor -- make sure each copy gets its own geometry object
EditorPointObject::EditorPointObject(const EditorPointObject &epo)
{
   mGeometry = boost::shared_ptr<Geometry>(new PointGeometry(*((PointGeometry *)epo.mGeometry.get())));  
}


// Offset: negative below the item, positive above
void EditorPointObject::renderItemText(const char *text, S32 offset, F32 currentScale)
{
   glColor(INSTRUCTION_TEXTCOLOR);
   S32 off = (INSTRUCTION_TEXTSIZE + INSTRUCTION_TEXTGAP) * offset - 10 - ((offset > 0) ? 5 : 0);
   Point pos = convertLevelToCanvasCoord(getVert(0));
   UserInterface::drawCenteredString(pos.x, pos.y - off, INSTRUCTION_TEXTSIZE, text);
}


void EditorPointObject::addToDock(Game *game, const Point &point)
{
   setVert(point, 0);
   Parent::addToDock(game, point);
}


////////////////////////////////////////
////////////////////////////////////////

string EditorItem::toString()
{
   F32 gs = getGame()->getGridSize();
   return string(getClassName()) + " " + (getVert(0) / gs).toString();
}


// Client only, in-game
void Item::render()
{
   // If the item is mounted, renderItem will be called from the ship it is mounted to
   if(mIsMounted)
      return;

   renderItem(mMoveState[RenderState].pos);
}


// This scaling factor allows us to draw actual item, letting it grow and shrink with editor scale, but places a limit on how small it will get
F32 EditorItem::getEditorRenderScaleFactor(F32 currentScale)
{
   const F32 thresh = 0.5;      // Size at which we'll stop rendering actual size, to keep item from getting too small

   return (currentScale < thresh) ? thresh / currentScale : 1;
}


void EditorItem::renderEditor(F32 currentScale)
{
   F32 scaleFact = getEditorRenderScaleFactor(currentScale);

   glPushMatrix();
      glScalef(scaleFact, scaleFact, 1);

      renderItem(getVert(0) / scaleFact);                    
   glPopMatrix();
}


F32 EditorItem::getEditorRadius(F32 currentScale)
{
   return (getRadius() + 2) * currentScale * getEditorRenderScaleFactor(currentScale);
}


// Runs on both client and server, comes from collision() on the server and the colliding client, and from
// unpackUpdate() in the case of all clients
//
// theShip could be NULL here, and this could still be legit (e.g. flag is in scope, and ship is out of scope)
void Item::mountToShip(Ship *theShip)     
{
   TNLAssert(isGhost() || isInDatabase(), "Error, mount item not in database.");

   if(mMount.isValid() && mMount == theShip)    // Already mounted on ship!  Nothing to do!
      return;

   if(mMount.isValid())                         // Mounted on something else; dismount!
      dismount();

   mMount = theShip;
   if(theShip)
      theShip->mMountedItems.push_back(this);

   mIsMounted = true;
   setMaskBits(MountMask);
}


void Item::onMountDestroyed()
{
   dismount();
}


// Runs on client & server, via different code paths
void Item::onItemDropped()
{
   if(!getGame())    // Can happen on game startup
      return;

   GameType *gt = getGame()->getGameType();
   if(!gt || !mMount.isValid())
      return;

   if(!isGhost())    // Server only; on client calls onItemDropped from dismount
   {
      gt->itemDropped(mMount, this);
      dismount();
   }

   mDroppedTimer.reset(DROP_DELAY);
}


// Client & server, called via different paths
void Item::dismount()
{
   if(mMount.isValid())      // Mount could be null if mount is out of scope, but is dropping an always-in-scope item
   {
      for(S32 i = 0; i < mMount->mMountedItems.size(); i++)
         if(mMount->mMountedItems[i].getPointer() == this)
         {
            mMount->mMountedItems.erase(i);     // Remove mounted item from our mount's list of mounted things
            break;
         }
   }

   if(isGhost())     // Client only; on server, we came from onItemDropped()
      onItemDropped();

   mMount = NULL;
   mIsMounted = false;
   setMaskBits(MountMask | PositionMask);    // Sending position fixes the super annoying "flag that can't be picked up" bug
}


void Item::setActualPos(const Point &p)
{
   mMoveState[ActualState].pos = p;
   mMoveState[ActualState].vel.set(0,0);
   setMaskBits(WarpPositionMask | PositionMask);
}


void Item::setActualVel(const Point &vel)
{
   mMoveState[ActualState].vel = vel;
   setMaskBits(WarpPositionMask | PositionMask);
}


Ship *Item::getMount()
{
   return mMount;
}


void Item::setZone(GoalZone *theZone)
{
   // If the item on which we're setting the zone is a flag (which, at this point, it always will be),
   // we want to make sure to update the zone itself.  This is mostly a convenience for robots searching
   // for objects that meet certain criteria, such as for zones that contain a flag.
   FlagItem *flag = dynamic_cast<FlagItem *>(this);

   if(flag)
   {
      GoalZone *z = ((theZone == NULL) ? flag->getZone() : theZone);

      if(z)
         z->mHasFlag = ((theZone == NULL) ? false : true );
   }

   // Now we can get around to setting the zone, like we came here to do
   mZone = theZone;
   setMaskBits(ZoneMask);
}


void Item::idle(GameObject::IdleCallPath path)
{
   if(!isInDatabase())
      return;

   Parent::idle(path);

   if(mIsMounted)    // Item is mounted on something else
   {
      if(mMount.isNull() || mMount->hasExploded)
      {
         if(!isGhost())    // Server only
            dismount();
      }
      else
      {
         mMoveState[RenderState].pos = mMount->getRenderPos();
         mMoveState[ActualState].pos = mMount->getActualPos();
      }
   }
   else              // Not mounted
   {
      float time = mCurrentMove.time * 0.001f;
      move(time, ActualState, false);
      if(path == GameObject::ServerIdleMainLoop)
      {
         // Only update if it's actually moving...
         if(mMoveState[ActualState].vel.lenSquared() != 0)
         {
            // Update less often on slow moving item, more often on fast moving item, and update when we change velocity.
            // Update at most every 5 seconds.
            updateTimer -= (mMoveState[ActualState].vel.len() + 20) * time;
            if(updateTimer < 0 || mMoveState[ActualState].vel.distSquared(prevMoveVelocity) > 100)
            {
               setMaskBits(PositionMask);
               updateTimer = 100;
               prevMoveVelocity = mMoveState[ActualState].vel;
            }
         }
         else if(prevMoveVelocity.lenSquared() != 0)
         {
            setMaskBits(PositionMask);  // update to client that this item is no longer moving.
            prevMoveVelocity.set(0,0);
         }

         mMoveState[RenderState] = mMoveState[ActualState];

      }
      else
         updateInterpolation();
   }
   updateExtent();

   // Server only...
   U32 deltaT = mCurrentMove.time;
   mDroppedTimer.update(deltaT);
}


static const S32 VEL_POINT_SEND_BITS = 511;     // 511 = 2^9 - 1, the biggest int we can pack into 9 bits.

U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = 0;
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Send id in inital packet
      stream->writeRangedU32(mItemId, 0, U16_MAX);
   }
   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(mMoveState[ActualState].pos, stream);
      writeCompressedVelocity(mMoveState[ActualState].vel, VEL_POINT_SEND_BITS, stream);      
      stream->writeFlag(updateMask & WarpPositionMask);
   }
   if(stream->writeFlag(updateMask & MountMask) && stream->writeFlag(mIsMounted))      // mIsMounted gets written iff MountMask is set  
   {
      S32 index = connection->getGhostIndex(mMount);     // Index of ship with item mounted

      if(stream->writeFlag(index != -1))                 // True if some ship has item, false if nothing is mounted
         stream->writeInt(index, GhostConnection::GhostIdBitSize);
      else
         retMask |= MountMask;
   }
   if(stream->writeFlag(updateMask & ZoneMask))
   {
      if(mZone.isValid())
      {
         S32 index = connection->getGhostIndex(mZone);
         if(stream->writeFlag(index != -1))
            stream->writeInt(index, GhostConnection::GhostIdBitSize);
         else
            retMask |= ZoneMask;
      }
      else
         stream->writeFlag(false);
   }
   return retMask;
}


void Item::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   bool interpolate = false;
   bool positionChanged = false;

   mInitial = stream->readFlag();

   if(mInitial)     // InitialMask
   {
      mItemId = stream->readRangedU32(0, U16_MAX);
   }

   if(stream->readFlag())     // PositionMask
   {
      ((GameConnection *) connection)->readCompressedPoint(mMoveState[ActualState].pos, stream);
      readCompressedVelocity(mMoveState[ActualState].vel, VEL_POINT_SEND_BITS, stream);   
      positionChanged = true;
      interpolate = !stream->readFlag();
   }

   if(stream->readFlag())     // MountMask
   {
      bool isMounted = stream->readFlag();
      if(isMounted)
      {
         Ship *theShip = NULL;
         
         if(stream->readFlag())
            theShip = dynamic_cast<Ship *>(connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize)));

         mountToShip(theShip);
      }
      else
         dismount();
   }

   if(stream->readFlag())     // ZoneMask
   {
      bool hasZone = stream->readFlag();
      if(hasZone)
         mZone = (GoalZone *) connection->resolveGhost(stream->readInt(GhostConnection::GhostIdBitSize));
      else
         mZone = NULL;
   }

   if(positionChanged)
   {
      if(interpolate)
      {
         mInterpolating = true;
         move(connection->getOneWayTime() * 0.001f, ActualState, false);
      }
      else
      {
         mInterpolating = false;
         mMoveState[RenderState] = mMoveState[ActualState];
      }
   }
}


bool Item::collide(GameObject *otherObject)
{
   return mIsCollideable && !mIsMounted;
}


S32 Item::getCaptureZone(lua_State *L) { if(mZone.isValid()) {mZone->push(L); return 1;} else return returnNil(L); }
S32 Item::getShip(lua_State *L) { if(mMount.isValid()) {mMount->push(L); return 1;} else return returnNil(L); }


////////////////////////////////////////
////////////////////////////////////////

EditorItem::EditorItem(Point p, bool collideable, F32 radius, F32 repopDelay) : Item(p, collideable, radius, repopDelay)
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

PickupItem::PickupItem(Point p, float radius, S32 repopDelay) : EditorItem(p, false, radius, 1)
{
   mRepopDelay = repopDelay;
   mIsVisible = true;
   mIsMomentarilyVisible = false;

   mNetFlags.set(Ghostable);
}


void PickupItem::idle(GameObject::IdleCallPath path)
{
   if(!mIsVisible && path == GameObject::ServerIdleMainLoop)
   {
      if(mRepopTimer.update(mCurrentMove.time))
      {
         setMaskBits(PickupMask);
         mIsVisible = true;

         // Check if there is a ship sitting on this item... it so, ship gets the repair!
         GameType *gt = getGame()->getGameType();
         if(gt)
         {
            for(S32 i = 0; i < gt->mClientList.size(); i++)
            {
               TNLAssert(gt->mClientList[i]->clientConnection, "Defunct client connection in item.cpp!");

               if(!gt->mClientList[i]->clientConnection)    // <-- not sure this ever happens
                  continue;

               Ship *client_ship = dynamic_cast<Ship *>(gt->mClientList[i]->clientConnection->getControlObject());

               if(!client_ship)
                  continue;
               if(client_ship->isOnObject(this)) {
                  S32 i = 1;
               }
               if(client_ship->isOnObject(this))
               {
                  collide(client_ship);
                  mIsMomentarilyVisible = true;
               }
            }
         }
      }
   }

   updateExtent();
}


bool PickupItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 2)
      return false;
   else if(!Parent::processArguments(argc, argv, game))
      return false;

   if(argc == 3)
   {
      S32 repopDelay = atoi(argv[2]) * 1000;    // 3rd param is time for this to regenerate in seconds
      if(repopDelay > 0)
         mRepopDelay = repopDelay;
      else
         mRepopDelay = -1;
   }

   return true;
}


string PickupItem::toString()
{
   F32 gs = getGame()->getGridSize();
   return string(getClassName()) + " " + (getVert(0) / gs).toString() + " " + (mRepopDelay != -1 ? itos(mRepopDelay / 1000) : "");
}


U32 PickupItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, updateMask, stream);
   stream->writeFlag(updateMask & InitialMask);
   stream->writeFlag(mIsVisible || mIsMomentarilyVisible);

   if(mIsMomentarilyVisible)
   {
      mIsMomentarilyVisible = false;
      setMaskBits(PickupMask);
   }

   return retMask;
}


void PickupItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
   bool isInitial = stream->readFlag();
   bool visible = stream->readFlag();

   if(!isInitial && !visible && mIsVisible)
      onClientPickup();
   mIsVisible = visible;
}

// Runs on both client and server, but does nothing on client
bool PickupItem::collide(GameObject *otherObject)
{
   if(mIsVisible && !isGhost() && otherObject->getObjectTypeMask() & (ShipType | RobotType))
   {
      if(pickup(dynamic_cast<Ship *>(otherObject)))
      {
         setMaskBits(PickupMask);
         mRepopTimer.reset(getRepopDelay());
         mIsVisible = false;
      }
   }
   return false;
}


};


