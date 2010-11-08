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

#include "glutInclude.h"

namespace Zap
{

   // Constructor
Item::Item(Point p, bool collideable, float radius, float mass) : MoveObject(p, radius, mass)
{
   mIsMounted = false;
   mIsCollideable = collideable;
   mObjectTypeMask = MoveableType | ItemType | CommandMapVisType;
   mInitial = false;
}

bool Item::processArguments(S32 argc, const char **argv)
{
   if(argc < 2)
      return false;

   Point pos;
   pos.read(argv);
   pos *= getGame()->getGridSize();
   for(U32 i = 0; i < MoveStateCount; i++)
      mMoveState[i].pos = pos;

   updateExtent();

   return true;
}


void Item::render()
{
   // if the item is mounted, renderItem will be called from the
   // ship it is mounted to
   if(mIsMounted)
      return;

   renderItem(mMoveState[RenderState].pos);
}


// Runs on both client and server, comes from collision() on the server and the colliding client, and from
// unpackUpdate() in the case of all clients
void Item::mountToShip(Ship *theShip)     // theShip could be NULL here
{
   TNLAssert(isGhost() || isInDatabase(), "Error, mount item not in database.");

   dismount();
   mMount = theShip;
   if(theShip)
   {
      theShip->mMountedItems.push_back(this);
      mIsMounted = true;
   }
   setMaskBits(MountMask);
}


void Item::onMountDestroyed()
{
   dismount();
}


void Item::dismount()
{
   if(mMount.isValid())
   {
      for(S32 i = 0; i < mMount->mMountedItems.size(); i++)
         if(mMount->mMountedItems[i].getPointer() == this)
         {
            mMount->mMountedItems.erase(i);     // Remove mounted item from our mount's list of mounted things
            break;
         }
   }
   
   if(isGhost())     // Client only
      onItemDropped();

   mMount = NULL;
   mIsMounted = false;
   setMaskBits(MountMask);
}


void Item::setActualPos(Point p)
{
   mMoveState[ActualState].pos = p;
   mMoveState[ActualState].vel.set(0,0);
   setMaskBits(WarpPositionMask | PositionMask);
}


void Item::setActualVel(Point vel)
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
         if(!isGhost())
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
         if(mMoveState[ActualState].vel.len() > 0.001f)
            setMaskBits(PositionMask);

         mMoveState[RenderState] = mMoveState[ActualState];

      }
      else
         updateInterpolation();
   }
   updateExtent();
}

U32 Item::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   U32 retMask = 0;
   if(stream->writeFlag(updateMask & InitialMask))
   {
      // Do nothing
   }
   if(stream->writeFlag(updateMask & PositionMask))
   {
      ((GameConnection *) connection)->writeCompressedPoint(mMoveState[ActualState].pos, stream);
      writeCompressedVelocity(mMoveState[ActualState].vel, 511, stream);      // 511? Why?
      stream->writeFlag(updateMask & WarpPositionMask);
   }
   if(stream->writeFlag(updateMask & MountMask) && stream->writeFlag(mIsMounted))   // True for mask, then True if mounted
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
       // Do nothing
   }

   if(stream->readFlag())     // PositionMask
   {
      ((GameConnection *) connection)->readCompressedPoint(mMoveState[ActualState].pos, stream);
      readCompressedVelocity(mMoveState[ActualState].vel, 511, stream);    // 511?  What? Why?
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

////////////////////////////////////////
////////////////////////////////////////

PickupItem::PickupItem(Point p, float radius, S32 repopDelay) : Item(p, false, radius, 1)
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


bool PickupItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 2)
      return false;
   else if(!Parent::processArguments(argc, argv))
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


