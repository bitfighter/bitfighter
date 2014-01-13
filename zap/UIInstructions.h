//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIINSTRUCTIONS_H_
#define _UIINSTRUCTIONS_H_

#include "UIAbstractInstructions.h"
#include "Point.h"

namespace Zap
{

#define INSTR_TABLE \
   INSTR_ITEM(InstructionControls,           "CONTROLS")            \
   INSTR_ITEM(InstructionLoadout,            "LOADOUT SELECTION")   \
   INSTR_ITEM(InstructionModules,            "MODULES")             \
   INSTR_ITEM(InstructionWeaponProjectiles,  "WEAPON PROJECTILES")  \
   INSTR_ITEM(InstructionSpyBugs,            "MINES & SPY BUGS")    \
   INSTR_ITEM(InstructionGameObjects1,       "GAME OBJECTS")        \
   INSTR_ITEM(InstructionGameObjects2,       "MORE GAME OBJECTS")   \
   INSTR_ITEM(InstructionGameObjects3,       "MORE GAME OBJECTS")   \
   INSTR_ITEM(InstructionsGameTypes,         "GAME TYPES")          \
   INSTR_ITEM(InstructionGameIndicators,     "ACHIEVEMENTS")        \
   INSTR_ITEM(InstructionAdvancedCommands,   "ADVANCED COMMANDS")   \
   INSTR_ITEM(InstructionSoundCommands,      "SOUND AND MUSIC")     \
   INSTR_ITEM(InstructionLevelCommands,      "LEVEL COMMANDS")      \
   INSTR_ITEM(InstructionBotCommands,        "BOT COMMANDS")        \
   INSTR_ITEM(InstructionAdminCommands,      "ADMIN COMMANDS")      \
   INSTR_ITEM(InstructionOwnerCommands,      "OWNER COMMANDS")      \
   INSTR_ITEM(InstructionDebugCommands,      "DEBUG COMMANDS")      \
   BUILD_DEPENDENT_ITEMS         
   /*
   INSTR_ITEM(InstructionScriptingConsole,   "SCRIPTING CONSOLE") \
   */
#ifdef TNL_DEBUG                                                    
#  define BUILD_DEPENDENT_ITEMS\
      INSTR_ITEM(InstructionTestCommands,        "TEST COMMANDS")      
#else
#  define BUILD_DEPENDENT_ITEMS 
#endif 


class InstructionsUserInterface : public AbstractInstructionsUserInterface
{
   typedef AbstractInstructionsUserInterface Parent;

public:
   enum IntructionPages {
#     define INSTR_ITEM(enumValue, b)  enumValue,
         INSTR_TABLE
#     undef INSTR_ITEM
      InstructionMaxPages
   };

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
   void renderPageGameTypes();
   void nextPage();
   void prevPage();

   Vector<Point> mResourceItemPoints, mTestItemPoints;

   UI::SymbolStringSetCollection mSymbolSets;
   UI::SymbolStringSet mLoadoutInstructions, mPageHeaders;
   UI::SymbolStringSet mGameTypeInstrs;

   void initNormalKeys_page1();
   void initPage2();
   void initPageHeaders();
   void initGameTypesPage();


public:
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


