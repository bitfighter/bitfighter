//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIEDITORINSTRUCTIONS_H_
#define _UIEDITORINSTRUCTIONS_H_

#include "UIAbstractInstructions.h"
#include "WallSegmentManager.h"
#include "Timer.h"

namespace Zap
{


class EditorInstructionsUserInterface : public AbstractInstructionsUserInterface
{
   typedef AbstractInstructionsUserInterface Parent;

private:
   S32 mCol1;
   S32 mCol2;
   S32 mCol3;
   S32 mCol4;

   S32 mCurPage;
   Timer mAnimTimer;
   S32 mAnimStage;
   WallSegmentManager mWallSegmentManager;

   UI::SymbolStringSetCollection mSymbolSets1Left,     mSymbolSets1Right;     // For page 1
   UI::SymbolStringSetCollection mSymbolSets2Left,     mSymbolSets2Right;     // For page 2
   UI::SymbolStringSet           mConsoleInstructions, mPluginInstructions;

   void onPageChanged();

public:
   explicit EditorInstructionsUserInterface(ClientGame *game);      // Constructor
   virtual ~EditorInstructionsUserInterface();

   void render();
   void renderPageCommands(S32 page);
   void renderPageWalls();

   S32 getPageCount();
 
   bool onKeyDown(InputCode inputCode);

   void nextPage();
   void prevPage();

   void onActivate();
   void exitInstructions();
   void idle(U32 timeDelta);
};

};

#endif


