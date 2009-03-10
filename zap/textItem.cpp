//-----------------------------------------------------------------------------------
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

#include "textItem.h"
#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "UI.h"
#include "../glut/glutInclude.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(TextItem);

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
   return -1;
}


// Create objects from parameters stored in level file
// Entry looks like: TextItem 0 50 10 10 11 11 Message goes here
void TextItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 7)
      return;

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

   // Correction for the Bitfighter logo
   if(mText == "Bitfighter")
   {
      F32 adjFact = 48 * getGame()->getGridSize() / 255;
      pos.x -= adjFact;
      dir.x -= adjFact;
   }

   computeExtent();
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

   Point c1 = Point(pos.x - buf * sinang, (pos.y + mSize) + buf * cosang);
   Point c2 = Point(pos.x + mSize * sinang, (pos.y + mSize) - mSize * cosang);
   Point c3 = Point(pos.x + len * cosang + mSize * sinang, (pos.y + mSize) + len * sinang - mSize * cosang);
   Point c4 = Point(pos.x + len * cosang - buf * sinang, (pos.y + mSize) + len * sinang + buf * cosang);

   F32 minx = min(c1.x, min(c2.x, min(c3.x, c4.x)));
   F32 miny = min(c1.y, min(c2.y, min(c3.y, c4.y)));
   F32 maxx = max(c1.x, max(c2.x, max(c3.x, c4.x)));
   F32 maxy = max(c1.y, max(c2.y, max(c3.y, c4.y)));

   Rect extent(Point(minx, miny), Point(maxx, maxy));

   setExtent(extent);
}

bool TextItem::getCollisionPoly(U32 state, Vector<Point> &polyPoints)
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


};

