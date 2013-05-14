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

#ifndef _UIINSTRUCTIONS_H_
#define _UIINSTRUCTIONS_H_

#include "UIAbstractInstructions.h"
#include "SymbolShape.h"
#include "Point.h"

namespace Zap
{

class InstructionsUserInterface : public AbstractInstructionsUserInterface
{
   typedef AbstractInstructionsUserInterface Parent;

private:
   S32 mCurPage;
   bool mUsingArrowKeys;

   S32 col1;
   S32 col2;
   S32 col3;
   S32 col4;

   bool usingArrowKeys();

   void renderPage1();
   void renderPage2();
   void renderModulesPage();
   void renderPageObjectDesc(U32 index);
   void renderPageGameIndicators();
   void renderPageCommands(U32 index, const char *msg = "");
   void nextPage();
   void prevPage();

   void renderKeyBindingQuad(S32 y, const char *str1, InputCodeManager::BindingName binding1, 
                                    const char *str2, InputCodeManager::BindingName binding2);

   Vector<Point> mResourceItemPoints, mTestItemPoints;

   UI::SymbolStringSet mControls;

public:

   // This must be kept aligned with pageHeaders[]
   enum IntructionPages {
      InstructionControls,
      InstructionLoadout,
      InstructionModules,
      InstructionWeaponProjectiles,
      InstructionSpyBugs,
      InstructionGameObjects1,
      InstructionGameObjects2,
      InstructionGameObjects3,
      InstructionGameIndicators,
      InstructionAdvancedCommands,
      InstructionSoundCommands,
      InstructionLevelCommands,
      InstructionAdminCommands,
      InstructionOwnerCommands,
      InstructionDebugCommands,
      //InstructionScriptingConsole,
      InstructionMaxPages
   };

   explicit InstructionsUserInterface(ClientGame *game);      // Constructor
   virtual ~InstructionsUserInterface();

   void render();

   bool onKeyDown(InputCode inputCode);

   void activatePage(IntructionPages pageIndex);
   void onActivate();
   void exitInstructions();
};

};

#endif


