//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo relased for Torque Network Library by GarageGames.com
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

#ifndef _GO_FAST_H_
#define _GO_FAST_H_

#include "SimpleLine.h"    // For SimpleLine def
#include "BfObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"

namespace Zap
{
class EditorAttributeMenuUI;

class SpeedZone : public SimpleLine
{
   typedef SimpleLine Parent;

private:
   Vector<Point> mPolyBounds;
   U16 mSpeed;             // Speed at which ship is propelled, defaults to defaultSpeed
   bool mSnapLocation;     // If true, ship will be snapped to center of speedzone before being ejected
   
   // Take our basic inputs, pos and dir, and expand them into a three element
   // vector (the three points of our triangle graphic), and compute its extent
   void preparePoints();

   void computeExtent();                                            // Bounding box for quick collision-possibility elimination

#ifndef ZAP_DEDICATED
   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for attribute editing; since it's static, don't bother with smart pointer
#endif

public:
   enum {
      halfWidth = 25,
      height = 64,
      defaultSnap = 0,     // 0 = false, 1 = true

      InitMask     = BIT(0),
      HitMask      = BIT(1),
   };

   SpeedZone(lua_State *L = NULL);     // Combined C++/Lua constructor
   virtual ~SpeedZone();               // Destructor
      
   SpeedZone *clone() const;

   static const U16 minSpeed = 500;       // How slow can you go?
   static const U16 maxSpeed = 5000;      // Max speed for the goFast
   static const U16 defaultSpeed = 2000;  // Default speed if none specified

   U16 getSpeed();
   void setSpeed(U16 speed);

   bool getSnapping();
   void setSnapping(bool snapping);

   F32 mRotateSpeed;
   U32 mUnpackInit;  // Some form of counter, to know that it is a rotating speed zone.

   static void generatePoints(const Point &pos, const Point &dir, F32 gridSize, Vector<Point> &points);
   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv, Game *game);  // Create objects from parameters stored in level file
   string toString(F32 gridSize) const;

   void onAddedToGame(Game *theGame);

   const Vector<Point> *getCollisionPoly() const;          // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   bool collided(BfObject *s, U32 stateIndex);
   void idle(BfObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor methods 
   Color getEditorRenderColor();

   void renderEditorItem();

   void onAttrsChanging();
   void onGeomChanging();
   void onGeomChanged();

#ifndef ZAP_DEDICATED
   // These four methods are all that's needed to add an editable attribute to a class...
   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   string getAttributeString();
#endif

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   TNL_DECLARE_CLASS(SpeedZone);


   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(SpeedZone);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 setDir(lua_State *L);
   S32 getDir(lua_State *L);
   S32 setSpeed(lua_State *L);
   S32 getSpeed(lua_State *L);
   S32 setSnapping(lua_State *L);
   S32 getSnapping(lua_State *L);
};


};

#endif


