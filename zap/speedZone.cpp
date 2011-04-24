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

#include "speedZone.h"
#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "UI.h"
#include "../glut/glutInclude.h"

namespace Zap
{

// Needed on OS X, but cause link errors in VC++
#ifndef WIN32
const U16 SpeedZone::minSpeed;
const U16 SpeedZone::maxSpeed;
const U16 SpeedZone::defaultSpeed;
#endif

TNL_IMPLEMENT_NETOBJECT(SpeedZone);

// Constructor
SpeedZone::SpeedZone()
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask = SpeedZoneType | CommandMapVisType;
   mSpeed = defaultSpeed;
   mSnapLocation = false;     // Don't snap unless specified
   mRotateSpeed = 0;
   mUnpackInit = 0;    // Some form of counter, to know that it is a rotating speed zone.
}


// Take our basic inputs, pos and dir, and expand them into a three element
// vector (the three points of our triangle graphic), and compute its extent
void SpeedZone::preparePoints()
{
   mPolyBounds = generatePoints(pos, dir);
   computeExtent();
}

Vector<Point> SpeedZone::generatePoints(Point pos, Point dir)
{
   Vector<Point> points;

   Point parallel(dir - pos);
   parallel.normalize();

   Point tip = pos + parallel * SpeedZone::height;
   Point perpendic(pos.y - tip.y, tip.x - pos.x);
   perpendic.normalize();

   const S32 inset = 3;
   const F32 chevronThickness = SpeedZone::height / 3;
   const F32 chevronDepth = SpeedZone::halfWidth - inset;
   
   for(S32 j = 0; j < 2; j++)
   {
      S32 offset = SpeedZone::halfWidth * 2 * j - (j * 4);

      // Red chevron
      points.push_back(pos + parallel * (chevronThickness + offset));                                          
      points.push_back(pos + perpendic * (SpeedZone::halfWidth-2*inset) + parallel * (inset + offset));                                   //  2   3
      points.push_back(pos + perpendic * (SpeedZone::halfWidth-2*inset) + parallel * (chevronThickness + inset + offset));                //    1    4
      points.push_back(pos + parallel * (chevronDepth + chevronThickness + inset + offset));                                              //  6    5
      points.push_back(pos - perpendic * (SpeedZone::halfWidth-2*inset) + parallel * (chevronThickness + inset + offset));
      points.push_back(pos - perpendic * (SpeedZone::halfWidth-2*inset) + parallel * (inset + offset));
   }

   return points;
}


void SpeedZone::render()
{
   renderSpeedZone(mPolyBounds, gClientGame->getCurrentTime());
}

//void SpeedZone::renderEditor()
// Special labeling for speedzones
//if((mSelected && gEditorUserInterface.getEditingSpecialAttrItem() == NONE) || isBeingEdited)
//{
//   glColor((gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSnap)) ? white : inactiveSpecialAttributeColor);
//   UserInterface::drawStringf_2pt(pos, dest, attrSize, 10, "Speed: %d", mSpeed);

//   glColor((gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSpeed)) ? white : inactiveSpecialAttributeColor);
//   UserInterface::drawStringf_2pt(pos, dest, attrSize, -2, "Snapping: %s", boolattr ? "On" : "Off");

//   glColor(white);

//   // TODO: This block should be moved to WorldItem
//   const char *msg, *instr;

//   if(gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::NoAttribute))
//   {
//      msg = "[Enter] to edit speed";
//      instr = "";
//   }
//   else if(gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSpeed))
//   {
//      msg = "[Enter] to edit snapping";
//      instr = "Up/Dn to change speed";
//   }
//   else if(gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::GoFastSnap))
//   {
//      msg = "[Enter] to stop editing";
//      instr = "Up/Dn to toggle snapping";
//   }
//   else
//   {
//      msg = "???";
//      instr = "???";
//   }

//   UserInterface::drawStringf_2pt(pos, dest, instrSize, -22, msg);
//   UserInterface::drawStringf_2pt(pos, dest, instrSize, -22 - instrSize - 2, instr);
//}



// This object should be drawn above polygons
S32 SpeedZone::getRenderSortValue()
{
   return 0;
}


// Create objects from parameters stored in level file
bool SpeedZone::processArguments(S32 argc2, const char **argv2)
{
   S32 argc = 0;
   const char *argv[8];  // 8 is ok, SpeedZone only supports 4 numbered args.
   for(S32 i=0; i<argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      switch(c)
      {
      case 'R': mRotateSpeed = atof(&argv2[i][1]); break;  // using second char to handle number, "R3.4" or "R-1.7"
      case 'S':
         if(strcmp(argv2[i], "SnapEnable"))
            mSnapLocation = true;
         break;
      }
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 8)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   // all string and chars args is processed, numbered arguments left from here down.

   if(argc < 4)      // Need two points at a minimum, with an optional speed item
      return false;

   pos.read(argv);
   pos *= getGame()->getGridSize();

   dir.read(argv + 2);
   dir *= getGame()->getGridSize();

   // Adjust the direction point so that it also represents the tip of the triangle
   Point offset(dir - pos);
   offset.normalize();
   dir = Point(pos + offset * height);

   preparePoints();

   if(argc >= 5)
      mSpeed = max(minSpeed, min(maxSpeed, (U16)(atoi(argv[4]))));

   return true;
}


void SpeedZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


// Bounding box for quick collision-possibility elimination
void SpeedZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);

   setExtent(extent);
}


// More precise boundary for more precise collision detection
bool SpeedZone::getCollisionPoly(Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}


static bool ignoreThisCollision = false;
// Checks collisions with a SpeedZone
bool SpeedZone::collide(GameObject *hitObject)
{
   if(ignoreThisCollision)
      return false;
   // This is run on both server and client side to reduce teleport lag effect.
   if(hitObject->getObjectTypeMask() & (ShipType | RobotType))     // Only ships & robots collide
   {
      MoveObject *s = dynamic_cast<MoveObject *>(hitObject);
      if(!s)
         return false;

      if(isGhost()) // on client, don't process speedZone on all moveObjects except the controlling one.
      {
         ClientGame *client = dynamic_cast<ClientGame *>(getGame());
         if(client)
         {
            GameConnection *gc = client->getConnectionToServer();
            if(gc && gc->getControlObject() != hitObject) 
               return false;
         }
      }
      return true;
   }
   return false;
}

// Handles collisions with a SpeedZone
void SpeedZone::collided(MoveObject *s, U32 stateIndex)
{
   {
      MoveObject::MoveState *moveState = &s->mMoveState[stateIndex];
     // Make sure ship hasn't been excluded
      //for(S32 i = 0; i < mExclusions.size(); i++)
         //if(mExclusions[i].ship == s)
            //return false;
      // Using ship's velosity and position to determine if it should be excluded


      Point impulse = (dir - pos);
      impulse.normalize(mSpeed);
      Point shipNormal = moveState->vel;
      shipNormal.normalize(mSpeed);
      F32 angleSpeed = mSpeed * 0.5f;

         // using mUnpackInit, as client does not know that mRotateSpeed is not zero.
      if(mSnapLocation && mRotateSpeed == 0 && mUnpackInit < 3)
         angleSpeed *= 0.01f;
      if(shipNormal.distanceTo(impulse) < angleSpeed && moveState->vel.len() > mSpeed)
         return;

      // This following line will cause ships entering the speedzone to have their location set to the same point
      // within the zone so that their path out will be very predictable.
      if(mSnapLocation)
      {
         //s->setActualPos(pos, false);

         Point diffpos = moveState->pos - pos;
         Point thisAngle = dir - pos;
         thisAngle.normalize();
         Point newPos = thisAngle * diffpos.dot(thisAngle) + pos + impulse * 0.001;
         //s->setActualPos(newpos, false);

         Point oldVel = moveState->vel;
         Point oldPos = moveState->pos;

         ignoreThisCollision = true;
         moveState->vel = newPos - moveState->pos;
         s->move(1, stateIndex, false);
         ignoreThisCollision = false;

         if(s->getActualPos().distSquared(newPos) > 1)  // make sure we can get to the position without going through walls.
         {
            moveState->pos = oldPos;
            moveState->vel = oldVel;
            return;
         }
         //s->mImpulseVector = impulse * 1.5;     // <-- why???
         moveState->vel = impulse * 1.5;
         //moveState->pos = newPos;

      }
      else
      {
         if(shipNormal.distanceTo(impulse) < mSpeed && moveState->vel.len() > mSpeed * 0.8)
            return;
         //s->mImpulseVector = impulse * 1.5;     // <-- why???
         moveState->vel += impulse * 1.5;
      }



      // To ensure we don't give multiple impulses to the same ship, we'll exclude it from
      // further action for about 300ms.  That should do the trick.

      //Exclusion exclusion;
      //exclusion.ship = s;
      //exclusion.time = getGame()->getCurrentTime() + 300;

      //mExclusions.push_back(exclusion);

      if(!s->isGhost() && stateIndex == MoveObject::ActualState) // only server needs to send information.
      {
         setMaskBits(HitMask);
         if(s->getControllingClient().isValid())
            // Trigger a sound on the player's machine: They're going to be so far away they'll never hear the sound emitted by the gofast itself...
            s->getControllingClient()->s2cDisplayMessage(0, SFXGoFastInside, "");
      }
   }

}


void SpeedZone::idle(GameObject::IdleCallPath path)
{
   // Check for old exclusions that no longer apply
   //for(S32 i = mExclusions.size()-1; i >= 0; i--)
      //if(mExclusions[i].time < getGame()->getCurrentTime())     // Exclusion has expired
         //mExclusions.erase_fast(i);
   if(mRotateSpeed != 0)
   {
      dir -= pos;
      F32 angle = dir.ATAN2();
      angle += mRotateSpeed * mCurrentMove.time * 0.001;
      dir.setPolar(1, angle);
      dir += pos;
      setMaskBits(InitMask);
      preparePoints();     // avoids off center problem..
   }
}


U32 SpeedZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   if(stream->writeFlag(updateMask & InitMask))    
   {
      stream->write(pos.x);
      stream->write(pos.y);

      stream->write(dir.x);
      stream->write(dir.y);

      stream->writeInt(mSpeed, 16);
      stream->writeFlag(mSnapLocation);
   }      

   return 0;
}


void SpeedZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   if(stream->readFlag())
   {
      mUnpackInit++;
      stream->read(&pos.x);
      stream->read(&pos.y);

      stream->read(&dir.x);
      stream->read(&dir.y);

      mSpeed = stream->readInt(16);
      mSnapLocation = stream->readFlag();

      preparePoints();
   }
   else 
      SFXObject::play(SFXGoFastOutside, pos, pos);
}

};


