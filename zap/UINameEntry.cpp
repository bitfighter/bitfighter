//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UINameEntry.h"

#include "FontManager.h"
#include "UIGame.h"
#include "UIEditor.h"      // Only used once, could probably be refactored out
#include "UIManager.h"
#include "UIMenus.h"

#include "ClientGame.h"
#include "DisplayManager.h"

#include "Colors.h"

#include "stringUtils.h"
#include "RenderUtils.h"

namespace Zap
{
using namespace std;

// Constructor
TextEntryUserInterface::TextEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   title = "ENTER TEXT:";
   instr1 = "";
   instr2 = "Enter some text above";
   setSecret(false);
   cursorPos = 0;
   resetOnActivate = true;
   lineEditor = LineEditor(MAX_PLAYER_NAME_LENGTH);
}

// Constructor
TextEntryUserInterface::~TextEntryUserInterface()
{
   // Do nothing
}


void TextEntryUserInterface::onActivate()
{
   if(resetOnActivate)
      lineEditor.clear();
}


static const S32 fontSize = 20;
static const S32 fontSizeBig = 30;
static const S32 TextEntryYPos = 325;


S32 TextEntryUserInterface::getFontSize() const
{
   S32 maxLineLength = 750;      // Pixels

   // Shrink the text to fit on-screen when text gets very long
   S32 w = RenderUtils::getStringWidthf(fontSizeBig, lineEditor.getDisplayString().c_str());
   if(w > maxLineLength)
      return S32(maxLineLength * (F32)fontSizeBig / (F32)w + 0.5f);
   else
      return fontSizeBig;
}


void TextEntryUserInterface::render() const
{
   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();

   // Center vertically
   S32 y = TextEntryYPos - 45;

   RenderUtils::drawCenteredString_fixed(y + fontSize, fontSize, Colors::white, title);
   y += 45;

   RenderUtils::drawCenteredString_fixed(canvasHeight - vertMargin - 2 * fontSize - 5, fontSize, Colors::green, instr1);
   RenderUtils::drawCenteredString_fixed(canvasHeight - vertMargin - fontSize,         fontSize, Colors::green, instr2);

   FontManager::pushFontContext(InputContext);

   S32 fontSizex = getFontSize();
   y += fontSizex;

   S32 x = (S32)RenderUtils::drawCenteredString_fixed(y, fontSizex, Colors::white, lineEditor.getDisplayString().c_str());
   lineEditor.drawCursor(x, y, fontSizeBig, Colors::white);
   FontManager::popFontContext();
}


void TextEntryUserInterface::setSecret(bool secret)
{
   lineEditor.setSecret(secret);
}


string TextEntryUserInterface::getText()
{
   return lineEditor.getString();
}


bool TextEntryUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   switch (inputCode)
   {
      case KEY_ENTER:
         onAccept(lineEditor.c_str());
         return true;
      case KEY_BACKSPACE:
         lineEditor.backspacePressed();
         return true;
      case KEY_DELETE:
         lineEditor.deletePressed();
         return true;
      case KEY_ESCAPE:
         onEscape();
         return true;
      default:
         return false;
   }
}


void TextEntryUserInterface::onTextInput(char ascii)
{
   lineEditor.addChar(ascii);
}


void TextEntryUserInterface::setString(string str)
{
   lineEditor.setString(str);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelNameEntryUserInterface::LevelNameEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   title = "ENTER LEVEL TO EDIT:";
   instr1 = "Enter an existing level, or create your own!";
   instr2 = "Arrows / wheel cycle existing levels | Tab completes partial name";
   resetOnActivate = false;
   lineEditor.setFilter(fileNameFilter);
   lineEditor.mMaxLen = MAX_FILE_NAME_LEN;

   mLevelIndex = 0;
   mFoundLevel = false;
}


// Destructor
LevelNameEntryUserInterface::~LevelNameEntryUserInterface()
{
   // Do nothing
}


void LevelNameEntryUserInterface::onEscape()
{
   playBoop();
   getUIManager()->reactivatePrevUI();      //gMainMenuUserInterface
}


void LevelNameEntryUserInterface::onActivate()
{
   Parent::onActivate();
   mLevelIndex = 0;

   mLevels = mGameSettings->getLevelList();

   // Remove the extension from the level file
   for(S32 i = 0; i < mLevels.size(); i++)
       mLevels[i] = stripExtension(mLevels[i]);

   mFoundLevel = setLevelIndex();
}


// See if the current level is on the list -- if so, set mLevelIndex to that level and return true
bool LevelNameEntryUserInterface::setLevelIndex()
{
   for(S32 i = 0; i < mLevels.size(); i++)
   {
      // Exact match
      if(mLevels[i] == lineEditor.getString())
      {
         mLevelIndex = i;
         return true;
      }
   }
   // is mLevels sorted correctly?
   for(S32 i = 0; i < mLevels.size(); i++)
   {
      // No exact match, but we just passed the item and have selected the closest one alphabetically following
      if(mLevels[i] > lineEditor.getString())
      {
         mLevelIndex = i;
         return false;
      }
   }

   mLevelIndex = 0;     // First item
   return false;
}


bool LevelNameEntryUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
   { 
      // Do nothing -- key handled
   }
   else if(inputCode == KEY_DOWN || inputCode == MOUSE_WHEEL_DOWN)
   {
      if(mLevels.size() == 0)
         return true;

      if(!mFoundLevel)           // If we have a partially entered name, just simulate hitting tab 
      {
         completePartial();      // Resets mFoundLevel
         if(!mFoundLevel)
            mLevelIndex--;       // Counteract increment below
      }

      mLevelIndex++;
      if(mLevelIndex >= mLevels.size())
         mLevelIndex = 0;

      lineEditor.setString(mLevels[mLevelIndex]);
   }

   else if(inputCode == KEY_UP || inputCode == MOUSE_WHEEL_UP)
   {
      if(mLevels.size() == 0)
         return true;

      if(!mFoundLevel)
         completePartial();

      mLevelIndex--;
      if(mLevelIndex < 0)
         mLevelIndex = mLevels.size() - 1;

      lineEditor.setString(mLevels[mLevelIndex]);
   }

   else if(inputCode == KEY_TAB)       // Tab will try to complete a name from whatever the user has already typed
      completePartial();

   else                                // Normal typed key - not handled
   {
      mFoundLevel = setLevelIndex();   // Update levelIndex to reflect current level
      return lineEditor.handleKey(inputCode);
   }

   // Something was handled!
   return true;
}


void LevelNameEntryUserInterface::completePartial()
{
   mFoundLevel = setLevelIndex();
   lineEditor.completePartial(&mLevels, lineEditor.getString(), 0, "", false);
   setLevelIndex();   // Update levelIndex to reflect current level
}


void LevelNameEntryUserInterface::onAccept(const char *name)
{
   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();
   ui->setLevelFileName(name);

   playBoop();
   getUIManager()->activate(ui, false);
   
   // Get that baby into the INI file
   mGameSettings->setSetting(IniKey::LastEditorName, string(name));
   saveSettingsToINI(&GameSettings::iniFile, mGameSettings);
   // Should be...
   //getGame()->getIniSettings()->saveSettingsToDisk();
}


void LevelNameEntryUserInterface::render() const
{
   static const S32 linesBefore = 6;
   static const S32 linesAfter = 3;

   S32 startIndex = MAX(0, mLevelIndex - linesBefore);
   S32 endIndex = MIN(mLevels.size() - 1, mLevelIndex + linesAfter);
   S32 size = getFontSize();

   for(S32 i = startIndex; i <= endIndex; i++)
   {
      if(i != mLevelIndex)
      {
         S32 y = TextEntryYPos + (i - mLevelIndex) * (fontSize * 2) + size;
         RenderUtils::drawCenteredString_fixed(y, size, Colors::gray20, mLevels[i].c_str());
      }
   }

   Parent::render();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
PasswordEntryUserInterface::PasswordEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   setSecret(true);
}


// Destructor
PasswordEntryUserInterface::~PasswordEntryUserInterface()
{
   // Do nothing
}


void PasswordEntryUserInterface::render() const
{
   if(getGame()->getConnectionToServer())
   {
      getUIManager()->getUI<GameUserInterface>()->render();
      dimUnderlyingUI(0.5f);
   }

   Parent::render();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ServerAccessPasswordEntryUserInterface::ServerAccessPasswordEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   // Do nothing
}


ServerAccessPasswordEntryUserInterface::~ServerAccessPasswordEntryUserInterface()
{
   // Do nothing
}


void ServerAccessPasswordEntryUserInterface::onAccept(const char *text)
{
   getGame()->submitServerAccessPassword(mConnectAddress, text);
}


void ServerAccessPasswordEntryUserInterface::onEscape()
{
   getUIManager()->activate<MainMenuUserInterface>();
}


void ServerAccessPasswordEntryUserInterface::setAddressToConnectTo(const Address &address)
{
   mConnectAddress = address;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ServerPasswordEntryUserInterface::ServerPasswordEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   title = "ENTER SERVER PASSWORD:";
   instr1 = "";
   instr2 = "Enter the password required for access to the server";
}


// Destructor
ServerPasswordEntryUserInterface::~ServerPasswordEntryUserInterface()
{
   // Do nothing
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
LevelChangeOrAdminPasswordEntryUserInterface::LevelChangeOrAdminPasswordEntryUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   title = "ENTER PASSWORD:";
   instr1 = "";
   instr2 = "Enter level change or admin password to change levels on this server";
}


// Destructor
LevelChangeOrAdminPasswordEntryUserInterface::~LevelChangeOrAdminPasswordEntryUserInterface()
{
   // Do nothing
}


void LevelChangeOrAdminPasswordEntryUserInterface::onAccept(const char *text)
{
   bool submitting = getGame()->submitServerPermissionsPassword(text);

   if(submitting)
   {
      getUIManager()->reactivatePrevUI();                                      // Reactivating clears subtitle message, so reactivate first...
      getUIManager()->getUI<GameMenuUserInterface>()->setSubtitle("** checking password **");     // ...then set the message
   }
   else
      getUIManager()->reactivatePrevUI();                                      // Otherwise, just reactivate the previous menu
}


void LevelChangeOrAdminPasswordEntryUserInterface::onEscape()
{
   getUIManager()->reactivatePrevUI();
}


////////////////////////////////////////
////////////////////////////////////////
};


