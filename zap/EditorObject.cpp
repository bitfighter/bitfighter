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

#include "EditorObject.h"
#include "WallSegmentManager.h"
#include "EngineeredItem.h"      // For Turret properties
#include "soccerGame.h"          // For soccer ball radius

#include "textItem.h"            // For copy constructor
#include "teleporter.h"          // For copy constructor
#include "speedZone.h"           // For copy constructor
#include "loadoutZone.h"
#include "goalZone.h"
#include "NexusGame.h"
#include "Colors.h"
#include "game.h"
#include "config.h"

#include "Geometry.h"            // For GeomType enum
#include "stringUtils.h"

#ifndef ZAP_DEDICATED
#   include "ClientGame.h"       // For ClientGame and getUIManager()
#   include "UIEditorMenus.h"    // For EditorAttributeMenuUI def
#   include "SDL_opengl.h"
#endif

using namespace boost;

namespace Zap
{
 

// Constructor
EditorObject::EditorObject() 
{ 
   mLitUp = false; 
   mSelected = false; 
   assignNewSerialNumber();
}


// Destructor
EditorObject::~EditorObject()
{
   // Do nothing
}


EditorObject *EditorObject::clone() const
{
   TNLAssert(false, "Clone method not implemented!");
   return NULL;
}


#ifndef ZAP_DEDICATED
void EditorObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
   mGame = game;

   unselectVerts();
   setTeam(teamIndex);
}


void EditorObject::addToEditor(ClientGame *game, GridDatabase *database)
{
   BfObject::addToGame(game, database);
   // constists of:
   //    mGame = game;
   //    addToDatabase();

   onGeomChanged();     // Added primarily as a generic way to get PolyWalls to build themselves after being dragged from the dock
}
#endif


#ifndef ZAP_DEDICATED
// Render selected and highlighted vertices, called from renderEditor
void EditorObject::renderAndLabelHighlightedVertices(F32 currentScale)
{
   F32 radius = getEditorRadius(currentScale);

   // Label and highlight any selected or lit up vertices.  This will also highlight point items.
   for(S32 i = 0; i < getVertCount(); i++)
      if(vertSelected(i) || isVertexLitUp(i) || ((mSelected || mLitUp)  && getVertCount() == 1))
      {
         glColor((vertSelected(i) || (mSelected && getGeomType() == geomPoint)) ? SELECT_COLOR : HIGHLIGHT_COLOR);

         Point center = getVert(i) + getEditorSelectionOffset(currentScale);

         drawSquare(center, radius / currentScale);
      }
}
#endif


Point EditorObject::getDockLabelPos()
{
   static const Point labelOffset(0, 11);

   return getPos() + labelOffset;
}


void EditorObject::highlightDockItem()
{
#ifndef ZAP_DEDICATED
   glColor(HIGHLIGHT_COLOR);
   drawSquare(getPos(), getDockRadius());
#endif
}


S32 EditorObject::getItemId()
{
   return mItemId;
}


void EditorObject::setItemId(S32 itemId)
{
   mItemId = itemId;
}


bool EditorObject::isSelected()
{
   return mSelected;
}


void EditorObject::setSelected(bool selected)
{
   mSelected = selected;
}


bool EditorObject::isLitUp() 
{ 
   return mLitUp; 
}


void EditorObject::setLitUp(bool litUp) 
{ 
   mLitUp = litUp; 

   if(!litUp) 
      setVertexLitUp(NONE); 
}


void EditorObject::initializeEditor()
{
   unselectVerts();
}


void EditorObject::onGeomChanging()
{
   if(getGeomType() == geomPolygon)
      onGeomChanged();               // Allows poly fill to get reshaped as vertices move

   onPointsChanged();
}


void EditorObject::onGeomChanged()
{
   updateExtentInDatabase();
}


void EditorObject::onItemDragging()
{
   // Do nothing
}


void EditorObject::onAttrsChanging()
{
   // Do nothing
}


void EditorObject::onAttrsChanged()
{
   // Do nothing
}


// Size of object in editor 
F32 EditorObject::getEditorRadius(F32 currentScale)
{
   return 10 * currentScale;   // 10 pixels is base size
}


// Return a pointer to a new copy of the object.  This is more like a duplicate or twin of the object -- it has the same
// serial number, and is already assigned to a game.
// You will have to delete this copy when you are done with it!
EditorObject *EditorObject::copy()
{
   EditorObject *newObject = clone();     
   newObject->initializeEditor();         // Marks all vertices as unselected

   return newObject;
}


// Return a pointer to a new copy of the object.  This copy will be completely new -- new serial number, mGame set to NULL, everything.
// You will have to delete this copy when you are done with it!
EditorObject *EditorObject::newCopy()
{
   EditorObject *newObject = copy();
   newObject->mGame = NULL;

   newObject->assignNewSerialNumber();    // Give this object an identity of its own

   return newObject;
}


void EditorObject::unselect()
{
   setSelected(false);
   setLitUp(false);

   unselectVerts();
}


void EditorObject::setSnapped(bool snapped)
{
   // Do nothing
}


// Called when item dragged from dock to editor -- overridden by several objects
void EditorObject::newObjectFromDock(F32 gridSize) 
{  
   assignNewSerialNumber();

   updateExtentInDatabase();
   clearGame();
}   





Point EditorObject::getEditorSelectionOffset(F32 scale)
{
   return Point(0,0);     // No offset for most items
}


Point EditorObject::getInitialPlacementOffset(F32 gridSize)
{
   return Point(0, 0);
}


void EditorObject::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   TNLAssert(false, "renderEditor not implemented!");
}


void EditorObject::renderDock()
{
   TNLAssert(false, "renderDock not implemented!");
}


EditorObjectDatabase *EditorObject::getEditorObjectDatabase()
{
   TNLAssert(dynamic_cast<EditorObjectDatabase *>(getDatabase()), "This should be a EditorObjectDatabase!");
   return static_cast<EditorObjectDatabase *>(getDatabase());
}


string EditorObject::getAttributeString()
{
   return "";
}


bool EditorObject::isVertexLitUp(S32 vertexIndex)
{
   return mVertexLitUp == vertexIndex;
}


void EditorObject::setVertexLitUp(S32 vertexIndex)
{
   mVertexLitUp = vertexIndex;
}


S32 EditorObject::getDockRadius()
{
   return 10;
}


const char *EditorObject::getEditorHelpString()
{
   TNLAssert(false, "getEditorHelpString method not implemented!");
   return "getEditorHelpString method not implemented!";  // better then a NULL crash in non-debug mode or continuing past the Assert
}


const char *EditorObject::getPrettyNamePlural()
{
   TNLAssert(false, "getPrettyNamePlural method not implemented!");
   return "getPrettyNamePlural method not implemented!";
}


const char *EditorObject::getOnDockName()
{
   TNLAssert(false, "getOnDockName method not implemented!");
   return "getOnDockName method not implemented!";
}


const char *EditorObject::getOnScreenName()
{
   TNLAssert(false, "getOnScreenName method not implemented!");
   return "getOnScreenName method not implemented!";
}


const char *EditorObject::getInstructionMsg()
{
   return "";
}

// For editing attributes:
EditorAttributeMenuUI *EditorObject::getAttributeMenu()
{
   return NULL;
}


void EditorObject::startEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   // Do nothing
}


void EditorObject::doneEditingAttrs(EditorAttributeMenuUI *attributeMenu)
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////

// TODO: merge with simpleLine values, put in editor
static const S32 INSTRUCTION_TEXTSIZE = 12;      
static const S32 INSTRUCTION_TEXTGAP = 4;
static const Color INSTRUCTION_TEXTCOLOR = Colors::white;      // TODO: Put in editor


// Constructor
PointObject::PointObject()
{
   setNewGeometry(geomPoint);
}


// Destructor
PointObject::~PointObject()
{
   // Do nothing
}


void PointObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
#ifndef ZAP_DEDICATED
   setPos(point);
   Parent::prepareForDock(game, point, teamIndex);
#endif
}

};
