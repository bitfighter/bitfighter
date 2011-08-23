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

#include "polygon.h"
#include "GeomUtils.h"           // For polygon triangulation
#include "gameObjectRender.h"
#include "Colors.h"

#ifndef ZAP_DEDICATED
#include "SDL/SDL_opengl.h"
#include "UI.h"
#endif

namespace Zap
{

// TODO: Put in editor ??
static const Color INSTRUCTION_TEXTCOLOR(1,1,1);
static const S32 INSTRUCTION_TEXTSIZE = 9;      
static const S32 INSTRUCTION_TEXTGAP = 3;

// Offset: negative below the item, positive above
void EditorPolygon::renderItemText(const char *text, S32 offset, F32 currentScale, const Point &currentOffset)
{
#ifndef ZAP_DEDICATED
   glColor(INSTRUCTION_TEXTCOLOR);
   S32 off = (INSTRUCTION_TEXTSIZE + INSTRUCTION_TEXTGAP) * offset - 10 - ((offset > 0) ? 5 : 0);
   Point pos = getVert(0) * currentScale + currentOffset;
   UserInterface::drawCenteredString(pos.x, pos.y - off, INSTRUCTION_TEXTSIZE, text);
#endif
}


void EditorPolygon::renderEditor(F32 currentScale)
{
#ifndef ZAP_DEDICATED
   if(mSelected || mLitUp)
      renderPolyHighlight();

   renderLinePolyVertices(currentScale);
#endif
}


void EditorPolygon::renderDock()
{
   renderEditor(1);
}


void EditorPolygon::highlightDockItem() 
{   
   renderPolyHighlight(); 
}  


// TODO: merge with versions in editor
const Color *HIGHLIGHT_COLOR = &Colors::white;
const Color *SELECT_COLOR = &Colors::yellow;


void EditorPolygon::renderPolyHighlight()
{
#ifndef ZAP_DEDICATED
   glLineWidth(gLineWidth3);
   glColor(mSelected ? *SELECT_COLOR : *HIGHLIGHT_COLOR);
   renderPolygonOutline(getOutline());
   glLineWidth(gDefaultLineWidth);
#endif
}


void EditorPolygon::labelDockItem()
{
#ifndef ZAP_DEDICATED
   renderDockItemLabel(getCentroid(), getOnDockName(), -2);
#endif
}


void EditorPolygon::prepareForDock(Game *game, const Point &point)
{
#ifndef ZAP_DEDICATED
   F32 h = 16;    // Entire height
   F32 w = 20;    // Half the width

   clearVerts();
   addVert(point + Point(-w, 0)); 
   addVert(point + Point( w, 0)); 
   addVert(point + Point( w, h)); 
   addVert(point + Point(-w, h)); 

   EditorObject::prepareForDock(game, point);
#endif
}


static const F32 INITIAL_HEIGHT = 0.9f;
static const F32 INITIAL_WIDTH = 0.3f;

// Called when we create a brand new object and insert it in the editor, like when we drag a new item from the dock
void EditorPolygon::newObjectFromDock(F32 gridSize)
{
#ifndef ZAP_DEDICATED
   EditorParent::newObjectFromDock(gridSize);

   F32 w = INITIAL_HEIGHT * gridSize / 2;
   F32 h = INITIAL_WIDTH * gridSize / 2;

   setVert(Point(-w, -h), 0);
   setVert(Point(-w,  h), 1);
   setVert(Point( w,  h), 2);
   setVert(Point( w, -h), 3);
#endif
}


// Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
Point EditorPolygon::getInitialPlacementOffset(F32 gridSize)
{ 
   return Point(INITIAL_HEIGHT * gridSize / 2, INITIAL_WIDTH * gridSize / 2); 
}


};

