//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
#ifndef _EDITOR_ATTRIBUTE_MENU_ITEM_BUILDER_H_
#define _EDITOR_ATTRIBUTE_MENU_ITEM_BUILDER_H_


namespace Zap
{

class BfObject;
class EditorAttributeMenuUI;
class ClientGame;


class EditorAttributeMenuItemBuilder
{
private:
   ClientGame *mGame;
   bool mInitialized;

public:
   EditorAttributeMenuItemBuilder();     // Constructor
   virtual ~EditorAttributeMenuItemBuilder();

   void initialize(ClientGame *game);

   EditorAttributeMenuUI *getAttributeMenu(BfObject *obj);
   static void startEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj);
   static void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu, BfObject *obj);
};


}

#endif
