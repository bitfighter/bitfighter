//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIAbstractInstructions.h"

#include "ClientGame.h"
#include "GameSettings.h"
#include "DisplayManager.h"
#include "Colors.h"

#include "RenderUtils.h"

namespace Zap
{


// Define static consts
const Color *AbstractInstructionsUserInterface::txtColor = &Colors::cyan;
const Color *AbstractInstructionsUserInterface::keyColor = &Colors::white;     
const Color *AbstractInstructionsUserInterface::secColor = &Colors::yellow;
const Color *AbstractInstructionsUserInterface::groupHeaderColor = &Colors::red;

// Import some symbols to reduce typing
using UI::SymbolString;
using UI::SymbolShapePtr;
using UI::SymbolStringSet;


// Constructor
AbstractInstructionsUserInterface::AbstractInstructionsUserInterface(ClientGame *clientGame, UIManager *uiManager) :
                                       Parent(clientGame, uiManager),
                                       mSpecialKeysInstrLeft(LineGap), 
                                       mSpecialKeysBindingsLeft(LineGap), 
                                       mSpecialKeysInstrRight(LineGap), 
                                       mSpecialKeysBindingsRight(LineGap),
                                       mWallInstr(LineGap),  
                                       mWallBindings(LineGap)
{
   // Do nothing
}


// Destructor
AbstractInstructionsUserInterface::~AbstractInstructionsUserInterface()
{
   // Do nothing
}


void AbstractInstructionsUserInterface::pack(SymbolStringSet &instrs,  SymbolStringSet &bindings,      // <== will be modified
                                             const ControlStringsEditor *helpBindings, S32 bindingCount)
{
   Vector<SymbolShapePtr> symbols;

   for(S32 i = 0; i < bindingCount; i++)
   {
      if(helpBindings[i].command == "-")
      {
         symbols.clear();
         symbols.push_back(SymbolString::getHorizLine(335, FontSize, &Colors::gray40));
         instrs.add(SymbolString(symbols));

         symbols.clear();
         symbols.push_back(SymbolString::getBlankSymbol(0, FontSize));
         bindings.add(SymbolString(symbols));
      }
      else if(helpBindings[i].command == "HEADER")
      {
         symbols.clear();
         symbols.push_back(SymbolString::getSymbolText(helpBindings[i].binding, FontSize, HelpContext, groupHeaderColor));
         instrs.add(SymbolString(symbols));

         symbols.clear();
         symbols.push_back(SymbolString::getBlankSymbol(0, FontSize));
         bindings.add(SymbolString(symbols));
      }
      else     // Normal line
      {
         symbols.clear();
         SymbolString::symbolParse(mGameSettings->getInputCodeManager(), helpBindings[i].command,
                                   symbols, HelpContext, FontSize, true, txtColor, keyColor);

         instrs.add(SymbolString(symbols));

         symbols.clear();
         SymbolString::symbolParse(mGameSettings->getInputCodeManager(), helpBindings[i].binding,
                                   symbols, HelpContext, FontSize, true, keyColor);
         bindings.add(SymbolString(symbols));
      }
   }
}


void AbstractInstructionsUserInterface::render(const char *header, S32 page, S32 pages) const
{
   static const char* prefix = "INSTRUCTIONS - ";
   static S32 fontSize = 25;
   static S32 prefixWidth = RenderUtils::getStringWidth(fontSize, prefix);

   // Draw header first as different color, then everything else
   mGL->glColor(Colors::cyan);
   RenderUtils::drawString(3 + prefixWidth, 3, fontSize, header);

   mGL->glColor(Colors::red);
   RenderUtils::drawString(3, 3, fontSize, prefix);

   RenderUtils::drawStringf(625, 3, fontSize, "PAGE %d/%d", page, pages);
   RenderUtils::drawCenteredString(571, 20, "LEFT - previous page   |   RIGHT, SPACE - next page   |   ESC exits");

   mGL->glColor(Colors::gray70);
   RenderUtils::drawHorizLine(0, 800, 32);
   RenderUtils::drawHorizLine(0, 800, 569);
}


void AbstractInstructionsUserInterface::renderConsoleCommands(const SymbolStringSet &instructions, 
                                                              const ControlStringsEditor *cmdList) const
{
   const S32 headerSize = 20;
   const S32 cmdSize = 16;
   const S32 cmdGap = 10;

   S32 ypos = 60;
   S32 cmdCol = horizMargin;                                                                             // Action column
   S32 descrCol = horizMargin + S32(DisplayManager::getScreenInfo()->getGameCanvasWidth() * 0.25) + 55;  // Control column

   ypos += instructions.render(cmdCol, ypos, UI::AlignmentLeft);

   ypos += 10 - cmdSize - cmdGap;

   Color secColor =   Colors::yellow;

   mGL->glColor(secColor);
   RenderUtils::drawString(cmdCol,   ypos, headerSize, "Code Example");
   RenderUtils::drawString(descrCol, ypos, headerSize, "Description");

   Vector<SymbolShapePtr> symbols;

   ypos += cmdSize + cmdGap;
   mGL->glColor(&Colors::gray70);
   RenderUtils::drawHorizLine(cmdCol, 750, ypos);

   ypos += 10;     // Small gap before cmds start
   ypos += cmdSize;

   for(S32 i = 0; cmdList[i].command != ""; i++)
   {
      if(cmdList[i].command[0] == '-')      // Horiz spacer
      {
         mGL->glColor(Colors::gray40);
         RenderUtils::drawHorizLine(cmdCol, cmdCol + 335, ypos + (cmdSize + cmdGap) / 4);
      }
      else
      {
         symbols.clear();
         SymbolString::symbolParse(mGameSettings->getInputCodeManager(), cmdList[i].command,
                                   symbols, HelpContext, cmdSize, true, txtColor, keyColor);

         SymbolString instrs(symbols);
         instrs.render(cmdCol, ypos, UI::AlignmentLeft);

         symbols.clear();
         SymbolString::symbolParse(mGameSettings->getInputCodeManager(), cmdList[i].binding,
                                   symbols, HelpContext, cmdSize, true, txtColor, keyColor);

         SymbolString keys(symbols);
         keys.render(descrCol, ypos, UI::AlignmentLeft);
      }

      ypos += cmdSize + cmdGap;
   }
}


}
