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

#include "SimpleLine.h"
#include "gameObjectRender.h"
#include <math.h>

namespace Zap
{

// Constructor
SimpleLine::SimpleLine()
{
   // Do nothing
}


void SimpleLine::renderDock()
{
   glColor(getEditorRenderColor());       // Blue for TextItem, red for GoFast, etc.
   drawFilledSquare(getVert(0), 5);       // Draw origin of item to give user something to grab on the dock
}


void SimpleLine::initializeEditor(F32 gridSize) 
{
   EditorParent::initializeEditor(gridSize);
   setVert(Point(0,0), 0);
   setVert(Point(1,0) * gridSize, 1);
}


// TODO: Put in editor ??
static const Color INSTRUCTION_TEXTCOLOR(1,1,1);
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;
static const Color ACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);    
static const Color INACTIVE_SPECIAL_ATTRIBUTE_COLOR = Color(.6, .6, .6);      // already in editor, called inactiveSpecialAttributeColor

static const Color SELECT_COLOR = yellow;

// Draw arrow that serves as the core of SimpleLine items in the editor
// Subclasses will fill in the rest
void SimpleLine::renderEditor(F32 currentScale)
{
   Point pos = getVert(0);
   Point dest = getVert(1);

   for(S32 i = 1; i >= 0; i--)
   {
      // Draw heavy colored line with colored core
      glLineWidth(i ? gLineWidth4 : gDefaultLineWidth);                
      glColor(getEditorRenderColor(), i ? .35 : 1);         // Get color from child class

      F32 ang = pos.angleTo(dest);
      const F32 al = 15;                // Length of arrow-head, in editor units (15 pixels)
      const F32 angoff = .5;            // Pitch of arrow-head prongs

      glBegin(GL_LINES);
         glVertex2f(dest.x, dest.y);    // Draw arrowhead
         glVertex2f(dest.x - cos(ang + angoff) * al, dest.y - sin(ang + angoff) * al);
         glVertex2f(dest.x, dest.y);
         glVertex2f(dest.x - cos(ang - angoff) * al, dest.y - sin(ang - angoff) * al);

         // Draw highlighted core on 2nd pass if item is selected, but not while it's being edited
         if(!i && (mSelected || mLitUp))
            glColor(SELECT_COLOR);

         glVertex(pos);                 // Draw connecting line
         glVertex(dest);
      glEnd();
   }

   renderEditorItem();
}


// Offset: negative below the item, positive above
void SimpleLine::renderItemText(const char *text, S32 offset, F32 currentScale)
{
   glColor(INSTRUCTION_TEXTCOLOR);
   S32 off = (INSTRUCTION_TEXTSIZE + INSTRUCTION_TEXTGAP) * offset - 10 - ((offset > 0) ? 5 : 0);
   UserInterface::drawStringf_2pt(convertLevelToCanvasCoord(getVert(0)), 
                                  convertLevelToCanvasCoord(getVert(1)), 
                                  INSTRUCTION_TEXTSIZE, off, text);
}


void SimpleLine::addToDock(Game *game, const Point &point)
{
   setVert(point, 0);
   EditorParent::addToDock(game, point);
}


};