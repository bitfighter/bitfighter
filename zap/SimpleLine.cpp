//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "SimpleLine.h"
#include "gameObjectRender.h"


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
   Color color = getEditorRenderColor();
   drawFilledSquare(getVert(0), 5, &color);       // Draw origin of item to give user something to grab on the dock
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
Point SimpleLine::getInitialPlacementOffset(U32 gridSize)  const
{ 
   return Point(INITIAL_ITEM_LENGTH * gridSize / 2, 0); 
}


// Draw arrow that serves as the core of SimpleLine items in the editor
// Subclasses will fill in the rest
void SimpleLine::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
#ifndef ZAP_DEDICATED
   renderHeavysetArrow(getVert(0), getVert(1), getEditorRenderColor(), isSelected(), isLitUp());
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
