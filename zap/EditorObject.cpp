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

#include "Geometry.h"            // For GeomType enum

#include "UIEditorMenus.h"       // For EditorAttributeMenuUI def


namespace Zap
{

S32 EditorObject::mNextSerialNumber = 0;


inline F32 getGridSize()
{
   return gEditorGame->getGridSize();
}


void EditorObject::addToEditor(Game *game)
{
   BfObject::addToGame(game);
   gEditorUserInterface.mItems.push_back(this);    // TODO: get rid of this, make all items come from the database
}


void EditorObject::addToDock(Game *game, const Point &point)
{
   mGame = game;
   mDockItem = true;
   
   unselectVerts();

   gEditorUserInterface.mDockItems.push_back(this);
}


void EditorObject::processEndPoints()
{
   if(getObjectTypeMask() & BarrierType)
      Barrier::constructBarrierEndPoints(getOutline(), getWidth() / getGridSize(), extendedEndPoints);

   else if(getObjectTypeMask() & PolyWallType)
   {
      extendedEndPoints.clear();
      for(S32 i = 1; i < getVertCount(); i++)
      {
         extendedEndPoints.push_back(getVert(i-1));
         extendedEndPoints.push_back(getVert(i));
      }

      // Close the loop
      extendedEndPoints.push_back(getVert(getVertCount()));
      extendedEndPoints.push_back(getVert(0));
   }
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
static const Color grayedOutColorBright = Color(.5, .5, .5);
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
      glColor(HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(magenta, alpha);
   else
      glColor(red, alpha);

   drawSquare(v, size / currentScale, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(white, alpha);
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
static const Color DOCK_LABEL_COLOR = white;


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
         glColor((vertSelected(i) || mSelected) ? SELECT_COLOR: HIGHLIGHT_COLOR);

         Point pos = gEditorUserInterface.convertLevelToCanvasCoord(getVert(i));

         drawSquare(pos, radius);
         labelVertex(pos, radius, getOnScreenName(), getVertLabel(i));
      }         
}


void EditorObject::renderDockItemLabel(const Point &pos, const char *label, F32 yOffset)
{
   F32 xpos = pos.x;
   F32 ypos = pos.y - DOCK_LABEL_SIZE / 2 + yOffset;
   glColor(DOCK_LABEL_COLOR);
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
      drawColor = SELECT_COLOR;
     
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


   //////////

   //else if(getObjectTypeMask() & ItemLineItem)
   //{
   //   glColor(getTeamColor(getTeam()), alpha);
   //   renderPolylineCenterline(alpha);

   //   if(!showingReferenceShip)
   //      renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);
   //}

   ////////////

   //else if(getObjectTypeMask() & ItemBarrierMaker)      
   //{
   //   if(!showingReferenceShip && getObjectTypeMask() & ItemBarrierMaker)
   //      renderPolylineCenterline(alpha);

   //   if(!showingReferenceShip)
   //      renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);
   //} 
   //else if(getGeomType() == geomPoly)    // Draw regular line objects and poly objects
   //{
   //   // Hide everything in ShowWallsOnly mode, and hide navMeshZones in ShowAllButNavZones mode, 
   //   // unless it's a dock item or we're showing the reference ship.  NavMeshZones are hidden when reference ship is shown
   //   if((showMode != ShowWallsOnly && (gEditorUserInterface.showingNavZones() && getObjectTypeMask() & ItemNavMeshZone || getObjectTypeMask() & ~ItemNavMeshZone)) &&
   //         !showingReferenceShip || mDockItem || showingReferenceShip && getObjectTypeMask() & ~ItemNavMeshZone)   
   //   {
   //      // A few items will get custom colors; most will get their team color
   //      if(hideit)
   //         glColor(grayedOutColorDim, alpha);
   //      else if(getObjectTypeMask() & ItemNexus)
   //         glColor(gNexusOpenColor, alpha);      // Render Nexus items in pale green to match the actual thing
   //      else if(getObjectTypeMask() & ItemPolyWall)
   //         glColor(EDITOR_WALL_FILL_COLOR);
   //      else
   //         glColor(getTeamColor(getTeam()), alpha);


   //      F32 ang = angleOfLongestSide(mVerts);

   //      if(mDockItem)    // Old school rendering on the dock
   //      {
   //         glPushMatrix();
   //            setLevelToCanvasCoordConversion();

   //            // Render the fill triangles
   //            renderTriangulatedPolygonFill(*getPolyFillPoints());

   //            glColor(hideit ? grayedOutColorBright : drawColor, alpha);
   //            glLineWidth(gLineWidth3);  
   //            renderPolygonOutline(mVerts);
   //            glLineWidth(gDefaultLineWidth);        // Restore line width
   //         glPopMatrix();

   //         // Let's add a label
   //         glColor(hideit ? grayedOutColorBright : drawColor, alpha);
   //         renderPolygonLabel(convertLevelToCanvasCoord(getCentroid(), !mDockItem), ang, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName());
   //      }
   //      else     // Not a dock item
   //      {
   //         glPushMatrix();  
   //            setLevelToCanvasCoordConversion();

   //            if(getObjectTypeMask() & ItemLoadoutZone)
   //               renderLoadoutZone(getTeamColor(getTeam()), mVerts, *getPolyFillPoints(), 
   //                                 getCentroid() * getGridSize(), ang, 1 / getGridSize());

   //            else if(getObjectTypeMask() & ItemGoalZone)
   //               renderGoalZone(getTeamColor(getTeam()), mVerts, *getPolyFillPoints(),  
   //                                 getCentroid() * getGridSize(), ang, false, 0, getScore(), 1 / getGridSize());

   //            else if(getObjectTypeMask() & ItemNexus)
   //               renderNexus(getVerts(), *getPolyFillPoints(), 
   //                                 getCentroid() * getGridSize(), ang, true, 0, 1 / getGridSize());

   //            else if(getObjectTypeMask() & ItemNavMeshZone)
   //               renderNavMeshZone(getVerts(), *getPolyFillPoints(), getCentroid(), showMode == NavZoneMode ? -2 : -1, 
   //                                 true, mSelected);

   //            else if(getObjectTypeMask() & ItemSlipZone)
   //               renderSlipZone(mVerts, *getPolyFillPoints(), getExtent());


   //            //else if(item.getObjectTypeMask() & ItemBarrierMaker)
   //            //   renderPolygon(item.fillPoints, item->getVerts(), gIniSettings.wallFillColor, gIniSettings.wallOutlineColor, 1);

   //            // If item is selected, and we're not in preview mode, draw a border highlight
   //            if(!showingReferenceShip && (mSelected || mLitUp || (gEditorUserInterface.mDraggingObjects && mAnyVertsSelected)))
   //            {        
   //               glColor(hideit ? grayedOutColorBright : drawColor, alpha);
   //               glLineWidth(gLineWidth3);  
   //               renderPolygonOutline(mVerts);
   //               glLineWidth(gDefaultLineWidth);        // Restore line width
   //            }

   //         glPopMatrix();
   //      }
   //   }

   //   // NavMeshZone verts will be drawn elsewhere
   //   if((getGeomType() == geomLine || showMode != ShowWallsOnly) && 
   //               !mDockItem && !showingReferenceShip)  
   //      renderLinePolyVertices(gEditorUserInterface.getCurrentScale(), alpha);                               
   //}
 
   //if(showMode != ShowWallsOnly ||  mDockItem || showingReferenceShip)   // Draw the various point items
   //{
   //   Color c = hideit ? grayedOutColorDim : getTeamColor(getTeam());           // And a color (based on team affiliation)

   //   if(getObjectTypeMask() & ItemFlag)             // Draw flag
   //   {
   //      glPushMatrix();
   //         glTranslatef(pos.x, pos.y, 0);
   //         glScalef(0.6, 0.6, 1);
   //         renderFlag(0, 0, c, hideit ? &grayedOutColorDim : NULL, alpha);
   //      glPopMatrix();
   //   }
   //   else if(getObjectTypeMask() & ItemFlagSpawn)    // Draw flag spawn point
   //   {
   //      if(showingReferenceShip && !mDockItem)
   //      {
   //         // Do nothing -- hidden in preview mode
   //      }
   //      else
   //      {
   //         glPushMatrix();
   //            glTranslatef(pos.x + 1, pos.y, 0);
   //            glScalef(0.4, 0.4, 1);
   //            renderFlag(0, 0, c, hideit ? &grayedOutColorDim : NULL, alpha);

   //            glColor(hideit ? grayedOutColorDim : white, alpha);
   //            drawCircle(-4, 0, 26);
   //         glPopMatrix();
   //      }
   //   }
   //   else if(getObjectTypeMask() & ItemAsteroidSpawn)    // Draw asteroid spawn point
   //   {
   //      if(showingReferenceShip && !mDockItem)
   //      {
   //         // Do nothing -- hidden in preview mode
   //      }
   //      else
   //      {
   //         glPushMatrix();
   //            glTranslatef(pos.x, pos.y, 0);
   //            glScalef(0.8, 0.8, 1);
   //            renderAsteroid(Point(0,0), asteroidDesign, .1, hideit ? &grayedOutColorDim : NULL, alpha);

   //            glColor(hideit ? grayedOutColorDim : white, alpha);
   //            drawCircle(0, 0, 13);
   //         glPopMatrix();
   //      }
   //   }
   //   else if(getObjectTypeMask() & ItemBouncyBall)   // Draw testitem
   //   {
   //      if(!mDockItem)
   //      {
   //         glPushMatrix();
   //            gEditorUserInterface.setTranslationAndScale(pos);
   //            renderTestItem(pos, alpha);
   //         glPopMatrix();
   //      }
   //      else     // Dock item rendering
   //      {
   //         glColor(hideit ? grayedOutColorBright : Color(1,1,0), alpha);
   //         drawPolygon(pos, 7, 8, 0);
   //      }
   //   }
   //   else if(getObjectTypeMask() & ItemAsteroid)   // Draw asteroid
   //   {
   //      if(!mDockItem)
   //      {
   //         glPushMatrix();
   //            gEditorUserInterface.setTranslationAndScale(pos);
   //            renderAsteroid(pos, asteroidDesign, asteroidRenderSize[0], hideit ? &grayedOutColorDim : NULL, alpha);
   //         glPopMatrix();
   //      }
   //      else     // Dock item rendering
   //         renderAsteroid(pos, asteroidDesign, .1, hideit ? &grayedOutColorDim : NULL, alpha);
   //   }

   //   else if(getObjectTypeMask() & ItemResource)   // Draw resourceItem
   //   {
   //      if(!mDockItem)
   //      {
   //         glPushMatrix();
   //            gEditorUserInterface.setTranslationAndScale(pos);
   //            renderResourceItem(pos, alpha);
   //         glPopMatrix();
   //      }
   //      else     // Dock item rendering
   //          renderResourceItem(pos, .4, hideit ? &grayedOutColorDim : NULL, alpha);
   //   }
   //   else if(getObjectTypeMask() & ItemSoccerBall)  // Soccer ball, obviously
   //   {
   //      if(!mDockItem)
   //      {
   //         glPushMatrix();
   //            gEditorUserInterface.setTranslationAndScale(pos);
   //            renderSoccerBall(pos, alpha);
   //         glPopMatrix();
   //      }
   //      else
   //      {
   //         glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7), alpha);
   //         drawCircle(pos, 9);
   //      }
   //   }
   //   else if(getObjectTypeMask() & ItemMine)  // And a mine
   //   {
   //      if(showingReferenceShip && !mDockItem) 
   //      {
   //          glPushMatrix();
   //            gEditorUserInterface.setTranslationAndScale(pos);
   //            renderMine(pos, true, true);
   //         glPopMatrix();
   //      }
   //      else
   //      {
   //         glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
   //         drawCircle(pos, 9 - (mDockItem ? 2 : 0));

   //         glColor(hideit ? grayedOutColorDim : Color(.1,.3,.3), alpha);
   //         drawCircle(pos, 5 - (mDockItem ? 1 : 0));

   //         drawLetter('M', pos, hideit ? grayedOutColorBright : drawColor, alpha);
   //      }
   //   }
   //   else if(getObjectTypeMask() & ItemSpyBug)  // And a spy bug
   //   {
   //      glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
   //      drawCircle(pos, 9 - (mDockItem ? 2 : 0));

   //      glColor(hideit ? grayedOutColorDim : getTeamColor(getTeam()), alpha);
   //      drawCircle(pos, 5 - (mDockItem ? 1 : 0));

   //      drawLetter('S', pos, hideit ? grayedOutColorBright : drawColor, alpha);

   //      // And show how far it can see... unless, of course, it's on the dock, and assuming the tab key has been pressed
   //      if(!mDockItem && showingReferenceShip && (mSelected || mLitUp))
   //      {
   //         glColor(getTeamColor(getTeam()), .25 * alpha);

   //         F32 size = getCurrentScale() / getGridSize() * F32(gSpyBugRange);

   //         drawFilledSquare(pos, size);
   //      }
   //   }

   //   else if(getObjectTypeMask() & ItemRepair)
   //      renderRepairItem(pos, true, hideit ? &grayedOutColorDim : NULL, alpha);

   //   else if(getObjectTypeMask() & ItemEnergy)
   //      renderEnergyItem(pos, true, hideit ? &grayedOutColorDim : NULL, alpha);

   //   else if(getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField)
   //   { 
   //      if(renderFull(getObjectTypeMask(), getCurrentScale(), mDockItem, mSnapped))      
   //      {
   //         if(getObjectTypeMask() & ItemTurret)
   //         {
   //            glPushMatrix();
   //               gEditorUserInterface.setTranslationAndScale(pos);
   //               renderTurret(c, pos, mAnchorNormal, true, 1.0, mAnchorNormal.ATAN2());
   //            glPopMatrix();
   //         }
   //         else   
   //         {
   //            glPushMatrix();
   //               gEditorUserInterface.setTranslationAndScale(pos);
   //               renderForceFieldProjector(pos, mAnchorNormal, c, true);
   //            glPopMatrix();

   //            F32 scaleFact = 1 / getGridSize(); 

   //            glPushMatrix();
   //               setLevelToCanvasCoordConversion();

   //               renderForceField(ForceFieldProjector::getForceFieldStartPoint(getVert(0), mAnchorNormal, scaleFact), 
   //                                forceFieldEnd, c, true, scaleFact);
   //            glPopMatrix();
   //         }
   //      }
   //      else
   //         renderGenericItem(pos, c, alpha, hideit ? grayedOutColorBright : drawColor, getObjectTypeMask() & ItemTurret ? 'T' : '>');  
   //   }
   //   else if(getObjectTypeMask() & ItemSpawn)
   //      renderGenericItem(pos, c, alpha, hideit ? grayedOutColorBright : drawColor, 'S');  




   //   // If this is an item that has a repop attribute, and the item is selected, draw the text
   //   if(!mDockItem && getHasRepop())
   //   {
   //      if(showMode != ShowWallsOnly && 
   //         ((mSelected || mLitUp) && !gEditorUserInterface.isEditingSpecialAttrItem()) &&
   //         (getObjectTypeMask() & ~ItemFlagSpawn || !strcmp(gEditorUserInterface.mGameType, "HuntersGameType")) || isBeingEdited())
   //      {
   //         glColor(white);

   //         const char *healword = (getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField) ? "10% Heal" : 
   //                                ((getObjectTypeMask() & ItemFlagSpawn || getObjectTypeMask() & ItemAsteroidSpawn) ? "Spawn Time" : "Regen");

   //         Point offset = getEditorSelectionOffset(getCurrentScale()).rotate(mAnchorNormal.ATAN2()) * getCurrentScale();

   //         S32 radius = getEditorRadius(getCurrentScale());
   //         offset.y += ((radius == NONE || mDockItem) ? 10 : (radius * getCurrentScale() / getGridSize())) - 6;

   //         if(repopDelay == 0)
   //            UserInterface::drawStringfc(pos.x + offset.x, pos.y + offset.y + attrSize, attrSize, "%s: Disabled", healword);
   //         else
   //            UserInterface::drawStringfc(pos.x + offset.x, pos.y + offset.y + 10, attrSize, "%s: %d sec%c", 
   //                                        healword, repopDelay, repopDelay != 1 ? 's' : 0);


   //         const char *msg;

   //         if(gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::NoAttribute))
   //            msg = "[Enter] to edit";
   //         else if(isBeingEdited() && gEditorUserInterface.isEditingSpecialAttribute(EditorUserInterface::RepopDelay))
   //            msg = "Up/Dn to change";
   //         else
   //            msg = "???";
   //         UserInterface::drawStringc(pos.x + offset.x, pos.y + offset.y + instrSize + 13, instrSize, msg);
   //      }
   //   }

   //   // If we have a turret, render it's range (if tab is depressed)
   //   if(getObjectTypeMask() & ItemTurret)
   //   {
   //      if(!mDockItem && showingReferenceShip && (mSelected || mLitUp))
   //      {
   //         glColor(getTeamColor(getTeam()), .25 * alpha);

   //         F32 size = getCurrentScale() / getGridSize() * (gWeapons[WeaponTurret].projLiveTime * gWeapons[WeaponTurret].projVelocity / 1000);
   //         drawFilledSquare(pos, size);
   //      }
   //   }
   //}

   glDisableBlend;
}

      // Draw highlighted border around item if selected
      //if(showMode != ShowWallsOnly && (mSelected || mLitUp))  
      //{
      //   // Dock items are never selected, but they can be highlighted
      //   Point pos = mDockItem ? getVert(0) : convertLevelToCanvasCoord(getVert(0));   

      //   glColor(drawColor);

      //   S32 radius = getEditorRadius(getCurrentScale());
      //   S32 highlightRadius = (radius == NONE || mDockItem) ? 10 : S32(radius * getCurrentScale() / getGridSize() + 0.5f);

      //   Point ctr = pos + getEditorSelectionOffset(getCurrentScale()).rotate(mAnchorNormal.ATAN2()) * getCurrentScale();   

      //   drawSquare(ctr, highlightRadius);
      //}

   //   // Add a label if we're hovering over it (or not, unless it's on the dock, where we've already labeled our items)
   //   // For the moment, we need special handling for turrets & forcefields :-(
   //   if(showMode != ShowWallsOnly && (mSelected || mLitUp) && 
   //         getOnScreenName() && !mDockItem &&
   //         !((getObjectTypeMask() & ItemTurret || getObjectTypeMask() & ItemForceField) && renderFull(getObjectTypeMask(), getCurrentScale(), mDockItem, mSnapped)))
   //   {
   //      glColor(drawColor);
   //      //UserInterface::drawStringc(pos.x, pos.y - EditorUserInterface::DOCK_LABEL_SIZE * 2 - 5, EditorUserInterface::DOCK_LABEL_SIZE, getOnScreenName()); // Label on top
   //   }
   // }

   //// Label our dock items
   //if(mDockItem && getGeomType() != geomPoly)      // Polys are already labeled internally
   //{
   //   glColor(hideit ? grayedOutColorBright : drawColor);
   //   F32 maxy = -F32_MAX;
   //   for(S32 j = 0; j < getVertCount(); j++)
   //      if(getVert(j).y > maxy)
   //         maxy = getVert(j).y;

   //   // Make some label position adjustments
   //   if(getGeomType() == geomSimpleLine)
   //      maxy -= 2;
   //   else if(getObjectTypeMask() & ItemSoccerBall)
   //      maxy += 1;

   //   F32 xpos = pos.x - UserInterface::getStringWidth(EditorUserInterface::DOCK_LABEL_SIZE, getOnDockName())/2;
   //   UserInterface::drawString(xpos, maxy + 8, EditorUserInterface::DOCK_LABEL_SIZE, getOnDockName());
   //}


// Draw barrier centerlines; wraps renderPolyline()  ==> lineItem, barrierMaker only
void EditorObject::renderPolylineCenterline(F32 alpha)
{
   // Render wall centerlines
   if(mSelected)
      glColor(SELECT_COLOR, alpha);
   else if(mLitUp && !anyVertsSelected())
      glColor(HIGHLIGHT_COLOR, alpha);
   else
      glColor(getTeamColor(mTeam), alpha);

   glLineWidth(WALL_SPINE_WIDTH);
   EditorUserInterface::renderPolyline(getOutline());
   glLineWidth(gDefaultLineWidth);
}


void EditorObject::initializeEditor(F32 gridSize)
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
      return SELECT_COLOR;       // yellow
   else if(mLitUp)
      return HIGHLIGHT_COLOR;    // white
   else  // Normal
      return Color(.75, .75, .75);
}


void EditorObject::saveItem(FILE *f)
{
   s_fprintf(f, "%s\n", toString().c_str());
}


// Helper function for newCopy()
static EditorObject *getNewEditorObject(EditorObject *obj)
{
   TextItem *textItem = dynamic_cast<TextItem *>(obj);
   if(textItem != NULL)
      return new TextItem(*textItem);

   Teleporter *teleporter = dynamic_cast<Teleporter *>(obj);
   if(teleporter != NULL)
      return new Teleporter(*teleporter);

   SpeedZone *speedZone = dynamic_cast<SpeedZone *>(obj);
   if(speedZone != NULL)
      return new SpeedZone(*speedZone);

   FlagItem *flagItem = dynamic_cast<FlagItem *>(obj);
   if(flagItem != NULL)
      return new FlagItem(*flagItem);

   FlagSpawn *flagSpawn = dynamic_cast<FlagSpawn *>(obj);
   if(flagSpawn != NULL)
      return new FlagSpawn(*flagSpawn);

   RepairItem *repairItem = dynamic_cast<RepairItem *>(obj);
   if(repairItem != NULL)
      return new RepairItem(*repairItem);

   TestItem *testItem = dynamic_cast<TestItem *>(obj);
   if(testItem != NULL)
      return new TestItem(*testItem);

   ResourceItem *resourceItem = dynamic_cast<ResourceItem *>(obj);
   if(resourceItem != NULL)
      return new ResourceItem(*resourceItem);

   Asteroid *asteroid = dynamic_cast<Asteroid *>(obj);
   if(asteroid != NULL)
      return new Asteroid(*asteroid);

   AsteroidSpawn *asteroidSpawn = dynamic_cast<AsteroidSpawn *>(obj);
   if(asteroidSpawn != NULL)
      return new AsteroidSpawn(*asteroidSpawn);

   Mine *mine = dynamic_cast<Mine *>(obj);
   if(mine != NULL)
      return new Mine(*mine);

   Spawn *shipSpawn = dynamic_cast<Spawn *>(obj);
   if(shipSpawn != NULL)
      return new Spawn(*shipSpawn);

   SpyBug *spyBug = dynamic_cast<SpyBug *>(obj);
   if(spyBug != NULL)
      return new SpyBug(*spyBug);

   LoadoutZone *loadoutZone = dynamic_cast<LoadoutZone *>(obj);
   if(loadoutZone != NULL)
      return new LoadoutZone(*loadoutZone);

   GoalZone *goalZone = dynamic_cast<GoalZone *>(obj);
   if(goalZone != NULL)
      return new GoalZone(*goalZone);

   HuntersNexusObject *nexus = dynamic_cast<HuntersNexusObject *>(obj);
   if(nexus != NULL)
      return new HuntersNexusObject(*nexus);

   Turret *turret = dynamic_cast<Turret *>(obj);
   if(turret != NULL)
      return new Turret(*turret);

   ForceFieldProjector *projector = dynamic_cast<ForceFieldProjector *>(obj);
   if(projector != NULL)
      return new ForceFieldProjector(*projector);

   WallItem *wallItem = dynamic_cast<WallItem *>(obj);
   if(wallItem != NULL)
      return new WallItem(*wallItem);

   PolyWall *polyWall = dynamic_cast<PolyWall *>(obj);
   if(polyWall != NULL)
      return new PolyWall(*polyWall);


   TNLAssert(false, "OBJECT NOT HANDLED IN COPY OPERATION!");

   return NULL;   
}


// Return a pointer to a new copy of the object.  You will have to delete this copy when you are done with it!
// This is kind of a hack, but not sure of a better way to do this...  perhaps a clone method in each object?
EditorObject *EditorObject::newCopy()
{
   EditorObject *newObject = getNewEditorObject(this);

   if(newObject)
   {
      newObject->setGame(NULL);         // mGame pointer will have been copied, but needs to be cleared before we can add this to the game
      newObject->mGeometry = mGeometry->copyGeometry();
      newObject->initializeEditor(getGridSize());
   }

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

   if(width > Barrier::MAX_BARRIER_WIDTH)
      width = Barrier::MAX_BARRIER_WIDTH;

   setWidth(width);

   onGeomChanged();
}


void EditorObject::decreaseWidth(S32 amt)
{
   S32 width = getWidth();
   
   width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing

   if(width < Barrier::MIN_BARRIER_WIDTH)
      width = Barrier::MIN_BARRIER_WIDTH;

   setWidth(width);

   onGeomChanged();
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