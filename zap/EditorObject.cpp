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
#include "config.h"

#include "Geometry.h"            // For GeomType enum

#include "UIEditorMenus.h"       // For EditorAttributeMenuUI def

#include "SDL/SDL_opengl.h"

using namespace boost;

namespace Zap
{

S32 EditorObject::mNextSerialNumber = 0;


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

   //setCreationTime(game->getCurrentTime());
   //onAddedToGame(game);
}



// TODO: Merge with copy in editor, if it's really needed
static F32 getRenderingAlpha(bool isScriptItem)
{
   return isScriptItem ? .6f : 1;     // Script items will appear somewhat translucent
}


// TODO: merge with UIEditor versions
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
      F32 txtSize = 6 / currentScale;
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

static void labelVertex(Point pos, F32 radius, const char *itemLabelTop, const char *itemLabelBottom, F32 scale)
{
   F32 labelSize = DOCK_LABEL_SIZE / scale;

   UserInterface::drawStringc(pos.x, pos.y - radius - labelSize - 5, labelSize, itemLabelTop);     // Above the vertex
   UserInterface::drawStringc(pos.x, pos.y + radius + 2, labelSize, itemLabelBottom);              // Below the vertex
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
            renderItemText(txt.c_str(), menuSize - i, currentScale, Point(0,0));
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

         drawSquare(getVert(i), radius / currentScale);
         labelVertex(getVert(i), radius / currentScale, getOnScreenName(), getVertLabel(i), currentScale);
      }         
}


void EditorObject::renderDockItemLabel(const Point &pos, const char *label, F32 yOffset)
{
   F32 xpos = pos.x;
   F32 ypos = pos.y - DOCK_LABEL_SIZE / 2 + yOffset;
   glColor(Colors::white);
   UserInterface::drawStringc(xpos, ypos, (F32)DOCK_LABEL_SIZE, label);
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


// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorObject::renderInEditor(F32 currentScale, const Point &currentOffset, S32 snapIndex, bool isScriptItem, bool showingReferenceShip, ShowMode showMode)
{
   const S32 instrSize = 9;      // Size of instructions for special items
   const S32 attrSize = 10;
   
   Point pos, dest;
   F32 alpha = getRenderingAlpha(isScriptItem);

   bool hideit = (showMode == ShowWallsOnly) && !(showingReferenceShip && !mDockItem);

   Color drawColor;
   if(hideit)
      glColor(Colors::gray50, alpha);
   else 
      glColor(getDrawColor(), alpha);

   glEnableBlend;        // Enable transparency

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
      glPushMatrix();
         glTranslate(currentOffset);
         glScale(currentScale);

         if(showingReferenceShip)
            render();
         else
            renderEditor(currentScale);

         if(!showingReferenceShip)
         {
            // Label item with instruction message describing what happens if user presses enter
            if(isSelected() && !isBeingEdited())
               renderItemText(getInstructionMsg(), -1, currentScale, currentOffset);

            renderAndLabelHighlightedVertices(currentScale);
            renderAttribs(currentScale);
         }

      glPopMatrix();   

   }

   glDisableBlend;
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


// Size of object in editor 
F32 EditorObject::getEditorRadius(F32 currentScale)
{
   return 10 * currentScale;   // 10 pixels is base size
}


// Return a pointer to a new copy of the object.  You will have to delete this copy when you are done with it!
// This is kind of a hack, but not sure of a better way to do this...  perhaps a clone method in each object?
EditorObject *EditorObject::newCopy()
{
   EditorObject *newObject = clone();     // TODO: Wrap in shared_ptr?

   newObject->initializeEditor();         // Unselects all vertices

   return newObject;
}


Color EditorObject::getTeamColor(S32 teamId) 
{ 
   return mGame->getTeamColor(teamId);
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
         renderVertex(UnselectedItemVertex, v, NO_NUMBER, currentScale, alpha, currentScale > 2 ? 2.f : 1.f);   // Solid red boxes, no number
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
//   if(getGeomType() == geomPolygon)
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


////////////////////////////////////////
////////////////////////////////////////

// TODO: merge with simpleLine values, put in editor
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;
//static const Color INSTRUCTION_TEXTCOLOR(1,1,1);      // TODO: Put in editor

// Offset: negative below the item, positive above
void EditorPointObject::renderItemText(const char *text, S32 offset, F32 currentScale, const Point &currentOffset)
{
   glColor(INSTRUCTION_TEXTCOLOR);
   S32 off = (INSTRUCTION_TEXTSIZE + INSTRUCTION_TEXTGAP) * offset - 10 - ((offset > 0) ? 5 : 0);

   Point pos = getVert(0) * currentScale + currentOffset;

   UserInterface::drawCenteredString(pos.x, pos.y - off, INSTRUCTION_TEXTSIZE, text);
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
