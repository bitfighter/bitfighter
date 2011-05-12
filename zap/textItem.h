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

#include "SimpleLine.h"       // For SimpleLine def
#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "polygon.h"          // For def of Polyline, for lineItem
#include "UI.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "../glut/glutInclude.h"
#include <string>


using namespace std;

namespace Zap
{

static const S32 MAX_TEXTITEM_LEN = 255;

class TextItem : public SimpleLine
{
   typedef GameObject Parent;

private:
   F32 mSize;            // Text size
   Point mPos;           // Location of text
   Point mDir;           // Direction text is "facing"
   string mText;         // Text itself

   // How are this item's vertices labeled in the editor? -- these can be private
   const char *getVertLabel(S32 index) { return index == 0 ? "Start" : "Direction"; }
   const char *getInstructionMsg() { return "[Enter] to edit text"; }

   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for text editing; since it's static, don't bother with smart pointer

public:
   static const U32 MAX_TEXT_SIZE = 255;
   static const U32 MIN_TEXT_SIZE = 10;

   //S32 mTeam;            // Team text is visible to (-1 for visible to all)
   
   TextItem();    // Constructor
   ~TextItem();   // Destructor

   static Vector<Point> generatePoints(Point pos, Point dir);
   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv);           // Create objects from parameters stored in level file
   string toString();
   void initializeEditor(F32 gridSize);

   void onAddedToGame(Game *theGame);
   void computeExtent();                                         // Bounding box for quick collision-possibility elimination

   Vector<Point> getVerts() { Vector<Point> p; p.push_back(mPos); p.push_back(mDir); return p; }      // TODO: Can we get rid of this somehow?


   EditorAttributeMenuUI *getAttributeMenu();

   bool getCollisionPoly(Vector<Point> &polyPoints);  // More precise boundary for precise collision detection
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor Methods

   Color getEditorRenderColor() { return Color(0,0,1); }

   void renderEditorItem();
   F32 getSize() { return mSize; }
   void setSize(F32 size) { mSize = size; }

   string getText() { return mText; }
   void setText(string text) { mText = text; }

   Point getVert(S32 index) { return index == 0 ? mPos : mDir; }
   void setVert(const Point &point, S32 index) { if(index == 0) mPos = point; else mDir = point; }

   // Provide a static hook into the object currently being edited with the attrubute editor for callback purposes
   static EditorObject *getAttributeEditorObject();

   void onAttrsChanging();
   void onAttrsChanged();
   void onGeomChanging();
   void onGeomChanged();

   void recalcTextSize();

   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   Point getInitialPlacementOffset(F32 gridSize) { return Point(.4, 0); }

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() { return "Draws a bit of text on the map.  Visible only to team, or to all if neutral."; }  
   const char *getPrettyNamePlural() { return "Text Items"; }
   const char *getOnDockName() { return "TextItem"; }
   const char *getOnScreenName() { return "Text"; }
   bool hasTeam() { return true; }
   bool canBeHostile() { return true; }
   bool canBeNeutral() { return true; }

   bool showAttribsWhenSelected() { return false; }      // We already show the attributes, as the text itself

   TNL_DECLARE_CLASS(TextItem);
};


////////////////////////////////////////
////////////////////////////////////////

class LineItem : public GameObject, public Polyline
{
private:
   typedef GameObject Parent;
   Vector<Point> mRenderPoints;    // Precomputed points used for rendering linework

public:
   U32 mWidth;           // Width of line
   S32 mTeam;            // Team text is visible to (-1 for visible to all)
   
   LineItem();   // Constructor

   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv);           // Create objects from parameters stored in level file
   void onAddedToGame(Game *theGame);
   void computeExtent();                                         // Bounding box for quick collision-possibility elimination

   bool getCollisionPoly(Vector<Point> &polyPoints);             // More precise boundary for precise collision detection
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   static const U32 MIN_LINE_WIDTH = 1;
   static const U32 MAX_LINE_WIDTH = 255;

   TNL_DECLARE_CLASS(LineItem);
};


};

#endif


