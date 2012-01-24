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
#   include "SDL/SDL_opengl.h"
#endif

using namespace boost;

namespace Zap
{
 
// Declare statics
S32 EditorObject::mNextSerialNumber = 0;
bool EditorObject::mBatchUpdatingGeom = false;

// Constructor
EditorObject::EditorObject() 
{ 
   mDockItem = false; 
   mLitUp = false; 
   mSelected = false; 
   mIsBeingEdited = false;
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


//void EditorObject::copyAttrs(EditorObject *target)
//{
//   target->mGeometry = mGeometry->copyGeometry();
//   target->mGame = mGame;
//
//   target->mDockItem = mDockItem; 
//   target->mLitUp = mLitUp; 
//   target->mSelected = mSelected; 
//   target->setObjectTypeMask(getObjectTypeMask()); 
//   mIsBeingEdited = false;
//   mSerialNumber = mNextSerialNumber++;
//}


void EditorObject::prepareForDock(Game *game, const Point &point)
{
   mGame = game;

   mDockItem = true;
   
   unselectVerts();
}


void EditorObject::addToEditor(Game *game)
{
   BfObject::addToGame(game, game->getEditorDatabase());
   // constists of:
   //    mGame = game;
   //    addToDatabase();
}


void EditorObject::assignNewSerialNumber()
{
   mSerialNumber = mNextSerialNumber++;
}


bool EditorObject::isBatchUpdatingGeom()
{
   return mBatchUpdatingGeom;
}


// TODO: Merge with copy in editor, if it's really needed
static F32 getRenderingAlpha(bool isScriptItem)
{
   return isScriptItem ? .6f : 1;     // Script items will appear somewhat translucent
}


static const S32 DOCK_LABEL_SIZE = 9;      // Size to label items on the dock

// Render selected and highlighted vertices, called from renderEditor
void EditorObject::renderAndLabelHighlightedVertices(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   F32 radius = getEditorRadius(currentScale);

   // Label and highlight any selected or lit up vertices.  This will also highlight point items.
   for(S32 i = 0; i < getVertCount(); i++)
      if(vertSelected(i) || isVertexLitUp(i) || ((mSelected || mLitUp)  && getVertCount() == 1))
      {
         glColor((vertSelected(i) || (mSelected && getGeomType() == geomPoint)) ? SELECT_COLOR : HIGHLIGHT_COLOR);

         Point center = getVert(i) + getEditorSelectionOffset(currentScale);

         drawSquare(center, radius / currentScale);
      }
#endif
}


void EditorObject::renderDockItemLabel(const Point &pos, const char *label, F32 yOffset)
{
#ifndef ZAP_DEDICATED
   F32 xpos = pos.x;
   F32 ypos = pos.y - DOCK_LABEL_SIZE / 2 + yOffset;
   glColor(Colors::white);
   UserInterface::drawStringc(xpos, ypos + (F32)DOCK_LABEL_SIZE, (F32)DOCK_LABEL_SIZE, label);
#endif
}


void EditorObject::labelDockItem()
{
   renderDockItemLabel(getVert(0), getOnDockName(), 11);
}


void EditorObject::highlightDockItem()
{
#ifndef ZAP_DEDICATED
   glColor(HIGHLIGHT_COLOR);
   drawSquare(getVert(0), getDockRadius());
#endif
}


// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorObject::renderInEditor(F32 currentScale, S32 snapIndex, bool isScriptItem, bool showingReferenceShip)
{
#ifndef ZAP_DEDICATED
   Point pos, dest;
   F32 alpha = getRenderingAlpha(isScriptItem);

   Color drawColor;

   if(mSelected)
      glColor(SELECT_COLOR, alpha);       // yellow
   else if(mLitUp)
      glColor(HIGHLIGHT_COLOR, alpha);    // white
   else  // Normal
      glColor(Color(.75), alpha);

   // Override drawColor for this special case
   if(anyVertsSelected())
      drawColor = *SELECT_COLOR;

   if(mDockItem)
   {
      renderDock();
      labelDockItem();

      if(mLitUp)
         highlightDockItem();
   }
   else  // Not a dock item
   {
      if(showingReferenceShip)
         renderEditorPreview(currentScale);
      else
      {
         renderEditor(currentScale);
         renderAndLabelHighlightedVertices(currentScale);
      }
   }
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


S32 EditorObject::getSerialNumber()
{
   return mSerialNumber;
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


bool EditorObject::isBeingEdited()
{
   return mIsBeingEdited;
}


void EditorObject::setIsBeingEdited(bool isBeingEdited)
{
   mIsBeingEdited = isBeingEdited;
}


void EditorObject::initializeEditor()
{
   unselectVerts();
}


S32 EditorObject::getScore()
{
   return mScore;
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


void EditorObject::saveItem(FILE *f, F32 gridSize)
{
   s_fprintf(f, "%s\n", toString(gridSize).c_str());
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
   EditorObject *newObject = clone();     // TODO: Wrap in shared_ptr?
   newObject->setExtent(getExtent());     // Why do we need to do this???
   newObject->initializeEditor();         // Unselects all vertices

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


const Color *EditorObject::getTeamColor(S32 teamId) 
{ 
   return mGame->getTeamColor(teamId);
}


// By default, we'll just render this as we do in game.  Occasionally (like with textItems), we may need to do something special.
void EditorObject::renderEditorPreview(F32 currentScale)
{
   GameObject *gameObject = dynamic_cast<GameObject *>(this);
   TNLAssert(gameObject, "This object cannot be cast to a GameObject, therefore I can't render it in preview mode!");

   if(gameObject)
      gameObject->render();
}


// Draw the vertices for a polygon or line item (i.e. walls)
void EditorObject::renderLinePolyVertices(F32 currentScale, F32 alpha)
{
#ifndef ZAP_DEDICATED

   const ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   // Draw the vertices of the wall or the polygon area
   for(S32 j = 0; j < getVertCount(); j++)
   {
      if(vertSelected(j))
         renderVertex(SelectedVertex, getVert(j), j, currentScale, alpha);             // Hollow yellow boxes with number
      else if(mLitUp && isVertexLitUp(j))
         renderVertex(HighlightedVertex, getVert(j), j, currentScale, alpha);          // Hollow yellow boxes with number
      else if(mSelected || mLitUp || anyVertsSelected())
         renderVertex(SelectedItemVertex, getVert(j), j, currentScale, alpha);         // Hollow red boxes with number
      else
         // Using getUIManager() here is a horrible hack... but not sure how to improve the situation...
         renderSmallSolidVertex(currentScale, getVert(j), clientGame->getUIManager()->getEditorUserInterface()->getSnapToWallCorners());
   }
#endif
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
   mDockItem = false;
   clearGame();
}   


// Move object to location, specifying (optional) vertex to be positioned at pos
void EditorObject::moveTo(const Point &pos, S32 snapVertex)
{
   offset(pos - getVert(snapVertex));
}


void EditorObject::offset(const Point &offset)
{
   for(S32 i = 0; i < getVertCount(); i++)
      setVert(getVert(i) + offset, i);
}


Point EditorObject::getEditorSelectionOffset(F32 scale)
{
   return Point(0,0);     // No offset for most items
}


Point EditorObject::getInitialPlacementOffset(F32 gridSize)
{
   return Point(0, 0);
}


void EditorObject::renderItemText(const char *text, S32 offset, F32 currentScale)
{
#ifndef ZAP_DEDICATED
   UserInterface::drawString(F32(580), F32(120), 12 / currentScale, text );     // TODO: Fix
#endif
}


void EditorObject::renderEditor(F32 currentScale)
{
   TNLAssert(false, "renderEditor not implemented!");
}


void EditorObject::renderDock()
{
   TNLAssert(false, "renderDock not implemented!");
}


void EditorObject::beginBatchGeomUpdate()
{
   mBatchUpdatingGeom = true;
}


#ifndef ZAP_DEDICATED
void EditorObject::endBatchGeomUpdate(ClientGame *game, bool modifiedWalls)
{
   if(modifiedWalls)
   {
      EditorObjectDatabase *database = game->getEditorDatabase();
      database->getWallSegmentManager()->finishedChangingWalls(database);
   }

   mBatchUpdatingGeom = false;
}
#endif


bool EditorObject::showAttribsWhenSelected()
{
   return true;
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


const char *EditorObject::getVertLabel(S32 index)
{
   return "";
}


const char *EditorObject::getEditorHelpString()
{
   TNLAssert(false, "getEditorHelpString method not implemented!");
   return NULL;
}


const char *EditorObject::getPrettyNamePlural()
{
   TNLAssert(false, "getPrettyNamePlural method not implemented!");
   return NULL;
}


const char *EditorObject::getOnDockName()
{
   TNLAssert(false, "getOnDockName method not implemented!");
   return NULL;
}


const char *EditorObject::getOnScreenName()
{
   TNLAssert(false, "getOnScreenName method not implemented!");
   return NULL;
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

 bool EditorObject::hasTeam()
{
   return true;
}


bool EditorObject::canBeNeutral()
{
   return true;
}


bool EditorObject::canBeHostile()
{
   return true;
}


////////////////////////////////////////
////////////////////////////////////////

// TODO: merge with simpleLine values, put in editor
static const S32 INSTRUCTION_TEXTSIZE = 12;      
static const S32 INSTRUCTION_TEXTGAP = 4;
static const Color INSTRUCTION_TEXTCOLOR = Colors::white;      // TODO: Put in editor


// Offset: negative below the item, positive above
void EditorPointObject::renderItemText(const char *text, S32 offset, F32 currentScale)
{
#ifndef ZAP_DEDICATED
   Parent::renderItemText(text, offset, currentScale);
   return;
   glColor(INSTRUCTION_TEXTCOLOR);

   Point pos = getVert(0);
   
   // Dividing by currentScale keeps the text a constant size in pixels
   UserInterface::drawCenteredString(pos.x, pos.y + getEditorRadius(currentScale) / currentScale, F32(INSTRUCTION_TEXTSIZE) / currentScale, text);
   UserInterface::drawCenteredString(pos.x, pos.y + (getEditorRadius(currentScale) + INSTRUCTION_TEXTSIZE * 1.25f) / currentScale, F32(INSTRUCTION_TEXTSIZE) / currentScale, "[Enter] to edit");
#endif
}


void EditorPointObject::prepareForDock(Game *game, const Point &point)
{
   setVert(point, 0);
   Parent::prepareForDock(game, point);
}


////////////////////////////////////////
////////////////////////////////////////

//string EditorItem::toString(F32 gridSize) const
//{
//   return string(getClassName()) + " " + geomToString(gridSize);
//}
//
//
//void EditorItem::renderEditor(F32 currentScale)
//{
//   renderItem(getVert(0));                    
//}
//
//
//F32 EditorItem::getEditorRadius(F32 currentScale)
//{
//   return (getRadius() + 2) * currentScale;
//}


};
