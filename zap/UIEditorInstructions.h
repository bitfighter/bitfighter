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

#ifndef _UIEDITORINSTRUCTIONS_H_
#define _UIEDITORINSTRUCTIONS_H_

#include "UI.h"
#include "timer.h"

namespace Zap
{

class EditorInstructionsUserInterface : public UserInterface
{
private:
   U32 mCurPage;
   Timer mAnimTimer;
   S32 mAnimStage;
public:
   EditorInstructionsUserInterface();      // Constructor
   void render();
   void renderPage1();
   void renderPage2();

   void onKeyDown(KeyCode keyCode, char ascii);

   void nextPage();
   void prevPage();

   void onActivate();
   void exitInstructions();
   void idle(U32 timeDelta);
};

extern EditorInstructionsUserInterface gEditorInstructionsUserInterface;

};

#endif


