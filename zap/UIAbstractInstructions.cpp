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
#include "FontManager.h"

namespace Zap
{


// Define static consts
const Color &AbstractInstructionsUserInterface::txtColor         = Colors::cyan;
const Color &AbstractInstructionsUserInterface::keyColor         = Colors::white;     
const Color &AbstractInstructionsUserInterface::secColor         = Colors::yellow;
const Color &AbstractInstructionsUserInterface::groupHeaderColor = Colors::red;

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
                                             const ControlStringsEditor *helpBindings, S32 bindingCount) const
{
   Vector<SymbolShapePtr> symbols;

   for(S32 i = 0; i < bindingCount; i++)
   {
      if(helpBindings[i].command == "-")
      {
         symbols.clear();
         symbols.push_back(SymbolString::getHorizLine(335, FontSize, Colors::gray40));
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
                                   symbols, HelpContext, FontSize, true, keyColor, keyColor);
         bindings.add(SymbolString(symbols));
      }
   }
}


void AbstractInstructionsUserInterface::render(const char *header, S32 page, S32 pages) const
{
   static const char* prefix = "INSTRUCTIONS - ";
   static const S32 fontSize = 25;
   static const S32 prefixWidth = RenderUtils::getStringWidth(fontSize, prefix);
   static const S32 y = 28;

   // Draw header first as different color, then everything else
   RenderUtils::drawString_fixed(3 + prefixWidth, y, fontSize, Colors::cyan, header);
   RenderUtils::drawString_fixed(3,               y, fontSize, Colors::red,  prefix);

   RenderUtils::drawStringf_fixed(625, y, fontSize, Colors::red, "PAGE %d/%d", page, pages);
   RenderUtils::drawCenteredString_fixed(591, 20, Colors::red, "LEFT - previous page   |   RIGHT, SPACE - next page   |   ESC exits");

   RenderUtils::drawHorizLine(32,  Colors::gray70);
   RenderUtils::drawHorizLine(569, Colors::gray70);
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

   FontManager::setFontColor(secColor);
   RenderUtils::drawString(cmdCol,   ypos, headerSize, "Code Example");
   RenderUtils::drawString(descrCol, ypos, headerSize, "Description");

   Vector<SymbolShapePtr> symbols;

   ypos += cmdSize + cmdGap;
   RenderUtils::drawHorizLine(cmdCol, 750, ypos, Colors::gray70);

   ypos += 10;     // Small gap before cmds start
   ypos += cmdSize;

   for(S32 i = 0; cmdList[i].command != ""; i++)
   {
      if(cmdList[i].command[0] == '-')      // Horiz spacer
         RenderUtils::drawHorizLine(cmdCol, cmdCol + 335, ypos + (cmdSize + cmdGap) / 4, Colors::gray40);
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
