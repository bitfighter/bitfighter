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
#include "engineeredObjects.h"   // For Turret properties
#include "soccerGame.h"          // For soccer ball radius

#include "textItem.h"            // For copy constructor
#include "teleporter.h"          // For copy constructor
#include "speedZone.h"           // For copy constructor
#include "loadoutZone.h"
#include "goalZone.h"
#include "huntersGame.h"
#include "Colors.h"
#include "game.h"

#include "Geometry.h"            // For GeomType enum

#include "UIEditorMenus.h"       // For EditorAttributeMenuUI def

#include "SDL/SDL_opengl.h"

using namespace boost;

namespace Zap
{

S32 EditorObject::mNextSerialNumber = 0;


inline F32 getGridSize()
{
   return gEditorGame->getGridSize();
}


// Constructor
EditorObject::EditorObject(GameObjectType objectType) 
{ 
   mDockItem = false; 
   mLitUp = false; 
   mSelected = false; 
   setObjectTypeMask(objectType); 
   mIsBeingEdited = false;
   assignNewSerialNumber();
}


// Destructor
EditorObject::~EditorObject()
{
   // Do nothing
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


void EditorObject::addToDock(Game *game, const Point &point)
{
   mGame = game;
   mDockItem = true;
   
   unselectVerts();

   gEditorUserInterface.addToDock(this);
}


// TODO: Merge with copy in editor, if it's really needed
static F32 getRenderingAlpha(bool isScriptItem)
{
   return isScriptItem ? .6 : 1;     // Script items will appear somewhat translucent
}


// TODO: Merge with copy in editor, if it's really needed
inline Point convertLevelToCanvasCoord(const Point &point, bool convert = true) 
{ 
   return gEditorUserInterface.convertLevelToCanvasCoord(point, convert); 
}


// Replaces the need to do a convertLevelToCanvasCoord on every point before rendering
static void setLevelToCanvasCoordConversion()
{
   F32 scale =  gEditorUserInterface.getCurrentScale();
   Point offset = gEditorUserInterface.getCurrentOffset();

   glTranslatef(offset.x, offset.y, 0);
   glScalef(scale, scale, 1);
} 

// TODO: merge with UIEditor versions
static const Color grayedOutColorBright = Colors::gray50;
static const Color grayedOutColorDim = Color(.25, .25, .25);
static const S32 NO_NUMBER = -1;

// Draw a vertex of a selected editor item
static void renderVertex(VertexRenderStyles style, const Point &v, S32 number, F32 currentScale, F32 alpha, F32 size)
{
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != NO_NUMBER)
   {
      glColor3f(.25, .25, .25);
      drawFilledSquare(v, size / currentScale);
   }

   if(style == HighlightedVertex)
      glColor(*HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(*SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(Colors::magenta, alpha);
   else
      glColor(Colors::red, alpha);

   drawSquare(v, size / currentScale, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(Colors::white, alpha);
      F32 txtSize = 6.0 / currentScale;
      UserInterface::drawStringf(v.x - F32(UserInterface::getStringWidthf(txtSize, "%d", number)) / 2, v.y - 3 / currentScale, txtSize, "%d", number);
   }
}


static void renderVertex(VertexRenderStyles style, const Point &v, S32 number, F32 currentScale, F32 alpha)
{
   renderVertex(style, v, number, currentScale, alpha, 5);
}


//static void renderVertex(VertexRenderStyles style, const Point &v, S32 number)
//{
//   renderVertex(style, v, number, 1);
//}


static const S32 DOCK_LABEL_SIZE = 9;      // Size to label items on the dock


static void labelVertex(Point pos, S32 radius, const char *itemLabelTop, const char *itemLabelBottom)
{
   F32 labelSize = DOCK_LABEL_SIZE;

   UserInterface::drawStringc(pos.x, pos.y - radius - labelSize - 5, labelSize, itemLabelTop);     // Above the vertex
   UserInterface::drawStringc(pos.x, pos.y + radius + 2, labelSize, itemLabelBottom);              // Below the vertex
}


// TODO: Fina a way to do this sans global
Point EditorObject::convertLevelToCanvasCoord(const Point &pt) 
{ 
   return gEditorUserInterface.convertLevelToCanvasCoord(pt); 
}


static const Color INSTRUCTION_TEXTCOLOR(1,1,1);      // TODO: Put in editor

void EditorObject::renderAttribs(F32 currentScale)
{
   if(isSelected() && !isBeingEdited() && showAttribsWhenSelected())
   {
      // Now list the attributes above the item
      EditorAttributeMenuUI *attrMenu = getAttributeMenu();

      if(attrMenu)
      {
         glColor(INSTRUCTION_TEXTCOLOR);

         S32 menuSize = attrMenu->menuItems.size();
         for(S32 i = 0; i < menuSize; i++)       
         {
            string txt = attrMenu->menuItems[i]->getPrompt() + ": " + attrMenu->menuItems[i]->getValue();      // TODO: Make this concatenation a method on the menuItems themselves?
            renderItemText(txt.c_str(), menuSize - i, 1);
         }
      }
   }
}


// Render selected and highlighted vertices, called from renderEditor
void EditorObject::renderAndLabelHighlightedVertices(F32 currentScale)
{
   F32 radius = getEditorRadius(currentScale);

   // Label and highlight any selected or lit up vertices.  This will also highlight point items.
   for(S32 i = 0; i < getVertCount(); i++)
      if(vertSelected(i) || isVertexLitUp(i) || ((mSelected || mLitUp)  && getVertCount() == 1))
      {
         glColor((vertSelected(i) || mSelected) ? SELECT_COLOR : HIGHLIGHT_COLOR);

         Point pos = gEditorUserInterface.convertLevelToCanvasCoord(getVert(i));

         drawSquare(pos, radius);
         labelVertex(pos, radius, getOnScreenName(), getVertLabel(i));
      }         
}


void EditorObject::renderDockItemLabel(const Point &pos, const char *label, F32 yOffset)
{
   F32 xpos = pos.x;
   F32 ypos = pos.y - DOCK_LABEL_SIZE / 2 + yOffset;
   glColor(Colors::white);
   UserInterface::drawStringc(xpos, ypos, DOCK_LABEL_SIZE, label);
}


void EditorObject::labelDockItem()
{
   renderDockItemLabel(getVert(0), getOnDockName(), 11);
}


void EditorObject::highlightDockItem()
{
   glColor(HIGHLIGHT_COLOR);
   drawSquare(getVert(0), getDockRadius());
}



extern void renderPolygon(const Vector<Point> &fillPoints, const Vector<Point> &outlinePoints, const Color &fillColor, const Color &outlineColor, F32 alpha = 1);

static const S32 asteroidDesign = 2;      // Design we'll use for all asteroids in editor

// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorObject::render(bool isScriptItem, bool showingReferenceShip, ShowMode showMode)    // TODO: pass scale
{
   const S32 instrSize = 9;      // Size of instructions for special items
   const S32 attrSize = 10;

   Point pos, dest;
   F32 alpha = getRenderingAlpha(isScriptItem);

   bool hideit = (showMode == ShowWallsOnly) && !(showingReferenceShip && !mDockItem);

   Color drawColor;
   if(hideit)
      glColor(grayedOutColorBright, alpha);
   else 
      glColor(getDrawColor(), alpha);

   glEnableBlend;        // Enable transparency

   S32 snapIndex = gEditorUserInterface.getSnapVertexIndex();

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
   else if(showingReferenceShip)
   {
      glPushMatrix();
         setLevelToCanvasCoordConversion();
         BfObject::render();
      glPopMatrix();
   }
   else
   {
      glPushMatrix();
         setLevelToCanvasCoordConversion();
         renderEditor(gEditorUserInterface.getCurrentScale());
      glPopMatrix();

      
      // Label item with instruction message describing what happens if user presses enter
      if(isSelected() && !isBeingEdited())
         renderItemText(getInstructionMsg(), -1, gEditorUserInterface.getCurrentScale());

      renderAndLabelHighlightedVertices(gEditorUserInterface.getCurrentScale());
      renderAttribs(gEditorUserInterface.getCurrentScale());
   }

   glDisableBlend;
}


// Draw barrier centerlines; wraps renderPolyline()  ==> lineItem, barrierMaker only
void EditorObject::renderPolylineCenterline(F32 alpha)
{
   // Render wall centerlines
   if(mSelected)
      glColor(SELECT_COLOR, alpha);
   else if(mLitUp && !anyVertsSelected())
      glColor(*HIGHLIGHT_COLOR, alpha);
   else
      glColor(getTeamColor(mTeam), alpha);

   glLineWidth(WALL_SPINE_WIDTH);
   EditorUserInterface::renderPolyline(getOutline());
   glLineWidth(gDefaultLineWidth);
}


void EditorObject::initializeEditor()
{
   unselectVerts();
}


void EditorObject::onGeomChanging()
{
   if(getGeomType() == geomPoly)
      onGeomChanged();               // Allows poly fill to get reshaped as vertices move
   onPointsChanged();
}


Color EditorObject::getDrawColor()
{
   if(mSelected)
      return *SELECT_COLOR;       // yellow
   else if(mLitUp)
      return *HIGHLIGHT_COLOR;    // white
   else  // Normal
      return Color(.75, .75, .75);
}


void EditorObject::saveItem(FILE *f, F32 gridSize)
{
   s_fprintf(f, "%s\n", toString(gridSize).c_str());
}


// Return a pointer to a new copy of the object.  You will have to delete this copy when you are done with it!
// This is kind of a hack, but not sure of a better way to do this...  perhaps a clone method in each object?
EditorObject *EditorObject::newCopy()
{
   EditorObject *newObject = clone();     // TODO: Wrap in shared_ptr?

   newObject->mGeometry = mGeometry->copyGeometry();

   //newObject->setGame(NULL);

   newObject->initializeEditor();         // Unselects all vertices

   return newObject;
}


Color EditorObject::getTeamColor(S32 teamId) 
{ 
   return gEditorUserInterface.getTeamColor(teamId);
}


// Draw the vertices for a polygon or line item (i.e. walls)
void EditorObject::renderLinePolyVertices(F32 currentScale, F32 alpha)
{
   // Draw the vertices of the wall or the polygon area
   for(S32 j = 0; j < getVertCount(); j++)
   {
      Point v = getVert(j);

      if(vertSelected(j))
         renderVertex(SelectedVertex, v, j, currentScale, alpha);             // Hollow yellow boxes with number
      else if(mLitUp && isVertexLitUp(j))
         renderVertex(HighlightedVertex, v, j, currentScale, alpha);          // Hollow yellow boxes with number
      else if(mSelected || mLitUp || anyVertsSelected())
         renderVertex(SelectedItemVertex, v, j, currentScale, alpha);         // Hollow red boxes with number
      else
         renderVertex(UnselectedItemVertex, v, NO_NUMBER, currentScale, alpha, currentScale > 2 ? 2 : 1);   // Solid red boxes, no number
   }
}


void EditorObject::unselect()
{
   setSelected(false);
   setLitUp(false);

   unselectVerts();
}


// Called when item dragged from dock to editor
void EditorObject::newObjectFromDock(F32 gridSize) 
{  
   assignNewSerialNumber();
}   


//void EditorObject::initializePolyGeom()
//{
//   // TODO: Use the same code already in polygon
//   if(getGeomType() == geomPoly)
//   {
//      Triangulate::Process(getVerts(), *getPolyFillPoints());   // Populates fillPoints from polygon outline
//      //TNLAssert(fillPoints.size() > 0, "Bogus polygon geometry detected!");
//
//      setCentroid(findCentroid(getVerts()));
//      setExtent(Rect(getVerts()));
//   }
//
//   forceFieldMountSegment = NULL;
//}


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


void EditorObject::increaseWidth(S32 amt)
{
   S32 width = getWidth();
   width += amt - (S32) width % amt;    // Handles rounding

   setWidth(width);
   onGeomChanged();
}


void EditorObject::decreaseWidth(S32 amt)
{
   S32 width = getWidth();
   width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing

   setWidth(width);
   onGeomChanged();
}


void EditorObject::setWidth(S32 width) 
{         
   // Bounds check
   if(width < Barrier::MIN_BARRIER_WIDTH)
      width = Barrier::MIN_BARRIER_WIDTH;
   else if(width > Barrier::MAX_BARRIER_WIDTH)
      width = Barrier::MAX_BARRIER_WIDTH; 

   mWidth = width; 
}


//// Radius of item in editor -- TODO: Push down to objects
//S32 EditorObject::getEditorRadius(F32 scale)
//{
//   if(getObjectTypeMask() & TestItemType)
//      return TestItem::TEST_ITEM_RADIUS;
//   else if(getObjectTypeMask() & ResourceItemType)
//      return ResourceItem::RESOURCE_ITEM_RADIUS;
//   else if(getObjectTypeMask() & AsteroidType)
//      return S32((F32)Asteroid::ASTEROID_RADIUS * 0.75f);
//   else if(getObjectTypeMask() & SoccerBallItemType)
//      return SoccerBallItem::SOCCER_BALL_RADIUS;
//   else if(getObjectTypeMask() & TurretType && renderFull(getObjectTypeMask(), scale, mDockItem, mSnapped))
//      return 25;
//   else return NONE;    // Use default
//}


Point EditorObject::getEditorSelectionOffset(F32 scale)
{
   return Point(0,0);     // No offset for most items
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

};
