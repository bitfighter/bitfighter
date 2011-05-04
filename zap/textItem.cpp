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

#include "textItem.h"
#include "gameNetInterface.h"
#include "glutInclude.h"

#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(TextItem);

// RDW - These need storage, I believe VC++ pukes on this,
// so I'm #ifdeffing them.
#ifndef TNL_OS_WIN32
const U32 TextItem::MAX_TEXT_SIZE;
const U32 TextItem::MIN_TEXT_SIZE;
#endif

// Constructor
TextItem::TextItem()
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= CommandMapVisType;     // Or not?
}


void TextItem::render()
{
   renderTextItem(pos, dir, mSize, mTeam, mText);
}

// This object should be drawn below others
S32 TextItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: TextItem 0 50 10 10 11 11 Message goes here
bool TextItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 7)
      return false;

   mTeam = atoi(argv[0]);

   pos.read(argv + 1);
   pos *= getGame()->getGridSize();

   dir.read(argv + 3);
   dir *= getGame()->getGridSize();

   mSize = atoi(argv[5]);
   if (mSize > MAX_TEXT_SIZE)
      mSize = MAX_TEXT_SIZE;
   else if(mSize < MIN_TEXT_SIZE)
      mSize = MIN_TEXT_SIZE;

   for(S32 i = 6; i < argc; i++)
   {
      mText += argv[i];
      if (i < argc - 1)
         mText += " ";
   }

   mText = mText.substr(0, MAX_TEXTITEM_LEN-1);      // Limit length to MAX_TEXTITEM_LEN chars (leaving room for a trailing null)


   computeExtent();

   return true;
}


void TextItem::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void TextItem::computeExtent()
{
   U32 len = UserInterface::getStringWidth(mSize, mText.c_str());
   U32 buf = mSize / 2;     // Provides some room to accomodate descenders on letters like j and g.

   F32 angle =  pos.angleTo(dir);
   F32 sinang = sin(angle);
   F32 cosang = cos(angle);

   F32 descenderFactor = .35;    // To account for y, g, j, etc.
   F32 h = mSize * (1 + descenderFactor);
   F32 w = len * 1.05;           // 1.05 adds just a little horizontal padding for certain words with trailing ys or other letters that are just a tiny bit longer than calculated
   F32 x = pos.x + mSize * descenderFactor * sinang;
   F32 y = pos.y + mSize * descenderFactor * cosang;

   F32 c1x = x - h * sinang * .5;
   F32 c1y = y;

   F32 c2x = x + w * cosang - h * sinang * .5;
   F32 c2y = y + w * sinang;

   F32 c3x = x + h * sinang * .5 + w * cosang;
   F32 c3y = y - h * cosang + w * sinang;

   F32 c4x = x + h * sinang * .5;
   F32 c4y = y - h * cosang;

   F32 minx = min(c1x, min(c2x, min(c3x, c4x)));
   F32 miny = min(c1y, min(c2y, min(c3y, c4y)));
   F32 maxx = max(c1x, max(c2x, max(c3x, c4x)));
   F32 maxy = max(c1y, max(c2y, max(c3y, c4y)));

   Rect extent(Point(minx, miny), Point(maxx, maxy));

   setExtent(extent);
}


bool TextItem::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}

// Handle collisions with a TextItem.  Easy, there are none.
bool TextItem::collide(GameObject *hitObject)
{
   return false;
}

void TextItem::idle(GameObject::IdleCallPath path)
{
   // Laze about, read a book, take a nap, whatever.
}

U32 TextItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->write(pos.x);
   stream->write(pos.y);

   stream->write(dir.x);
   stream->write(dir.y);

   stream->writeRangedU32(mSize, 0, MAX_TEXT_SIZE);
   stream->write(mTeam);

   stream->writeString(mText.c_str(), (U8) mText.length());      // Safe to cast text.length to U8 because we've limited it's length to MAX_TEXTITEM_LEN

   return 0;
}

void TextItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   char txt[MAX_TEXTITEM_LEN];

   stream->read(&pos.x);
   stream->read(&pos.y);

   stream->read(&dir.x);
   stream->read(&dir.y);

   mSize = stream->readRangedU32(0, MAX_TEXT_SIZE);
   stream->read(&mTeam);

   stream->readString(txt);

   mText = txt;
   computeExtent();
}

////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(LineItem);

// Constructor
LineItem::LineItem()
{ 
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= CommandMapVisType;
}


void LineItem::render()
{
   // Don't render opposing team's text items
   if(!gClientGame || !gClientGame->getConnectionToServer())      // Not sure if this is really needed...
      return;

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if( (!ship && mTeam != -1) || (ship && ship->getTeam() != mTeam && mTeam != -1) )
      return;

   GameType *gt = gClientGame->getGameType();
   TNLAssert(gt, "Invalid gameType in LineItem::render()!");

   glColor(gt->getTeamColor(mTeam));

   //TNLAssert(mRenderPoints.size() % 4 == 0, "Invalid number of points in mRenderPoints vector!");

   glBegin(GL_LINE_STRIP); 
      for(S32 i = 0; i < mPolyBounds.size(); i++)
         glVertex(mPolyBounds[i]);
   glEnd();
}

// This object should be drawn below others
S32 LineItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: LineItem 0 50 10 10 11 11 Message goes here
bool LineItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return false;

   mTeam = atoi(argv[0]);
   mWidth = max(min(atoi(argv[1]), static_cast<int>(MAX_LINE_WIDTH)), static_cast<int>(MIN_LINE_WIDTH));
   //mWidth = min(atoi(argv[1]), static_cast<int>(MAX_LINE_WIDTH));
   processPolyBounds(argc, argv, 2, getGame()->getGridSize());

   computeExtent();

   return true;
}


void LineItem::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void LineItem::computeExtent()
{
   setExtent(computePolyExtents());
}


bool LineItem::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}


// Handle collisions with a LineItem.  Easy, there are none.
bool LineItem::collide(GameObject *hitObject)
{
   return false;
}


void LineItem::idle(GameObject::IdleCallPath path)
{
   // Do nothing
}


U32 LineItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //stream->writeRangedU32(mWidth, 0, MAX_LINE_WIDTH);
   stream->write(mTeam);

   Polyline::packUpdate(connection, stream);

   return 0;
}


void LineItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //mWidth = stream->readRangedU32(0, MAX_LINE_WIDTH);
   stream->read(&mTeam);

   if(!Polyline::unpackUpdate(connection, stream))
      return;

   setExtent(computePolyExtents());
}


};


