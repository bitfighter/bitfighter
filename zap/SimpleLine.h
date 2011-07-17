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

namespace Zap
{

class SimpleLine : public EditorObject, public GameObject
{
   typedef GameObject Parent;
   typedef EditorObject EditorParent;

private:
   virtual Color getEditorRenderColor() = 0;

   virtual const char *getVertLabel(S32 index) = 0;          

protected:
   virtual S32 getDockRadius() { return 8; }                       // Size of object on dock
   virtual F32 getEditorRadius(F32 currentScale) { return 7; }     // Size of object (or in this case vertex) in editor

public:
   SimpleLine(GameObjectType objectType = UnknownType);       // Constructor

   // Some properties about the item that will be needed in the editor
   virtual const char *getOnDockName() = 0;

   void renderDock();     // Render item on the dock
   void renderEditor(F32 currentScale);
   virtual void renderEditorItem() = 0;
   void renderItemText(const char *text, S32 offset, F32 currentScale);

   virtual void newObjectFromDock(F32 gridSize);

   void addToDock(Game *game, const Point &point);
};


};

#endif