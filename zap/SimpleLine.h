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
   virtual const Color &getEditorRenderColor() const = 0;

protected:
   virtual S32 getDockRadius();                       // Size of object on dock
   virtual F32 getEditorRadius(F32 currentScale);     // Size of object (or in this case vertex) in editor

public:
   SimpleLine();           // Constructor
   virtual ~SimpleLine();  // Destructor

   // Some properties about the item that will be needed in the editor
   virtual const char *getOnDockName() = 0;

   void renderDock();                       
   // Child classes will call this
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);

   virtual void newObjectFromDock(F32 gridSize);
   virtual Point getInitialPlacementOffset(U32 gridSize) const;

   void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
};


};

#endif
