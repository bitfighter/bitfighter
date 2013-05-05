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

#include "UIAbstractInstructions.h"
#include "ScreenInfo.h"
#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"

namespace Zap
{

extern void drawHorizLine(S32 x1, S32 x2, S32 y);


// Constructor
AbstractInstructionsUserInterface::AbstractInstructionsUserInterface(ClientGame *clientGame) : Parent(clientGame)
{
   /* Do nothing */
}


void AbstractInstructionsUserInterface::renderConsoleCommands(const char *activationCommand, const ControlStringsEditor *cmdList)
{
   S32 ypos = 50;

   S32 cmdCol = horizMargin;                                                         // Action column
   S32 descrCol = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 55;   // Control column

   const S32 instrSize = 18;

   glColor(Colors::green);
   drawStringf(cmdCol, ypos, instrSize, activationCommand);
   ypos += 28;

   Color cmdColor =   Colors::cyan;
   Color descrColor = Colors::white;
   Color secColor =   Colors::yellow;

   const S32 headerSize = 20;
   const S32 cmdSize = 16;
   const S32 cmdGap = 10;

   glColor(secColor);
   drawString(cmdCol, ypos, headerSize, "Code Example");
   drawString(descrCol, ypos, headerSize, "Description");

   ypos += cmdSize + cmdGap;
   drawHorizLine(cmdCol, 750, ypos);

   ypos += 5;     // Small gap before cmds start

   for(S32 i = 0; cmdList[i].command != ""; i++)
   {
      if(cmdList[i].command[0] == '-')      // Horiz spacer
      {
         glColor(Colors::gray40);
         drawHorizLine(cmdCol, cmdCol + 335, ypos + (cmdSize + cmdGap) / 4);
      }
      else
      {
         glColor(cmdColor);
         drawString(cmdCol, ypos, cmdSize, cmdList[i].command.c_str());      // Textual description of function (1st arg in lists above)

         glColor(descrColor);
         drawString(descrCol, ypos, cmdSize, cmdList[i].descr.c_str());
      }
      ypos += cmdSize + cmdGap;
   }
}

}
