//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SIMPLELINE_H_
#define _SIMPLELINE_H_


#include "BfObject.h"      // Parent

namespace Zap
{

class SimpleLine : public BfObject
{
   typedef BfObject Parent;

private:
   virtual Color getEditorRenderColor() = 0;

protected:
   virtual S32 getDockRadius();                       // Size of object on dock
   virtual F32 getEditorRadius(F32 currentScale);     // Size of object (or in this case vertex) in editor

public:
   SimpleLine();           // Constructor
   virtual ~SimpleLine();  // Destructor

   // Some properties about the item that will be needed in the editor
   virtual const char *getOnDockName() = 0;

   void renderDock();                       
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);     
   virtual void renderEditorItem() = 0;      // Helper for renderEditor

   virtual void newObjectFromDock(F32 gridSize);
   virtual Point getInitialPlacementOffset(F32 gridSize);

   void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
};


};

#endif
