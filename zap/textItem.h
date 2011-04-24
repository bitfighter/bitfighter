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


#include "gameObject.h"
#include "UIEditor.h"      // For EditorObject (to be moved!)
#include "gameType.h"
#include "gameNetInterface.h"
#include "polygon.h"    // For def of Polyline, for lineItem
#include "UI.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "../glut/glutInclude.h"
#include <string>

//TODO: Make these regular vars
#define MAX_TEXTITEM_LEN 255

using namespace std;

namespace Zap
{

class SimpleLine : public EditorObject
{
private:
   virtual Color getEditorRenderColor() = 0;

   virtual const char *getOriginBottomLabel() = 0;          
   virtual const char *getDestinationBottomLabel() = 0;

public:
   // Some properties about the item that will be needed in the editor
   GeomType getGeomType() { return geomSimpleLine; }

   virtual Point getVert(S32 index) = 0;
   virtual const char *getOnDockName() = 0;

   void renderDock();     // Render item on the dock
   void renderEditor(F32 currentScale);
   virtual void renderEditorItem(F32 currentScale) = 0;

   virtual S32 getVertCount() { return 2; }

   void deleteVert(S32 vertIndex) { /* Do nothing */ }
};


////////////////////////////////////////
////////////////////////////////////////

class TextItem : public SimpleLine
{
   typedef GameObject Parent;

private:
   U32 mSize;            // Text size
   Point mPos;           // Location of text
   Point mDir;           // Direction text is "facing"
   string mText;         // Text itself

   Color getEditorRenderColor() { return Color(0,0,1); }

   // For editor
   // How are this item's vertices labeled in the editor?
   const char *getOriginBottomLabel() { return "Start"; }
   const char *getDestinationBottomLabel() { return "Direction"; }


            /*if(getObjectTypeMask() & ItemTeleporter)
            return "Intake Vortex";
         else if(getObjectTypeMask() & ItemSpeedZone)
            return "Location";
         else if(getObjectTypeMask() & ItemTextItem)
            return "Start";*/

                    /* if(getObjectTypeMask() & ItemTeleporter)
            return "Destination";
         else if(getObjectTypeMask() & ItemSpeedZone || getObjectTypeMask() & ItemTextItem)
            return "Direction";*/

public:
   static const U32 MAX_TEXT_SIZE = 255;
   static const U32 MIN_TEXT_SIZE = 10;

   S32 mTeam;            // Team text is visible to (-1 for visible to all)
   
   TextItem();    // Constructor
   ~TextItem();   // Destructor

   static Vector<Point> generatePoints(Point pos, Point dir);
   void render();
   void renderEditorItem(F32 currentScale);
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv);           // Create objects from parameters stored in level file
   void saveItem(FILE *f);

   void onAddedToGame(Game *theGame);
   void computeExtent();                                         // Bounding box for quick collision-possibility elimination

   U32 getSize() { return mSize; }

   Vector<Point> getVerts() { Vector<Point> p; p.push_back(mPos); p.push_back(mDir); return p; }      // TODO: Can we get rid of this somehow?

   Point getVert(S32 index) { return index == 0 ? mPos : mDir; }
   void setVert(const Point &point, S32 index) { if(index == 0) mPos = point; else mDir = point; }

   LineEditor lineEditor;    // For editing in the editor

   bool getCollisionPoly(Vector<Point> &polyPoints);  // More precise boundary for precise collision detection
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor Methods
   void onAttrsChanging();
   void onGeomChanging();
   void onGeomChanged();

   // Some properties about the item that will be needed in the editor
   bool hasText() { return true; }
   const char *getEditorHelpString() { return "Draws a bit of text on the map.  Visible only to team, or to all if neutral."; }  
   const char *getPrettyNamePlural() { return "Text Items"; }
   const char *getOnDockName() { return "TextItem"; }
   const char *getOnScreenName() { return "Text"; }
   bool hasTeam() { return true; }
   bool canBeHostile() { return true; }
   bool canBeNeutral() { return true; }
   bool getHasRepop() { return false; }


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


