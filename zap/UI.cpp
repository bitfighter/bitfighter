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


#include "UI.h"

#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIMenus.h"
#include "UIManager.h"

#include "ClientGame.h"
#include "Console.h"             // For console rendering
#include "Colors.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "masterConnection.h"    // For MasterServerConnection def
#include "VideoSystem.h"
#include "SoundSystem.h"
#include "OpenglUtils.h"
#include "LoadoutIndicator.h"    // For LoadoutIndicatorHeight

#include "MathUtils.h"           // For RADIANS_TO_DEGREES def
#include "RenderUtils.h"

#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

#ifdef TNL_OS_XBOX
   S32 UserInterface::horizMargin = 50;
   S32 UserInterface::vertMargin = 38;
#else
   S32 UserInterface::horizMargin = 15;
   S32 UserInterface::vertMargin = 15;
#endif

S32 UserInterface::messageMargin = UserInterface::vertMargin + UI::LoadoutIndicator::LoadoutIndicatorHeight + 5;

extern Vector<ClientGame *> gClientGames;


////////////////////////////////////////
////////////////////////////////////////

// Constructor
UserInterface::UserInterface(ClientGame *clientGame)
{
   mClientGame = clientGame;
   mTimeSinceLastInput = 0;
   mDisableShipKeyboardInput = true;
}


UserInterface::~UserInterface()
{
   // Do nothing
}


ClientGame *UserInterface::getGame() const
{
   return mClientGame;
}


UIManager *UserInterface::getUIManager() const 
{ 
   TNLAssert(mClientGame, "mGame is NULL!");
   return mClientGame->getUIManager(); 
}


bool UserInterface::usesEditorScreenMode()
{
   return false;
}


void UserInterface::activate()
{
   onActivate(); 
}


void UserInterface::reactivate()
{
   onReactivate();
}


void UserInterface::onActivate()          { /* Do nothing */ }
void UserInterface::onReactivate()        { /* Do nothing */ }
void UserInterface::onDisplayModeChange() { /* Do nothing */ }


void UserInterface::onDeactivate(bool nextUIUsesEditorScreenMode)
{
   if(nextUIUsesEditorScreenMode != usesEditorScreenMode())
      VideoSystem::actualizeScreenMode(getGame()->getSettings(), true, nextUIUsesEditorScreenMode);
}


U32 UserInterface::getTimeSinceLastInput()
{
   return mTimeSinceLastInput;
}


void UserInterface::playBoop()
{
   SoundSystem::playSoundEffect(SFXUIBoop, 1);
}


// Render master connection state if we're not connected
void UserInterface::renderMasterStatus()
{
   MasterServerConnection *conn = mClientGame->getConnectionToMaster();

   if(conn && conn->getConnectionState() != NetConnection::Connected)
   {
      glColor(Colors::white);
      drawStringf(10, 550, 15, "Master Server - %s", GameConnection::getConnectionStateString(conn->getConnectionState()));
   }
}


void UserInterface::renderConsole() const
{
#ifndef BF_NO_CONSOLE
   // Temporarily disable scissors mode so we can use the full width of the screen
   // to show our console text, black bars be damned!
   bool scissorMode = glIsEnabled(GL_SCISSOR_TEST);

   if(scissorMode) 
      glDisable(GL_SCISSOR_TEST);

   gConsole.render();

   if(scissorMode) 
      glEnable(GL_SCISSOR_TEST);
#endif
}


extern ScreenInfo gScreenInfo;

void UserInterface::renderMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset, S32 style) const
{
   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 inset = 100;                    // Inset for left and right edges of box
   const S32 titleSize = 30;           // Size of title
   const S32 titleGap = titleSize / 3; // Spacing between title and first line of text

   S32 textSize;

   if(style == 1)
      textSize = 18;            // Size of text and instructions
   else if(style == 2)
      textSize = 30;

   const S32 textGap = textSize / 3;   // Spacing between text lines
   const S32 instrGap = 15;            // Gap between last line of text and instruction line

   S32 titleSpace = titleSize + titleGap;
   S32 boxHeight = titleSpace + 2 * vertMargin + (msgLines + 1) * (textSize + textGap) + instrGap;

   if(strcmp(instr, "") == 0)
      boxHeight -= (instrGap + textSize);

   if(strcmp(title, "") == 0)
   {
      boxHeight -= titleSpace;
      titleSpace = 0;
   }

   S32 boxTop = (canvasHeight - boxHeight) / 2 + vertOffset;

   S32 maxLen = 0;
   for(S32 i = 0; i < msgLines; i++)
   {
      S32 len = getStringWidth(textSize, message[i].c_str()) + 20;     // 20 gives a little breathing room on the edges
      if(len > maxLen)
         maxLen = len;
   }

   if(canvasWidth - 2 * inset < maxLen)
      inset = (canvasWidth - maxLen) / 2;


   if(style == 1)       
   {
      // Solid red box with white border
      glDisable(GL_BLEND);
      drawFilledRect(inset, boxTop, canvasWidth - inset, boxTop + boxHeight, Colors::red30, Colors::white);
      glEnable(GL_BLEND);
      glColor(Colors::white);
   }
   else if(style == 2)
   {
      renderCenteredFancyBox(boxTop, boxHeight, inset, 15, Colors::black, 0.70f, Colors::blue);
   }

   // Draw title, message, and footer
   drawCenteredString(boxTop + vertMargin, titleSize, title);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + vertMargin + titleSpace + i * (textSize + textGap), textSize, message[i].c_str());

   drawCenteredString(boxTop + boxHeight - vertMargin - textSize, textSize, instr);
}


// This renders a semi-transparent box with two corners angled.  A fancy UI element.
void UserInterface::renderFancyBox(S32 xLeft, S32 yTop, S32 xRight, S32 yBottom, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   Point p[] = {
         Point(xLeft, yTop),                                // Top
         Point(xRight - cornerInset, yTop),
         Point(xRight, yTop + cornerInset),    // Edge
         Point(xRight, yBottom),      // Bottom
         Point(xLeft + cornerInset, yBottom),
         Point(xLeft, yBottom - cornerInset)     // Edge
   };

   Vector<Point> points(p, ARRAYSIZE(p));

   // Fill
   glColor(fillColor, fillAlpha);
   renderPointVector(&points, GL_POLYGON);

   // Border
   glColor(borderColor, fillAlpha);
   renderPointVector(&points, GL_LINE_LOOP);
}


void UserInterface::renderCenteredFancyBox(S32 boxTop, S32 boxHeight, S32 inset, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor)
{
   renderFancyBox(inset, boxTop, gScreenInfo.getGameCanvasWidth() - inset, boxTop + boxHeight, cornerInset, fillColor, fillAlpha, borderColor);
}


// This function could use some further cleaning; currently only used for the delayed spawn notification
void UserInterface::renderUnboxedMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset) const
{
   dimUnderlyingUI();

   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   static const S32 titleSize = 30;              // Size of title
   static const S32 titleGap = titleSize / 3;    // Spacing between title and first line of text
   static const S32 textSize = 36;               // Size of text and instructions
   static const S32 textGap = textSize / 3;      // Spacing between text lines
   static const S32 instrGap = 15;               // Gap between last line of text and instruction line

   S32 actualLines = 0;
   for(S32 i = msgLines - 1; i >= 0; i--)
      if(message[i] != "")
      {
         actualLines = i + 1;
         break;
      }


   S32 titleSpace = titleSize + titleGap;
   S32 boxHeight = titleSpace + actualLines * (textSize + textGap) + instrGap;

   if(strcmp(instr, "") == 0)
      boxHeight -= instrGap;

   if(strcmp(title, "") == 0)
   {
      boxHeight -= titleSpace;
      titleSpace = 0;
   }

   S32 boxTop = (canvasHeight - boxHeight) / 2;

   // Draw title, message, and footer
   glColor(Colors::blue);
   drawCenteredString(boxTop + vertMargin, titleSize, title);

   S32 boxWidth = 500;
   drawHollowRect((canvasWidth - boxWidth) / 2, boxTop - vertMargin, canvasWidth - ((canvasWidth - boxWidth) / 2), boxTop + boxHeight + vertMargin);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + titleSpace + i * (textSize + textGap), textSize, message[i].c_str());

   drawCenteredString(boxTop + boxHeight / 2 - textSize, textSize, instr);
}


// Static method
void UserInterface::dimUnderlyingUI(F32 amount)
{
   glColor(Colors::black, amount); 

   TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

   drawFilledRect (0, 0, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());
}


void UserInterface::drawMenuItemHighlight(S32 x1, S32 y1, S32 x2, S32 y2, bool disabled)
{
   if(disabled)
      drawFilledRect(x1, y1, x2, y2, Color(0.4), Color(0.8));
   else
      drawFilledRect(x1, y1, x2, y2, Colors::blue40, Colors::blue);
}


// These will be overridden in child classes if needed
void UserInterface::render() 
{ 
   // Do nothing -- probably never even gets called
}


void UserInterface::idle(U32 timeDelta)
{ 
   mTimeSinceLastInput += timeDelta;
}


void UserInterface::onMouseMoved()                         
{ 
   mTimeSinceLastInput = 0;
}


void UserInterface::onMouseDragged()  { /* Do nothing */ }


InputCode UserInterface::getInputCode(GameSettings *settings, InputCodeManager::BindingName binding)
{
   return settings->getInputCodeManager()->getBinding(binding);
}


void UserInterface::setInputCode(GameSettings *settings, InputCodeManager::BindingName binding, InputCode inputCode)
{
   settings->getInputCodeManager()->setBinding(binding, inputCode);
}


bool UserInterface::checkInputCode(GameSettings *settings, InputCodeManager::BindingName binding, InputCode inputCode)
{
   return getInputCode(settings, binding) == settings->getInputCodeManager()->filterInputCode(inputCode);
}


const char *UserInterface::getInputCodeString(GameSettings *settings, InputCodeManager::BindingName binding)
{
   return InputCodeManager::inputCodeToString(getInputCode(settings, binding));
}


class ChatUserInterface;
class NameEntryUserInterface;
class DiagnosticUserInterface;
 
bool UserInterface::onKeyDown(InputCode inputCode)
{ 
   mTimeSinceLastInput = 0;

   bool handled = false;

   GameSettings *settings = getGame()->getSettings();
   UIManager *uiManager = getGame()->getUIManager();

   if(checkInputCode(settings, InputCodeManager::BINDING_DIAG, inputCode))              // Turn on diagnostic overlay
   { 
      if(uiManager->isCurrentUI<DiagnosticUserInterface>())
         return false;

      uiManager->activate<DiagnosticUserInterface>();

      playBoop();
      
      handled = true;
   }
   else if(checkInputCode(settings, InputCodeManager::BINDING_OUTGAMECHAT, inputCode))  // Turn on Global Chat overlay
   {
      // Don't activate if we're already in chat or if we're on the Name Entry
      // screen (since we don't have a nick yet)
      if(uiManager->isCurrentUI<ChatUserInterface>() || uiManager->isCurrentUI<NameEntryUserInterface>())
         return false;

      getGame()->getUIManager()->activate<ChatUserInterface>();
      playBoop();

      handled = true;
   }

   return handled;
}


void UserInterface::onKeyUp(InputCode inputCode) { /* Do nothing */ }
void UserInterface::onTextInput(char ascii)      { /* Do nothing */ }



// Dumps any keys and raw stick button inputs depressed to the screen when in diagnostic mode.
// This should make it easier to see what happens when users press joystick buttons.
void UserInterface::renderDiagnosticKeysOverlay()
{
   if(gClientGames[0]->getSettings()->getIniSettings()->diagnosticKeyDumpMode)
   {
     S32 vpos = gScreenInfo.getGameCanvasHeight() / 2;
     S32 hpos = horizMargin;

     glColor(Colors::white);

     // Key states
     for (U32 i = 0; i < MAX_INPUT_CODES; i++)
        if(InputCodeManager::getState((InputCode) i))
           hpos += drawStringAndGetWidth( hpos, vpos, 18, InputCodeManager::inputCodeToString((InputCode) i) );

      vpos += 23;
      hpos = horizMargin;
      glColor(Colors::magenta);

      for(U32 i = 0; i < Joystick::MaxSdlButtons; i++)
         if(Joystick::ButtonMask & (1 << i))
         {
            drawStringf( hpos, vpos, 18, "RawBut [%d]", i );
            hpos += getStringWidthf(18, "RawBut [%d]", i ) + 5;
         }
   }
}   


////////////////////////////////////////
////////////////////////////////////////


void UserInterfaceData::get()
{
   vertMargin  = UserInterface::vertMargin;
   horizMargin = UserInterface::horizMargin;
   chatMargin  = UserInterface::messageMargin;
}


void UserInterfaceData::set()
{
   UserInterface::vertMargin  = vertMargin;
   UserInterface::horizMargin = horizMargin;
   UserInterface::messageMargin = chatMargin;
}


};

