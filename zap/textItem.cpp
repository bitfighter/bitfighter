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
#include "gameObjectRender.h"    // For renderTextItem()
#include "game.h"
#include "item.h"
#include "stringUtils.h"
#include "teamInfo.h"            // For TEAM_NEUTRAL

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "UIEditorMenus.h"     // For TextItemEditorAttributeMenuUI def
#  include "OpenglUtils.h"
#endif


#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(TextItem);


#ifndef ZAP_DEDICATED
EditorAttributeMenuUI *TextItem::mAttributeMenuUI = NULL;
#endif

// Combined Lua / C++ constructor
TextItem::TextItem(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = TextItemTypeNumber;

   // Some default values
   mSize = 20;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
TextItem::~TextItem()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


TextItem *TextItem::clone() const
{
   return new TextItem(*this);
}


const char *TextItem::getInstructionMsg()
{
   return "[Enter] to edit text";
}


void TextItem::newObjectFromDock(F32 gridSize)
{
   mText = "Your text here";

   Parent::newObjectFromDock(gridSize);

   recalcTextSize();    // Has to be after Parent::newObjectFromDock(gridSize); that sets the length of the line, which changes text size
}


// In game rendering
void TextItem::render()
{
#ifndef ZAP_DEDICATED
   ClientGame *game = static_cast<ClientGame *>(getGame());
   
   // Don't render opposing team's text items if we are in a game... but in editor preview mode, where
   // we don't have a connection to the server, text will be rendered normally
   if(game->getConnectionToServer())
   {
      Ship *ship = dynamic_cast<Ship *>(game->getConnectionToServer()->getControlObject());
      if( (!ship && getTeam() != TEAM_NEUTRAL) || (ship && ship->getTeam() != getTeam() && getTeam() != TEAM_NEUTRAL) )
         return;
   }

   renderEditorItem();
#endif
}


// Called by SimpleItem::renderEditor()
void TextItem::renderEditorItem()
{
   renderTextItem(getVert(0), getVert(1), mSize, mText, getColor());
}


const char *TextItem::getOnScreenName()     { return "Text";      }
const char *TextItem::getOnDockName()       { return "TextItem";  }
const char *TextItem::getPrettyNamePlural() { return "TextItems"; }
const char *TextItem::getEditorHelpString() { return "Draws a bit of text on the map.  Visible only to team, or to all if neutral."; }


bool TextItem::hasTeam()      { return true; }
bool TextItem::canBeHostile() { return true; }
bool TextItem::canBeNeutral() { return true; }


Color TextItem::getEditorRenderColor()
{
   return Colors::blue;
}


F32 TextItem::getSize()
{
   return mSize;
}


// Set text size subject to min and max defined in TextItem
void TextItem::setSize(F32 desiredSize)
{
   mSize = max(min(desiredSize, (F32)MAX_TEXT_SIZE), (F32)MIN_TEXT_SIZE);
}


string TextItem::getText()
{
   return mText;
}


void TextItem::setText(string text)
{
   mText = text;
   onGeomChanged();
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

   setTeam(atoi(argv[0]));

   Point pos, dir;

   pos.read(argv + 1);
   pos *= game->getGridSize();

   dir.read(argv + 3);
   dir *= game->getGridSize();

   setSize((F32)atof(argv[5]));

   // Assemble any remainin args into a string
   mText = "";
   for(S32 i = 6; i < argc; i++)
   {
      mText += argv[i];
      if(i < argc - 1)
         mText += " ";
   }

   setGeom(pos, dir);

   return true;
}


void TextItem::setGeom(const Vector<Point> &points)
{
   if(points.size() >= 2)
      setGeom(points[0], points[1]);
}


void TextItem::setGeom(const Point &pos, const Point &dest)
{
   setVert(pos, 0);
   setVert(dest, 1);

   updateExtentInDatabase();
}


string TextItem::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(getTeam()) + " " + geomToString(gridSize) + " " + ftos(mSize, 3) + " " + writeLevelString(mText.c_str());
}


// Editor
void TextItem::recalcTextSize()
{
#ifndef ZAP_DEDICATED
   const F32 dummyTextSize = 120;

   F32 lineLen = getVert(0).distanceTo(getVert(1));      // In in-game units
   F32 strWidth = F32(UserInterface::getStringWidth(dummyTextSize, mText.c_str())) / dummyTextSize; 
   F32 size = lineLen / strWidth;

  setSize(size);
#endif
}


void TextItem::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


// Bounding box for display scoping purposes
Rect TextItem::calcExtents()
{
#ifdef ZAP_DEDICATED
   // Don't care much about it on the server, as server won't render, and nothing collides with TextItems
	return(Rect(getVert(0), getVert(1)));
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

   return extent;
#endif
}


bool TextItem::getCollisionPoly(Vector<Point> &polyPoints) const
{
   return false;
}


// Handle collisions with a TextItem.  Easy, there are none.
bool TextItem::collide(BfObject *hitObject)
{
   return false;
}


void TextItem::idle(BfObject::IdleCallPath path)
{
   // Laze about, read a book, take a nap, whatever
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
   updateExtentInDatabase();
}


///// Editor Methods

void TextItem::onAttrsChanging() { onGeomChanged(); }    // Runs when text is being changed in the editor
void TextItem::onAttrsChanged()  { onGeomChanged(); }
void TextItem::onGeomChanging()  { onGeomChanged(); }
                                 
void TextItem::onGeomChanged()
{
   recalcTextSize();
   Parent::onGeomChanged();
}


#ifndef ZAP_DEDICATED
// Static method: Provide hook into the object currently being edited with the attrubute editor for callback purposes
BfObject *TextItem::getAttributeEditorObject()     
{
   return mAttributeMenuUI->getObject(); 
}


static void textEditedCallback(string text)
{
   TextItem *textItem = dynamic_cast<TextItem *>(TextItem::getAttributeEditorObject());
   textItem->setText(text);
}


EditorAttributeMenuUI *TextItem::getAttributeMenu()
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(getGame());
      mAttributeMenuUI = new EditorAttributeMenuUI(clientGame);

      // "Blah" will be overwritten when startEditingAttrs() is called
      TextEntryMenuItem *menuItem = new TextEntryMenuItem("Text: ", "Blah", "", "", MAX_TEXTITEM_LEN);
      menuItem->setTextEditedCallback(textEditedCallback);

      mAttributeMenuUI->addMenuItem(menuItem);

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


// Get the menu looking like what we want
void TextItem::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setValue(mText);
}


// Retrieve the values we need from the menu
void TextItem::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   mText = attributeMenu->getMenuItem(0)->getValue();
}

#endif


//// Lua methods

/**
  *  @luaclass TextItem
  *  @brief Display text message in level.
  *  @descr A %TextItem displays text in a level.  If the %TextItem belongs to a team, it is only visible to players on that team.
  *         If it is assigned to NeutralTeam (the default), it will be visible to all players.  Text is always displayed in the color
  *         of the team it belongs to.
  *
  * Note that you will likely want to set the text of a new %TextItem (see setText()), as, by default, the display string is blank.
  *
  * Geometry for a %TextItem consists of two points representing the start and end points of the item.  Text will be scaled to
  * fit between these points.
  */
//               Fn name     Param profiles       Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, setText,      ARRAYDEF({{ STR, END }}), 1 ) \
   METHOD(CLASS, getText,      ARRAYDEF({{      END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE_NEW(TextItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(TextItem, LUA_METHODS);

#undef LUA_METHODS


const char *TextItem::luaClassName = "TextItem";
REGISTER_LUA_SUBCLASS(TextItem, BfObject);


/** 
  *  @luafunc TextItem::setText(text)
  *  @brief Sets the text of a %TextItem.
  *  @param text - A \e string specifying what the %TextItem should display.
  */
S32 TextItem::lua_setText(lua_State *L)
{
   checkArgList(L, functionArgs, "TextItem", "setText");

   setText(getString(L, 1));

   return 0;
}


/** 
  *  @luafunc text TextItem::getText()
  *  @brief Sets the text of a %TextItem.
  *  @return text - A \e string specifying what the %TextItem is currently displaying.
  */
S32 TextItem::lua_getText(lua_State *L)
{
   return returnString(L, getText().c_str());
}


};
