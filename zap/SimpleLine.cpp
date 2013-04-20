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
#include "Colors.h"
#include "config.h"

#include <math.h>


namespace Zap
{

// Constructor
SimpleLine::SimpleLine()
{ 
   setNewGeometry(geomSimpleLine);
}


// Destructor
SimpleLine::~SimpleLine()
{ 
   // Do nothing
}


S32 SimpleLine::getDockRadius()
{
   return 8;
}


F32 SimpleLine::getEditorRadius(F32 currentScale)
{
   return 7;
}


void SimpleLine::renderDock()
{
#ifndef ZAP_DEDICATED
   drawFilledSquare(getVert(0), 5, &getEditorRenderColor());       // Draw origin of item to give user something to grab on the dock
#endif
}


const F32 INITIAL_ITEM_LENGTH = 1.0;

// Called when we create a brand new object and insert it in the editor, like when we drag a new item from the dock
void SimpleLine::newObjectFromDock(F32 gridSize) 
{
#ifndef ZAP_DEDICATED
   setVert(Point(0,0), 0);
   setVert(Point(1,0) * INITIAL_ITEM_LENGTH * gridSize, 1);

   Parent::newObjectFromDock(gridSize);
#endif
}


// Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
// In this case, we'll drag these items by their slender midriff.
Point SimpleLine::getInitialPlacementOffset(F32 gridSize) 
{ 
   return Point(INITIAL_ITEM_LENGTH * gridSize / 2, 0); 
}


// Draw arrow that serves as the core of SimpleLine items in the editor
// Subclasses will fill in the rest
void SimpleLine::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   renderHeavysetArrow(getVert(0), getVert(1), getEditorRenderColor(), isSelected(), isLitUp());
   renderEditorItem();
#endif
}


void SimpleLine::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
#ifndef ZAP_DEDICATED
   setVert(point, 0);
   Parent::prepareForDock(game, point, teamIndex);
#endif
}


};
