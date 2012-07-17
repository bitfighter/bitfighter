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

#ifndef _SIMPLELINE_H_
#define _SIMPLELINE_H_

#include "EditorObject.h"
//#include "UIEditor.h"      // For EditorObject (to be moved!)
#include "Geometry.h"

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

   virtual void setGeom(const Vector<Point> &points);

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
