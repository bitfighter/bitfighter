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

#ifndef _LINEITEM_H_
#define _LINEITEM_H_

#include "SimpleLine.h"       // For SimpleLine def

using namespace std;

namespace Zap
{


class LineItem : public CentroidObject
{
   typedef CentroidObject Parent;

private:
   Vector<Point> mRenderPoints;     // Precomputed points used for rendering linework

   S32 mWidth;    

public:
   explicit LineItem(lua_State *L = NULL);   // Combined C++ / Lua constructor
   virtual ~LineItem();                      // Destructor
   LineItem *clone() const;

   virtual void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv, Game *game);   // Create objects from parameters stored in level file
   void onAddedToGame(Game *theGame);
   void computeExtent();                                             // Bounding box for quick collision-possibility elimination

   const Vector<Point> *getCollisionPoly() const;                    // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   void idle(BfObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

 
   /////
   // Editor methods
   string toLevelCode(F32 gridSize) const;
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);
   virtual const Color *getEditorRenderColor();


   // Thickness-related
   virtual void setWidth(S32 width);
   virtual void setWidth(S32 width, S32 min, S32 max);
   virtual S32 getWidth() const;
   void changeWidth(S32 amt);  


   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   static const S32 MIN_LINE_WIDTH = 1;
   static const S32 MAX_LINE_WIDTH = 255;

   TNL_DECLARE_CLASS(LineItem);

   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(LineItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];
};


};

#endif


