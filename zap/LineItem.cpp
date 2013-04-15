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

#include "LineItem.h"
#include "gameObjectRender.h"    // For renderPolyLineVertices()
#include "stringUtils.h"         // For itos
#include "teamInfo.h"            // For TEAM_NEUTRAL
#include "ship.h"                // To check player's team
#include "game.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"
#  include "OpenglUtils.h"       // For glColor, et al
#  include "UIEditorMenus.h"     // For EditorAttributeMenuUI def
#endif


#include <math.h>

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(LineItem);

// Why does GCC need this for signed int?
#ifndef TNL_OS_WIN32
   const S32 LineItem::MIN_LINE_WIDTH;
   const S32 LineItem::MAX_LINE_WIDTH;
#endif

// Statics:
#ifndef ZAP_DEDICATED
   EditorAttributeMenuUI *LineItem::mAttributeMenuUI = NULL;
#endif

// Combined C++ / Lua constructor
LineItem::LineItem(lua_State *L)
{ 
   mNetFlags.set(Ghostable);
   setNewGeometry(geomPolyLine);
   mObjectTypeNumber = LineTypeNumber;

   mWidth = 2;
   mGlobal = true;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
   
   if(L)
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { SIMPLE_LINE, END }, { SIMPLE_LINE, TEAM_INDX, END }}, 3 };

      S32 profile = checkArgList(L, constructorArgList, "LineItem", "constructor");

      if(profile == 1)
         setGeom(L, 1);

      else if(profile == 2)
      {
         setGeom(L, 1);
         setTeam(lua_tointeger(L, -1)); 
      }
   }
}


// Destructor
LineItem::~LineItem()
{ 
   LUAW_DESTRUCTOR_CLEANUP;
}


LineItem *LineItem::clone() const
{
   return new LineItem(*this);
}


void LineItem::render()
{
#ifndef ZAP_DEDICATED
   bool sameTeam = false;

   GameConnection *gc = static_cast<ClientGame *>(getGame())->getConnectionToServer();

   // Don't render opposing team's text items... gc will only exist in-game
   if(gc)
   {
      BfObject *object = gc->getControlObject();

      if(!object || object->getObjectTypeNumber() != PlayerShipTypeNumber)
         return;

      Ship *ship = static_cast<Ship *>(object);

      if(getTeam() == TEAM_NEUTRAL || ship->getTeam() == getTeam())
         sameTeam = true;
   }
   else
      sameTeam = true;     // Render item regardless of team when in editor, which is when gc will be NULL

   // Now render
   if(mGlobal || sameTeam)
   {
      glColor(getColor());
      renderLine(getOutline());
   }
#endif
}


void LineItem::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   if(!isSelected() && !isLitUp())
      glColor(getEditorRenderColor());

   renderLine(getOutline());
   renderPolyLineVertices(this, snappingToWallCornersEnabled, currentScale);
#endif
}


const Color *LineItem::getEditorRenderColor() 
{ 
   return getColor(); 
}


// This object should be drawn below others
S32 LineItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// LineItem <team> <width> <x> <y> ...
bool LineItem::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc < 6)
      return false;

   setTeam (atoi(argv[0]));
   setWidth(atoi(argv[1]));

   int firstCoord = 2;
   if(strcmp(argv[2], "Global") == 0)
   {
      mGlobal = true;
      firstCoord = 3;
   }
   else
      mGlobal = false;

   readGeom(argc, argv, firstCoord, game->getGridSize());

   computeExtent();

   return true;
}


string LineItem::toLevelCode(F32 gridSize) const
{
   string out = string(appendId(getClassName())) + " " + itos(getTeam()) + " " + itos(getWidth());

   if(mGlobal)
      out += " Global";

   out += " " + geomToLevelCode(gridSize);

   return out;
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
   updateExtentInDatabase();
}


const Vector<Point> *LineItem::getCollisionPoly() const
{
   return NULL;
}


// Handle collisions with a LineItem.  Easy, there are none.
bool LineItem::collide(BfObject *hitObject)
{
   return false;
}


void LineItem::idle(BfObject::IdleCallPath path)
{
   // Do nothing
}


U32 LineItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //stream->writeRangedU32(mWidth, 0, MAX_LINE_WIDTH);
   writeThisTeam(stream);
   stream->write(mGlobal);

   packGeom(connection, stream);

   return 0;
}


void LineItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //mWidth = stream->readRangedU32(0, MAX_LINE_WIDTH);
   readThisTeam(stream);
   mGlobal = stream->readFlag();  // Set this client side

   unpackGeom(connection, stream);
   updateExtentInDatabase();
}


S32 LineItem::getWidth() const
{
   return mWidth;
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

#ifndef ZAP_DEDICATED

EditorAttributeMenuUI *LineItem::getAttributeMenu()
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(getGame());

      mAttributeMenuUI = new EditorAttributeMenuUI(clientGame);

      mAttributeMenuUI->addMenuItem(new YesNoMenuItem("Global:", true, "Viewable by all teams"));

      // Add our standard save and exit option to the menu
      mAttributeMenuUI->addSaveAndQuitMenuItem();
   }

   return mAttributeMenuUI;
}


// Get the menu looking like what we want
void LineItem::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   attributeMenu->getMenuItem(0)->setIntValue(mGlobal ? 1 : 0);
}


// Retrieve the values we need from the menu
void LineItem::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   mGlobal = attributeMenu->getMenuItem(0)->getIntValue();    // Returns 0 or 1
}


// Render some attributes when item is selected but not being edited
void LineItem::fillAttributesVectors(Vector<string> &keys, Vector<string> &values)
{
   keys.push_back("Global");    values.push_back(mGlobal ? "Yes" : "No");
}

#endif


const char *LineItem::getOnScreenName()     { return "Line";      }
const char *LineItem::getPrettyNamePlural() { return "LineItems"; }
const char *LineItem::getOnDockName()       { return "LineItem";  }
const char *LineItem::getEditorHelpString() { return "Draws a line on the map.  Visible only to team, or to all if neutral."; }

bool LineItem::hasTeam()      { return true; }
bool LineItem::canBeHostile() { return true; }
bool LineItem::canBeNeutral() { return true; }


/////
// Lua interface

/**
  *  @luaclass LineItem
  *  @brief    Decorative line visible to one or all teams.  Has no specific game function.
  *  @descr    If a %LineItem is assigned to a team, it will only be visible to players on that team.  If
  *            the %LineItem is neutral (team == NeutralTeamIndx, the default), it will be visible to all players regardless of team.
  *  @geom     The geometry of a %LineItem is a polyline (i.e. 2 or more points)
  */
//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
      METHOD(CLASS, setGlobal, ARRAYDEF({{ BOOL,    END }}), 1 ) \
      METHOD(CLASS, getGlobal, ARRAYDEF({{          END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(LineItem, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(LineItem, LUA_METHODS);

#undef LUA_METHODS


/**
  *  @luafunc LineItem::setGlobal(global)
  *  @brief   Sets the %LineItem's global parameter.
  *  @descr   LineItems are normally viewable by all players in a game.  If you wish to only let the LineItem
  *           be viewable to the owning team, set to false.  Make sure you call setTeam() on the LineItem
  *           first.
  *  Global is on by default.
  *  @param   global - False if this %LineItem should be viewable only by the owning team, otherwise viewable
  *           by all teams.
  */
S32 LineItem::lua_setGlobal(lua_State *L)
{
   checkArgList(L, functionArgs, "LineItem", "setGlobal");
   mGlobal = getBool(L, 1);

   return 0;
}


/**
  *  @luafunc num LineItem::getSnapping()
  *  @brief   Returns the %LineItem's global parameter.
  *  @return  A boolean; true if global is enabled, false if not.
  */
S32 LineItem::lua_getGlobal(lua_State *L)
{
   return returnBool(L, mGlobal);
}


const char *LineItem::luaClassName = "LineItem";
REGISTER_LUA_SUBCLASS(LineItem, BfObject);

};
