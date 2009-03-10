//----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "BotNavMeshZone.h"
#include "SweptEllipsoid.h"
#include "Robot.h"
#include "UIMenus.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(BotNavMeshZone);

extern S32 gMaxPolygonPoints;


Vector<SafePtr<BotNavMeshZone> > gBotNavMeshZones;     // List of all our zones


// Constructor
BotNavMeshZone::BotNavMeshZone()
{
   mObjectTypeMask = BotNavMeshZoneType | CommandMapVisType;
   mNetFlags.set(Ghostable);    // For now, so we can see them on the client to help with debugging
   mZoneID = gBotNavMeshZones.size();
   gBotNavMeshZones.push_back(this);
}

// Destructor
BotNavMeshZone::~BotNavMeshZone()
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   if(gBotNavMeshZones[i] == this)
   {
      gBotNavMeshZones.erase(i);
      break;
   }
}

// Return the center of this zone
Point BotNavMeshZone::getCenter()
{
   return getExtent().getCenter();     // Good enough for government work
}

void BotNavMeshZone::render()     // For now... in future will be invisible!
{
   glEnable(GL_BLEND);
      F32 alpha = 0.5;
   glColor3f(.2, .2, 0);

   // Render loadout zone trinagle geometry
   for(S32 i = 0; i < mPolyFill.size(); i+=3)
   {
      glBegin(GL_POLYGON);
      for(S32 j = i; j < i + 3; j++)
         glVertex2f(mPolyFill[j].x, mPolyFill[j].y);
      glEnd();
   }

      glColor3f(.7, .7, 0);
   glBegin(GL_LINE_LOOP);
      for(S32 i = 0; i < mPolyBounds.size(); i++)
         glVertex2f(mPolyBounds[i].x, mPolyBounds[i].y);
   glEnd();

   Rect extents = this->getExtent();
   Point center = extents.getCenter();

   glPushMatrix();
   glTranslatef(center.x, center.y, 0);
      glColor3f(1,1,0);
      char buf[24];
      dSprintf(buf, 24, "ZONE %d/%d", mNeighbors.size(), mZoneID);
      renderCenteredString(Point(0,0), 25, buf);
   glPopMatrix();




      glColor3f(1,0,0);
      for(S32 i = 0; i < mNeighbors.size(); i++)
      {
         glBegin(GL_LINES);
         glVertex2f(mNeighbors[i].borderStart.x, mNeighbors[i].borderStart.y);
         glVertex2f(mNeighbors[i].borderEnd.x, mNeighbors[i].borderEnd.y);
         glEnd();
      }

  
      glDisable(GL_BLEND);
}



// Use this to help keep track of which robots are where
// Only gets run on the server, never on client, obviously, because that's where the bots are!!!
bool BotNavMeshZone::collide(GameObject *hitObject)
{
   if(hitObject->getObjectTypeMask() & RobotType)     // Only care about robots...
   {
      Robot *r = (Robot *) hitObject;
      r->setCurrentZone(mZoneID);
   }
   return false;
}


S32 BotNavMeshZone::getRenderSortValue()
{
   return -2;
}

// Create objects from parameters stored in level file
void BotNavMeshZone::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return;

   processPolyBounds(argc, argv, 0, mPolyBounds);
   computeExtent();  // Not needed?
}


void BotNavMeshZone::onAddedToGame(Game *theGame)
{
   if(!isGhost())     // For now, so we can see them on the client to help with debugging
      setScopeAlways();

   // Don't need to increment our objectloaded counter, as this object resides only on the server
}

// Bounding box for quick collision-possibility elimination
void BotNavMeshZone::computeExtent()
{
   Rect extent(mPolyBounds[0], mPolyBounds[0]);
   for(S32 i = 1; i < mPolyBounds.size(); i++)
      extent.unionPoint(mPolyBounds[i]);
   setExtent(extent);
}

// More precise boundary for precise collision detection
bool BotNavMeshZone::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
{
   for(S32 i = 0; i < mPolyBounds.size(); i++)
      polyPoints.push_back(mPolyBounds[i]);
   return true;
}


   // These methods will be empty later...
 U32 BotNavMeshZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->writeInt(mZoneID, 16);
   stream->writeEnum(mPolyBounds.size(), gMaxPolygonPoints);
   for(S32 i = 0; i < mPolyBounds.size(); i++)
   {
      stream->write(mPolyBounds[i].x);
      stream->write(mPolyBounds[i].y);
   }

   stream->writeEnum(mNeighbors.size(), 15);
   for(S32 i = 0; i < mNeighbors.size(); i++)
   {
      stream->write(mNeighbors[i].borderStart.x);
      stream->write(mNeighbors[i].borderStart.y);

      stream->write(mNeighbors[i].borderEnd.x);
      stream->write(mNeighbors[i].borderEnd.y);
   }

   return 0;
}

void BotNavMeshZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   mZoneID = stream->readInt(16);
   U32 size = stream->readEnum(gMaxPolygonPoints);
   for(U32 i = 0; i < size; i++)
   {
      Point p;
      stream->read(&p.x);
      stream->read(&p.y);
      mPolyBounds.push_back(p);
   }
   if(size)
   {
      computeExtent();
      Triangulate::Process(mPolyBounds, mPolyFill);
   }


   size = stream->readEnum(15);
   for(U32 i = 0; i < size; i++)
   {
      NeighboringZone n;
      Point p1;
      stream->read(&p1.x);
      stream->read(&p1.y);
      Point p2;
      stream->read(&p2.x);
      stream->read(&p2.y);
      n.borderStart = p1;
      n.borderEnd = p2;
      mNeighbors.push_back(n);
   }

}

extern bool PolygonContains(const Point *inVertices, int inNumVertices, const Point &inPoint);

S32 findZoneContaining(Point p)
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!

      if( gBotNavMeshZones[i]->getExtent().contains(p) 
                        && (PolygonContains(gBotNavMeshZones[i]->mPolyBounds.address(), gBotNavMeshZones[i]->mPolyBounds.size(), p)) )
         return i;
   }
   return -1;
}


extern bool segmentsCoincident(Point p1, Point p2, Point p3, Point p4);


void buildBotNavMeshZoneConnections()
{
   for(S32 i = 0; i < gBotNavMeshZones.size(); i++)
   {
      for(S32 j = i; j < gBotNavMeshZones.size(); j++)
      {
         if(i == j)
            continue;      // Don't check self...

         // Do zones i and j touch?  First a quick and dirty bounds check:
         if(!gBotNavMeshZones[i]->getExtent().intersectsOrBorders(gBotNavMeshZones[j]->getExtent()))
            continue;

         logprintf("+++++++Candidate: %d - %d", i, j);

         // Check for unlikely but fatal situation: Not enough vertices
          if(gBotNavMeshZones[i]->mPolyBounds.size() < 3 || gBotNavMeshZones[j]->mPolyBounds.size() < 3)
          {
             logprintf("Too few pts!");
            continue;
          }

         Point pi1, pi2, pj1, pj2;
         bool found = false;

         // Now, do we actually touch?  Let's look, segment by segment
         for(S32 ii = 0; !found && ii < gBotNavMeshZones[i]->mPolyBounds.size(); ii++)
         {
            pi1 = gBotNavMeshZones[i]->mPolyBounds[ii];
            if(ii == gBotNavMeshZones[i]->mPolyBounds.size() - 1)
               pi2 = gBotNavMeshZones[i]->mPolyBounds[0];
            else
               pi2 = gBotNavMeshZones[i]->mPolyBounds[ii + 1];

            for(S32 jj = 0; !found && jj < gBotNavMeshZones[j]->mPolyBounds.size(); jj++)
            {
               pj1 = gBotNavMeshZones[j]->mPolyBounds[jj];
               if(jj == gBotNavMeshZones[j]->mPolyBounds.size() - 1)
                  pj2 = gBotNavMeshZones[j]->mPolyBounds[0];
               else
                  pj2 = gBotNavMeshZones[j]->mPolyBounds[jj + 1];

               Point bordStart, bordEnd;

               if(segsOverlap(pi1, pi2, pj1, pj2, bordStart, bordEnd))
               {
                  Rect r(bordStart, bordEnd);      // Convience for computing center of border area

                  NeighboringZone n1;
                  n1.zoneID = j;
                  n1.borderStart = bordStart;
                  n1.borderEnd = bordEnd;
                  n1.distTo = gBotNavMeshZones[i]->getExtent().getCenter().distanceTo(r.getCenter());     // Whew!
                  gBotNavMeshZones[i]->mNeighbors.push_back(n1);

                  NeighboringZone n2;
                  n2.zoneID = i;
                  n2.borderStart = bordStart;
                  n2.borderEnd = bordEnd;
                  n2.distTo = gBotNavMeshZones[j]->getExtent().getCenter().distanceTo(r.getCenter());     
                  gBotNavMeshZones[j]->mNeighbors.push_back(n2);

                  logprintf("====================== %d Zones touch: %d - %d",gBotNavMeshZones[i]->mNeighbors.size(), i, j);
                  found = true;
               }
            }
         }
      }
   }
   
}


};

