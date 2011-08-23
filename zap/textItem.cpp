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
#include "gameObjectRender.h"    // For renderPointVector()
#include "game.h"
#include "item.h"
#include "stringUtils.h"

#ifndef ZAP_DEDICATED
#include "ClientGame.h"
#include "UIEditorMenus.h"       // For TextItemEditorAttributeMenuUI def
#include "SDL/SDL_opengl.h"
#endif



#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(TextItem);


#ifndef ZAP_DEDICATED
EditorAttributeMenuUI *TextItem::mAttributeMenuUI = NULL;
#endif

// Constructor
TextItem::TextItem()
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = TextItemTypeNumber;

   // Some default values
   mSize = 20;
   mTeam = MoveItem::TEAM_NEUTRAL;
}


// Destructor
TextItem::~TextItem()
{
   // Do nothing
}


TextItem *TextItem::clone() const
{
   return new TextItem(*this);

   //copyAttrs(clone);

   //return clone;
}

//
//void TextItem::copyAttrs(TextItem *target)
//{
//   SimpleLine::copyAttrs(target);
//
//   target->mSize = mSize;
//   target->mText = mText;
//}


void TextItem::newObjectFromDock(F32 gridSize)
{
   Parent::newObjectFromDock(gridSize);

   mText = "Your text here";
   recalcTextSize();
}


// In game rendering
void TextItem::render()
{
#ifndef ZAP_DEDICATED
   ClientGame *game = dynamic_cast<ClientGame *>(getGame());
   
   // Don't render opposing team's text items
   if(!game || !game->getConnectionToServer() || !game->getGameType())
      return;

   Ship *ship = dynamic_cast<Ship *>(game->getConnectionToServer()->getControlObject());
   if( (!ship && mTeam != -1) || (ship && ship->getTeam() != mTeam && mTeam != -1) )
      return;

   renderTextItem(getVert(0), getVert(1), mSize, mText, game->getTeamColor(mTeam));
#endif
}


// Called by SimpleItem::renderEditor()
void TextItem::renderEditorItem()
{
   renderTextItem(getVert(0), getVert(1), mSize, mText, getGame()->getTeamColor(mTeam));
}


// This object should be drawn below others
S32 TextItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: TextItem 0 50 10 10 11 11 Message goes here
bool TextItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 7)
      return false;

   mTeam = atoi(argv[0]);

   Point pos, dir;

   pos.read(argv + 1);
   pos *= game->getGridSize();

   dir.read(argv + 3);
   dir *= game->getGridSize();

   setVert(pos, 0);
   setVert(dir, 1);

   mSize = (F32)atof(argv[5]);
   mSize = max(min(mSize, (F32)MAX_TEXT_SIZE), (F32)MIN_TEXT_SIZE);      // Note that same line exists below, in recalcXXX()... combine?

   // Assemble any remainin args into a string
   mText = "";
   for(S32 i = 6; i < argc; i++)
   {
      mText += argv[i];
      if(i < argc - 1)
         mText += " ";
   }

   computeExtent();

   return true;
}


string TextItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize) + " " + ftos(mSize, 3) + " " + mText;
}


// Editor
void TextItem::recalcTextSize()
{
#ifndef ZAP_DEDICATED
   const F32 dummyTextSize = 120;

   F32 lineLen = getVert(0).distanceTo(getVert(1));      // In in-game units
   F32 strWidth = F32(UserInterface::getStringWidth(dummyTextSize, mText.c_str())) / dummyTextSize; 
   F32 size = lineLen / strWidth;

   // Compute text size subject to min and max defined in TextItem
   mSize = max(min(size, (F32)MAX_TEXT_SIZE), (F32)MIN_TEXT_SIZE);
#endif
}


void TextItem::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void TextItem::computeExtent()
{
#ifdef ZAP_DEDICATED
   // don't care much about it in server, as server won't render, and does not do colliding to textTiem
	setExtent(Rect(getVert(0), getVert(1)));
#else
   F32 len = UserInterface::getStringWidth(mSize, mText.c_str());
   //F32 buf = mSize / 2;     // Provides some room to accomodate descenders on letters like j and g.

   F32 angle =  getVert(0).angleTo(getVert(1));
   F32 sinang = sin(angle);
   F32 cosang = cos(angle);

   F32 descenderFactor = .35f;    // To account for y, g, j, etc.
   F32 h = mSize * (1 + descenderFactor);
   F32 w = len * 1.05f;           // 1.05 adds just a little horizontal padding for certain words with trailing ys or other letters that are just a tiny bit longer than calculated
   F32 x = getVert(0).x + mSize * descenderFactor * sinang;
   F32 y = getVert(0).y + mSize * descenderFactor * cosang;

   F32 c1x = x - h * sinang * .5f;
   F32 c1y = y;

   F32 c2x = x + w * cosang - h * sinang * .5f;
   F32 c2y = y + w * sinang;

   F32 c3x = x + h * sinang * .5f + w * cosang;
   F32 c3y = y - h * cosang + w * sinang;

   F32 c4x = x + h * sinang * .5f;
   F32 c4y = y - h * cosang;

   F32 minx = min(c1x, min(c2x, min(c3x, c4x)));
   F32 miny = min(c1y, min(c2y, min(c3y, c4y)));
   F32 maxx = max(c1x, max(c2x, max(c3x, c4x)));
   F32 maxy = max(c1y, max(c2y, max(c3y, c4y)));

   Rect extent(Point(minx, miny), Point(maxx, maxy));

   setExtent(extent);
#endif
}


bool TextItem::getCollisionPoly(Vector<Point> &polyPoints) const
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
   Point pos = getVert(0);
   Point dir = getVert(1);

   pos.write(stream);
   dir.write(stream);

   stream->writeRangedU32((U32)mSize, 0, MAX_TEXT_SIZE);
   writeThisTeam(stream);

   stream->writeString(mText.c_str(), (U8) mText.length());      // Safe to cast text.length to U8 because we've limited it's length to MAX_TEXTITEM_LEN

   return 0;
}


void TextItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   char txt[MAX_TEXTITEM_LEN];

   Point pos, dir;

   pos.read(stream);
   dir.read(stream);

   setVert(pos, 0);
   setVert(dir, 1);

   mSize = (F32)stream->readRangedU32(0, MAX_TEXT_SIZE);
   readThisTeam(stream);

   stream->readString(txt);

   mText = txt;
   computeExtent();
}


///// Editor Methods

// Runs when text is being changed in the editor
void TextItem::onAttrsChanging()
{
   onGeomChanged();
}

void TextItem::onAttrsChanged()
{
   onGeomChanged();
}

void TextItem::onGeomChanging()
{
   onGeomChanged();
}

void TextItem::onGeomChanged()
{
   recalcTextSize();
}


#ifndef ZAP_DEDICATED
// Static method: Provide hook into the object currently being edited with the attrubute editor for callback purposes
EditorObject *TextItem::getAttributeEditorObject()     
{
   return mAttributeMenuUI->getObject(); 
}


EditorAttributeMenuUI *TextItem::getAttributeMenu(ClientGame *game)
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
      mAttributeMenuUI = new TextItemEditorAttributeMenuUI(game);

   // Udate the editor with attributes from our current object
   mAttributeMenuUI->menuItems[0]->setValue(mText);

   return mAttributeMenuUI;
}
#endif


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(LineItem);

// Why does GCC need this for signed int?
#ifndef TNL_OS_WIN32
const S32 LineItem::MIN_LINE_WIDTH;
const S32 LineItem::MAX_LINE_WIDTH;
#endif

// Constructor
LineItem::LineItem()
{ 
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = LineTypeNumber;
}


LineItem *LineItem::clone() const
{
   return new LineItem(*this);
}

//
//// Copy constructor -- make sure each copy gets its own geometry object
//LineItem::LineItem(const LineItem &li)
//{
//   mGeometry = boost::shared_ptr<Geometry>(new PolylineGeometry(*((PolylineGeometry *)li.mGeometry.get())));  
//}
//

void LineItem::render()
{
#ifndef ZAP_DEDICATED
   GameConnection *gc = dynamic_cast<ClientGame *>(getGame())->getConnectionToServer();
   // Don't render opposing team's text items
   if(!gc)      // Not sure if this is really needed...
      return;

   Ship *ship = dynamic_cast<Ship *>(gc->getControlObject());
   if( (!ship && mTeam != -1) || (ship && ship->getTeam() != mTeam && mTeam != -1) )
      return;

   glColor(getGame()->getTeamColor(mTeam));
   renderPointVector(getOutline(), GL_LINE_STRIP);
#endif
}


void LineItem::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   if(!mSelected)
      glColor(getEditorRenderColor());
   else
      glColor(SELECT_COLOR);

   renderPointVector(getOutline(), GL_LINE_STRIP);

   renderLinePolyVertices(currentScale);
#endif
}


const Color *LineItem::getEditorRenderColor() const 
{ 
   return getGame()->getTeamColor(mTeam); 
}


// This object should be drawn below others
S32 LineItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: LineItem 0 50 10 10 11 11 Message goes here
bool LineItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 6)
      return false;

   mTeam = atoi(argv[0]);
   setWidth(atoi(argv[1]));
   readGeom(argc, argv, 2, game->getGridSize());

   computeExtent();

   return true;
}


string LineItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + itos(getWidth()) + " " + geomToString(gridSize);
}


void LineItem::onAddedToGame(Game *game)
{
   Parent::onAddedToGame(game);

   if(!isGhost())
      setScopeAlways();
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void LineItem::computeExtent()
{
   setExtent();
}


bool LineItem::getCollisionPoly(Vector<Point> &polyPoints) const
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
   writeThisTeam(stream);

   packGeom(connection, stream);

   return 0;
}


void LineItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //mWidth = stream->readRangedU32(0, MAX_LINE_WIDTH);
   readThisTeam(stream);

   unpackGeom(connection, stream);
   setExtent();
}


void LineItem::setWidth(S32 width, S32 min, S32 max)
{
   // Bounds check
   if(width < min)
      width = min;
   else if(width > max)
      width = max; 

   mWidth = width; 
}


void LineItem::setWidth(S32 width) 
{         
   setWidth(width, LineItem::MIN_LINE_WIDTH, LineItem::MAX_LINE_WIDTH);
}


void LineItem::changeWidth(S32 amt)
{
   S32 width = mWidth;

   if(amt > 0)
      width += amt - (S32) width % amt;    // Handles rounding
   else
   {
      amt *= -1;
      width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing
   }

   setWidth(width);
   onGeomChanged();
}


};
