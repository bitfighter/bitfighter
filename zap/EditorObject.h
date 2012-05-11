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


#ifndef _EDITOROBJECT_H_
#define _EDITOROBJECT_H_

#include "gameObject.h"          // We inherit from this -- for BfObject, for now

#include "Point.h"
#include "tnlVector.h"
#include "Geometry.h"

using namespace std;
using namespace TNL;

namespace Zap
{

class EditorAttributeMenuUI;
class WallSegment;
class ClientGame;

class EditorObject : virtual public BfObject   // Interface class  -- All editor objects need to implement this
{
protected:
   S32 mItemId;         // Item's unique id... 0 if there is none

public:
   EditorObject();                  // Constructor
   virtual ~EditorObject();         // Virtual destructor
   virtual EditorObject *clone() const;

   EditorObject *copy();            // Makes a duplicate of the item (see method for explanation)
   EditorObject *newCopy();         // Creates a brand new object based on the current one (see method for explanation)

   // Account for the fact that the apparent selection center and actual object center are not quite aligned
   virtual Point getEditorSelectionOffset(F32 currentScale);  

#ifndef ZAP_DEDICATED
   void renderAndLabelHighlightedVertices(F32 currentScale);      // Render selected and highlighted vertices, called from renderEditor
#endif
   virtual void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);


   EditorObjectDatabase *getEditorObjectDatabase();

   void setSnapped(bool snapped);                  // Overridden in EngineeredItem 

   // Objects can be different sizes on the dock and in the editor.  We need to draw selection boxes in both locations,
   // and these functions specify how big those boxes should be.  Override if implementing a non-standard sized item.
   // (strictly speaking, only getEditorRadius needs to be public, but it make sense to keep these together organizationally.)
   virtual S32 getDockRadius();                     // Size of object on dock
   virtual F32 getEditorRadius(F32 currentScale);   // Size of object in editor
   virtual string getAttributeString();             // Used for displaying text in lower-left in editor

   virtual string toString(F32 gridSize) const = 0; // Generates levelcode line for object      --> TODO: Rename to toLevelCode()?

   ///// Dock related
#ifndef ZAP_DEDICATED
   virtual void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
#endif
   virtual void newObjectFromDock(F32 gridSize);   // Called when item dragged from dock to editor -- overridden by several objects
   // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement seem more natural.
   virtual Point getInitialPlacementOffset(F32 gridSize);

   ///// Dock item rendering methods
   virtual void renderDock();   
   virtual Point getDockLabelPos();
   virtual void highlightDockItem();

   //////
   // Things are happening in the editor; the editor must respond!
   virtual void onGeomChanging();      // Item geom is interactively changing
   virtual void onGeomChanged();       // Item changed geometry (or moved), do any internal updating that might be required

   virtual void onItemDragging();      // Item is being dragged around the screen

   virtual void onAttrsChanging();     // Attr is in the process of being changed (i.e. a char was typed for a textItem)
   virtual void onAttrsChanged();      // Attrs changed

   /////

   S32 getItemId();
   void setItemId(S32 itemId);
   
   virtual void initializeEditor();

   virtual const char *getOnScreenName();
   virtual const char *getPrettyNamePlural();
   virtual const char *getOnDockName();
   virtual const char *getEditorHelpString();

   virtual const char *getInstructionMsg();                                // Message printed below item when it is selected

   // For editing attributes:
   virtual EditorAttributeMenuUI *getAttributeMenu();                      // Override in child if it has an attribute menu
   virtual void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);   // Called when we start editing to get menus populated
   virtual void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we're done to retrieve values set by the menu
};


////////////////////////////////////////
////////////////////////////////////////

// Class with editor methods related to point things

class PointObject : public EditorObject
{
   typedef EditorObject Parent;

public:
   PointObject();             // Constructor
   virtual ~PointObject();    // Destructor

   void prepareForDock(ClientGame *game, const Point &point, S32 teamIndex);
};


};
#endif
