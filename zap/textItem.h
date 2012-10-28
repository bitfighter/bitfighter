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

#ifndef _TEXTITEM_H_
#define _TEXTITEM_H_

#include "SimpleLine.h"       // For base class def
#include "Colors.h"

using namespace std;

namespace Zap
{

class ClientGame;
static const S32 MAX_TEXTITEM_LEN = 255;

class TextItem : public SimpleLine
{
   typedef SimpleLine Parent;

private:
   F32 mSize;              // Text size
   string mText;           // Text itself

   // How are this item's vertices labeled in the editor? -- these can be private
   const char *getInstructionMsg();

#ifndef ZAP_DEDICATED
   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for text editing; since it's static, don't bother with smart pointer
#endif

public:
   static const S32 MAX_TEXT_SIZE = 255;
   static const S32 MIN_TEXT_SIZE = 10;

   TextItem(lua_State *L = NULL);   // Combined Lua / C++ constructor
   virtual ~TextItem();             // Destructor

   TextItem *clone() const;

   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv, Game *game);  // Create objects from parameters stored in level file
   string toString(F32 gridSize) const;
   void setGeom(const Vector<Point> &points);
   void setGeom(const Point &pos, const Point &dest);
   Rect calcExtents();      // Bounding box for display scoping purposes


   void onAddedToGame(Game *theGame);  

#ifndef ZAP_DEDICATED

   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   // Provide a static hook into the object currently being edited with the attrubute editor for callback purposes
   static BfObject *getAttributeEditorObject();

#endif

   bool getCollisionPoly(Vector<Point> &polyPoints) const;          // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   void idle(BfObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor Methods

   Color getEditorRenderColor();

   void renderEditorItem();
   F32 getSize();

   string getText();
   void setText(string text);

   void onAttrsChanging();
   void onAttrsChanged();
   void onGeomChanging();
   void onGeomChanged();

   void recalcTextSize();
   void setSize(F32 desiredSize);


   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   void newObjectFromDock(F32 gridSize);

   TNL_DECLARE_CLASS(TextItem);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(TextItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_setText(lua_State *L);
   S32 lua_getText(lua_State *L);
};


};

#endif


