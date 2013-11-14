//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


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

#include "FontManager.h"

#include "MathUtils.h"           // For RADIANS_TO_DEGREES def
#include "RenderUtils.h"

#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

using namespace UI;

// Define statics
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


bool UserInterface::usesEditorScreenMode() const
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

static const S32 MessageBoxPadding = 10;  
static const S32 TitleSize = 30;
static const S32 TitleGap = 10;           // Spacing between title and first line of message box
static const S32 TitleHeight = TitleSize + TitleGap;
static const S32 TextSize = 18;
static const S32 TextSizeBig = 30;


void UserInterface::renderMessageBox(const char *title, const char *instr, string message[], S32 msgLines, S32 vertOffset, S32 style) const
{
   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 textSize;

   if(style == 1)
      textSize = TextSize;             // Size of text and instructions
   else if(style == 2)
      textSize = TextSizeBig;

   const S32 textGap = textSize / 3;   // Spacing between text lines
   const S32 instrGap = 15;            // Gap between last line of text and instruction line

   S32 titleHeight = strcmp(title, "") != 0 ? TitleHeight : 0;

   S32 boxHeight  = titleHeight + 2 * vertMargin + (msgLines + 1) * (textSize + textGap) + instrGap;

   if(strcmp(instr, "") == 0)
      boxHeight -= (instrGap + textSize);

   S32 boxTop = (canvasHeight - boxHeight) / 2 + vertOffset;

   S32 maxLen = 0;
   for(S32 i = 0; i < msgLines; i++)
   {
      S32 len = getStringWidth(textSize, message[i].c_str()) + MessageBoxPadding * 2; 
      if(len > maxLen)
         maxLen = len;
   }

   S32 boxwidth = max(UIManager::MessageBoxWrapWidth, maxLen);
   S32 inset = (canvasWidth - boxwidth) / 2;                            // Inset for left and right edges of box

   if(style == 1)       
      renderCenteredFancyBox(boxTop, boxHeight, inset, 15, Colors::red30, 1.0f, Colors::white);
   else if(style == 2)
      renderCenteredFancyBox(boxTop, boxHeight, inset, 15, Colors::black, 0.70f, Colors::blue);

   // Draw title, message, and footer
   drawCenteredString(boxTop + vertMargin, TitleSize, title);

   for(S32 i = 0; i < msgLines; i++)
      drawCenteredString(boxTop + vertMargin + titleHeight + i * (textSize + textGap), textSize, message[i].c_str());

   drawCenteredString(boxTop + boxHeight - vertMargin - textSize, textSize, instr);
}


void UserInterface::renderCenteredFancyBox(S32 boxTop, S32 boxHeight, S32 inset, S32 cornerInset, const Color &fillColor, 
                                           F32 fillAlpha, const Color &borderColor)
{
   drawFilledFancyBox(inset, boxTop, gScreenInfo.getGameCanvasWidth() - inset, boxTop + boxHeight, cornerInset, fillColor, fillAlpha, borderColor);
}


void UserInterface::renderMessageBox(const SymbolShapePtr &title, const SymbolShapePtr &instr, 
                                           SymbolShapePtr *message, S32 msgLines, S32 vertOffset, S32 style) const
{
   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const S32 titleTextGap = 30;        // Space between title and rest of message

   S32 textSize;

   if(style == 1)
      textSize = TextSize;             // Size of text and instructions
   else if(style == 2)
      textSize = TextSizeBig;

   const S32 textGap = textSize / 3;   // Spacing between text lines
   const S32 instrGap = 15;            // Gap between last line of text and instruction line
   const S32 instrGapBottom = 5;       // A bit of extra gap below the instr. line

   S32 titleHeight = title->getHeight();
   if(titleHeight > 0)
      titleHeight += TitleGap;

   S32 instrHeight = instr == NULL ? 0 : instr->getHeight();
   if(instrHeight > 0)
      instrHeight += instrGap + instrGapBottom;

   S32 boxHeight = titleHeight + 2 * vertMargin + (msgLines + 1) * (textSize + textGap) + instrHeight;

   S32 boxTop = (canvasHeight - boxHeight) / 2 + vertOffset;

   S32 maxLen = 0;
   for(S32 i = 0; i < msgLines; i++)
   {
      S32 len = message[i]->getWidth() + MessageBoxPadding * 2;
      if(len > maxLen)
         maxLen = len;
   }

   S32 boxwidth = max(UIManager::MessageBoxWrapWidth, maxLen);
   S32 inset = (canvasWidth - boxwidth) / 2;   // Inset for left and right edges of box

   if(style == 1)       
      renderCenteredFancyBox(boxTop, boxHeight, inset, 15, Colors::red30, 1.0f, Colors::white);
   else if(style == 2)
      renderCenteredFancyBox(boxTop, boxHeight, inset, 15, Colors::black, 0.70f, Colors::blue);

   // Draw title
   title->render(gScreenInfo.getGameCanvasWidth() / 2, boxTop + vertMargin + TitleSize, AlignmentCenter);

   // Draw messages
   S32 y = boxTop + titleHeight + titleTextGap + textSize;

   for(S32 i = 0; i < msgLines; i++)
   {
      message[i]->render(gScreenInfo.getGameCanvasWidth() / 2, y, AlignmentCenter);
      y += message[i]->getHeight() + textGap;
   }

   // And footer
   if(instr)
      instr->render(gScreenInfo.getGameCanvasWidth() / 2, boxTop + boxHeight - vertMargin - instrGapBottom, AlignmentCenter);
}


// Static method
void UserInterface::dimUnderlyingUI(F32 amount)
{
   glColor(Colors::black, amount); 

   TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

   drawFilledRect (0, 0, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());
}


// Draw blue rectangle around selected menu item
void UserInterface::drawMenuItemHighlight(S32 x1, S32 y1, S32 x2, S32 y2, bool disabled)
{
   if(disabled)
      drawFilledRect(x1, y1, x2, y2, Colors::gray40, Colors::gray80);
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


InputCode UserInterface::getInputCode(GameSettings *settings, InputCodeManager::BindingNameEnum binding)
{
   return settings->getInputCodeManager()->getBinding(binding);
}


void UserInterface::setInputCode(GameSettings *settings, InputCodeManager::BindingNameEnum binding, InputCode inputCode)
{
   settings->getInputCodeManager()->setBinding(binding, inputCode);
}


bool UserInterface::checkInputCode(InputCodeManager::BindingNameEnum binding, InputCode inputCode)
{
   GameSettings *settings = getGame()->getSettings();

   InputCode bindingCode = getInputCode(settings, binding);

   // Handle modified keys
   if(InputCodeManager::isModified(bindingCode))
      return inputCode == InputCodeManager::getBaseKey(bindingCode) && 
             InputCodeManager::checkModifier(InputCodeManager::getModifier(bindingCode));

   // Else just do a simple key check.  filterInputCode deals with the numeric keypad.
   else
      return bindingCode == settings->getInputCodeManager()->filterInputCode(inputCode);
}


const char *UserInterface::getInputCodeString(GameSettings *settings, InputCodeManager::BindingNameEnum binding)
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

   UIManager *uiManager = getGame()->getUIManager();

   if(checkInputCode(InputCodeManager::BINDING_DIAG, inputCode))              // Turn on diagnostic overlay
   { 
      if(uiManager->isCurrentUI<DiagnosticUserInterface>())
         return false;

      uiManager->activate<DiagnosticUserInterface>();

      playBoop();
      
      handled = true;
   }
   else if(checkInputCode(InputCodeManager::BINDING_OUTGAMECHAT, inputCode))  // Turn on Global Chat overlay
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


//void UserInterfaceData::get()
//{
//   vertMargin  = UserInterface::vertMargin;
//   horizMargin = UserInterface::horizMargin;
//   chatMargin  = UserInterface::messageMargin;
//}
//
//
//void UserInterfaceData::set()
//{
//   UserInterface::vertMargin  = vertMargin;
//   UserInterface::horizMargin = horizMargin;
//   UserInterface::messageMargin = chatMargin;
//}


};

