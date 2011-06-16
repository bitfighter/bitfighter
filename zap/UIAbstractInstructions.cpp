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
#include "Color.h"
#include "Colors.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{


void AbstractInstructionsUserInterface::renderConsoleCommands(const char *activationCommand, ControlStringsEditor *cmdList)
{
   S32 ypos = 50;

   S32 cmdCol = horizMargin;                                                         // Action column
   S32 descrCol = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 55;   // Control column

   const S32 instrSize = 18;

   glColor3f(0,1,0);
   drawStringf(cmdCol, ypos, instrSize, activationCommand);
   ypos += 28;

   Color cmdColor =   Colors::cyan;
   Color descrColor = Colors::white;
   Color secColor =   Colors::yellow;

   const S32 headerSize = 20;
   const S32 cmdSize = 16;
   const S32 cmdGap = 10;

   glColor(secColor);
   drawString(cmdCol, ypos, headerSize, "Command");
   drawString(descrCol, ypos, headerSize, "Description");

   //glColor3f(0,1,0);
   ypos += cmdSize + cmdGap;
   glBegin(GL_LINES);
      glVertex(cmdCol, ypos);
      glVertex(750, ypos);
   glEnd();

   ypos += 5;     // Small gap before cmds start

   for(S32 i = 0; cmdList[i].command; i++)
   {
      if(cmdList[i].command[0] == '-')      // Horiz spacer
      {
         glColor(0.4, 0.4, 0.4);
         glBegin(GL_LINES);
            glVertex(cmdCol, ypos + (cmdSize + cmdGap) / 4);
            glVertex(cmdCol + 335, ypos + (cmdSize + cmdGap) / 4);
         glEnd();
      }
      else
      {
         glColor(cmdColor);
         drawString(cmdCol, ypos, cmdSize, cmdList[i].command);      // Textual description of function (1st arg in lists above)
         glColor(descrColor);
         drawString(descrCol, ypos, cmdSize, cmdList[i].descr);
      }
      ypos += cmdSize + cmdGap;
   }
}

}
