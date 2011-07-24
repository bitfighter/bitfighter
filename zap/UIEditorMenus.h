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

#ifndef _UIEDITORMENUS_H_
#define _UIEDITORMENUS_H_


#include "UIMenus.h"
#include "UIEditor.h"      // For EditorObject

namespace Zap
{

class EditorObject;

// Base class from which all editor attribute menus are descended
class EditorAttributeMenuUI : public MenuUserInterface
{
   typedef MenuUserInterface Parent;
      
protected:
      EditorObject *mObject;      // Object whose attributes are being edited

public:
   EditorAttributeMenuUI(Game *game) : Parent(game) { /* Do nothing */ }    // Constructor
   EditorObject *getObject() { return mObject; }
   virtual void startEditing(EditorObject *object) { mObject = object; }
   virtual void doneEditing(EditorObject *object);
   void render();
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class GoFastEditorAttributeMenuUI : public EditorAttributeMenuUI
{
   typedef EditorAttributeMenuUI Parent;

public:
   GoFastEditorAttributeMenuUI(Game *game);    // Constructor
   void startEditing(EditorObject *object);
   void doneEditing(EditorObject *object);
};


////////////////////////////////////////
////////////////////////////////////////

class TextItemEditorAttributeMenuUI : public EditorAttributeMenuUI
{
   typedef EditorAttributeMenuUI Parent;

public:
   TextItemEditorAttributeMenuUI(Game *game);   // Constructor
   void startEditing(EditorObject *object);
   void doneEditing(EditorObject *object);
};



};

#endif