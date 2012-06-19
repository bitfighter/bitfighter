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

#include "quickChatHelper.h"
#include "UIGame.h"
#include "gameType.h"
#include "gameObjectRender.h"
#include "input.h"
#include "UIMenus.h"
#include "config.h"
#include "Colors.h"
#include "ClientGame.h"
#include "JoystickRender.h"
#include "ClientGame.h"

#include "OpenglUtils.h"

#include <ctype.h>

namespace Zap
{

Vector<QuickChatNode> gQuickChatTree;      // Holds our tree of QuickChat groups and messages, as defined in the INI file

QuickChatHelper::QuickChatHelper(ClientGame *clientGame) : Parent(clientGame)
{
   mCurNode = 0;
}

extern Color gGlobalChatColor;
extern Color gTeamChatColor;
extern Color gErrorMessageTextColor;


// Returns true if there was something to render, false if our current chat tree position has nothing to render.  This can happen
// when a chat tree has a bunch of keyboard only items and we're in joystick mode... if no items are drawn, there's no point
// in remaining in QuickChat mode, is there?
void QuickChatHelper::render()
{
   S32 yPos = MENU_TOP;
   const S32 fontSize = 15;

   GameSettings *settings = getGame()->getSettings();

   if(!gQuickChatTree.size())
   {
      glColor(gErrorMessageTextColor);
      UserInterface::drawCenteredString(yPos, fontSize, "Quick Chat messages improperly configured.  Please see bitfighter.ini.");
      return;
   }

   Vector<QuickChatNode> renderNodes;
   InputMode inputMode = settings->getInputCodeManager()->getInputMode();
   bool showKeyboardKeys = settings->getIniSettings()->showKeyboardKeys;

   S32 walk = mCurNode;
   U32 matchLevel = gQuickChatTree[walk].depth + 1;
   walk++;

   // First get to the end...
   while(gQuickChatTree[walk].depth >= matchLevel)
      walk++;

   // Then draw bottom up...
   while(walk != mCurNode)
   {     // When we're using a controller, don't present options with no defined controller key
      if(gQuickChatTree[walk].depth == matchLevel && ( (inputMode == InputModeKeyboard) || showKeyboardKeys || 
                                                       (gQuickChatTree[walk].buttonCode != KEY_UNKNOWN) ))
         renderNodes.push_back(gQuickChatTree[walk]);
      walk--;
   }

   const S32 indent = 20;
   const Color quickChatMenuHeaderColor(Colors::red);

   drawMenuBorderLine(yPos, quickChatMenuHeaderColor);

   glColor(quickChatMenuHeaderColor);
   UserInterface::drawString(UserInterface::horizMargin, yPos, fontSize, "QuickChat menu");
   yPos += fontSize + 10;

   if(!renderNodes.size())    // Nothing to render, let's go home
   {
      glColor(Colors::red); 
      UserInterface::drawString(UserInterface::horizMargin, yPos, fontSize, "No messages here (misconfiguration?)");
      yPos += fontSize + 7;
   }
   else
   {
      bool showKeys = showKeyboardKeys || (inputMode == InputModeKeyboard);

      S32 xPosBase = UserInterface::horizMargin + (showKeys ? 0 : indent);
      S32 messageIndent = (matchLevel == 1) ? indent : 0;    // No indenting on submenus

      for(S32 i = 0; i < renderNodes.size(); i++)
      {
         S32 xPos = xPosBase + (renderNodes[i].isMsgItem ? messageIndent : 0);

         // Draw key controls for selecting quick chat items
         if(inputMode == InputModeJoystick && renderNodes[i].buttonCode != KEY_UNKNOWN)     // Only draw joystick buttons when in joystick mode
            JoystickRender::renderControllerButton((F32)xPos, (F32)yPos, Joystick::SelectedPresetIndex, renderNodes[i].buttonCode, false);

         Color color = renderNodes[i].teamOnly ? gTeamChatColor : gGlobalChatColor;
         if(showKeys)
         {
            glColor(color);
            JoystickRender::renderControllerButton(F32(xPos + indent + 10), (F32)yPos, Joystick::SelectedPresetIndex, renderNodes[i].inputCode, false);
         }
 
         glColor(color);
         UserInterface::drawString(UserInterface::horizMargin + 50 + (renderNodes[i].isMsgItem ? messageIndent : 0), yPos, fontSize, renderNodes[i].caption.c_str());
         yPos += fontSize + 7;
      }
   }

   const S32 fontSizeSm = fontSize - 4;

   glColor(gTeamChatColor);
   UserInterface::drawString(UserInterface::horizMargin + indent, yPos, fontSizeSm, "Team Message");
   glColor(gGlobalChatColor);
   UserInterface::drawString(UserInterface::horizMargin + indent + S32(UserInterface::getStringWidth(fontSizeSm, "Team Message ")), yPos, fontSizeSm, "Global Message");

   yPos += 12;

   // Add some help text
   drawMenuBorderLine(yPos - fontSize - 2, quickChatMenuHeaderColor);
   yPos += 8;
   drawMenuCancelText(yPos, quickChatMenuHeaderColor, fontSize);

   return;
}


void QuickChatHelper::onMenuShow()
{
   mCurNode = 0;
}


// Returns true if key was used, false if not
bool QuickChatHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))
      return true;

   if(!gQuickChatTree.size())       // We'll crash if we go any further!
      return false;

   // Try to find a match if we can...

   // Set up walk...
   S32 walk = mCurNode;
   U32 matchLevel = gQuickChatTree[walk].depth + 1;
   walk++;

   // Iterate over anything at our desired depth or lower
   while(gQuickChatTree[walk].depth >= matchLevel)
   {
      // If it has the same key...
      bool match = (inputCode == gQuickChatTree[walk].inputCode) || (inputCode == gQuickChatTree[walk].buttonCode);

      if(match && gQuickChatTree[walk].depth == matchLevel)
      {
         // ...then select it
         mCurNode = walk;

         UserInterface::playBoop();

         // If we're at a leaf (ie, next child down is higher or equal to us), then issue the chat and call it good
         walk++;
         if(gQuickChatTree[mCurNode].depth >= gQuickChatTree[walk].depth)
         {
            exitHelper();

            if(gQuickChatTree[mCurNode].commandOnly)
               getGame()->runCommand(gQuickChatTree[mCurNode].msg.c_str());

            else
            {
               GameType *gt = getGame()->getGameType();

               if(gt)
               {
                  StringTableEntry entry(gQuickChatTree[mCurNode].msg.c_str());
                  gt->c2sSendChatSTE(!gQuickChatTree[mCurNode].teamOnly, entry);
               }
            }
         }
         return true;
      }
      walk++;
   }

   return false;
}


bool QuickChatHelper::isMovementDisabled()
{
   return false;
}


};

