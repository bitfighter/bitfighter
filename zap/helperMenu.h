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

#ifndef _HELPERMENU_H_
#define _HELPERMENU_H_

#include "InputCode.h"
#include "tnl.h"
#include "Color.h"
#include "Timer.h"

using namespace TNL; 


namespace Zap
{

class ClientGame;
class UIManager;

class HelperMenu
{
public:
   enum HelperMenuType {
      ChatHelperType,
      QuickChatHelperType,
      LoadoutHelperType,
      EngineerHelperType,
      ShuffleTeamsHelperType
   };

private:
   virtual const char *getCancelMessage();
   virtual InputCode getActivationKey();

   bool mShowing;    // True when menu is being activated, false when deactivating

   ClientGame *mClientGame;

   virtual F32 getHelperWidth() const;

protected:
   static const S32 MENU_TOP = 180;     // Location of top of overlay menu

   Timer mShowTimer;

   S32 getLeftEdgeOfMenuPos();    // Return left edge of menu

   // Shortcut helper function
   virtual void exitHelper();

   void drawMenuBorderLine(S32 xPos, S32 yPos, const Color &color);
   void drawMenuCancelText(S32 xPos, S32 yPos, const Color &color, S32 fontSize);

   ClientGame *getGame();


public:
   explicit HelperMenu(ClientGame *clientGame);    // Constructor
   virtual ~HelperMenu();                          // Destructor

   virtual void render() = 0;
   virtual void idle(U32 delta);
   virtual void onMenuShow();

   bool isClosing() const;                         // Return true if menu is playing the closing animation
   F32 getFraction();                              // Get fraction of openness


   virtual bool processInputCode(InputCode inputCode);  
   virtual void onTextInput(char ascii);

   virtual void activateHelp(UIManager *uiManager);   // Open help to an appropriate page
   virtual bool isEngineerHelper();                   // Returns false, overridden by EngineerHelper
   virtual bool isChatHelper();
   virtual bool isMovementDisabled();                 // Is ship movement disabled while this helper is active?
   virtual bool isChatDisabled();                     // Returns true if chat and friends should be disabled while this is active
};


};

#endif


