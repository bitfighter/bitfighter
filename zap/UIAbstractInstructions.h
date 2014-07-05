//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#ifndef _UIABSTRACTINSTRUCTIONS_H_
#define _UIABSTRACTINSTRUCTIONS_H_


#include "UI.h"
#include "SymbolShape.h"


namespace Zap {


class AbstractInstructionsUserInterface : public UserInterface
{
   typedef UserInterface Parent;

public:
   struct ControlStringsEditor {
      string command;
      string binding;
   };

   static const S32 HeaderFontSize = 20;
   static const S32 FontSize = 18;
   static const S32 LineGap = 8;

   static const Color *txtColor;
   static const Color *keyColor;
   static const Color *secColor;
   static const Color *groupHeaderColor;

protected:
   void render(const char *header, S32 page, S32 pages) const;
   void renderConsoleCommands(const UI::SymbolStringSet &instructions, const ControlStringsEditor *cmdList) const;

   UI::SymbolStringSet 
         mSpecialKeysInstrLeft,   mSpecialKeysBindingsLeft, 
         mSpecialKeysInstrRight,  mSpecialKeysBindingsRight,
         mWallInstr,              mWallBindings;

   void pack(UI::SymbolStringSet &Instrs, UI::SymbolStringSet &Bindings, 
            const ControlStringsEditor *helpBindings, S32 bindingCount, GameSettings *settings);

public:
   explicit AbstractInstructionsUserInterface(ClientGame *clientGame);  // Constructor
   virtual ~AbstractInstructionsUserInterface();                        // Destructor
};

}

#endif


