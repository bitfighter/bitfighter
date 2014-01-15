////------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UITeamDefMenu.h"

#include "UIEditor.h"
#include "UIManager.h"

#include "EditorTeam.h"
#include "DisplayManager.h"
#include "ClientGame.h"
#include "Cursor.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"
#include "stringUtils.h"

#include "UIColorPicker.h"

#include "FontManager.h"

#include <string>

namespace Zap
{

// Note: Do not make any of the following team names longer than MAX_TEAM_NAME_LENGTH, which is currently 32
// Note: Make sure we have at least 9 presets below...  (instructions are wired for keys 1-9)
TeamPreset gTeamPresets[] = {
   { "Blue",        0,     0,    1 },
   { "Red",         1,     0,    0 },
   { "Yellow",      1,     1,    0 },
   { "Green",       0,     1,    0 },
   { "Pink",        1, .45f, .875f },
   { "Orange",      1,  .67f,    0 },
   { "Lilac",     .79f,   .5, .96f },
   { "LightBlue", .45f, .875f,   1 },
   { "Ruby",      .67f,    0,    0 },
};


// Other ideas
//Team Blue 0 0 1
//Team Red 1 0 0
//Team Green 0 1 0
//Team Yellow 1 1 0
//Team Turquoise 0 1 1
//Team Pink 1 0 1
//Team Orange 1 0.5 0
//Team Black 0 0 0
//Team White 1 1 1
//Team Sapphire 0 0 0.7
//Team Ruby 0.7 0 0
//Team Emerald 0 0.7 0
//Team Lime 0.8 1 0
//Team DarkAngel 0 0.7 0.7
//Team Purple 0.7 0 0.7
//Team Peach 1 0.7 0


namespace UI
{

static SymbolString getSymbolString(const string &text, const InputCodeManager *inputCodeManager, S32 size, const Color &color)
{
   Vector<SymbolShapePtr> symbols;

   SymbolString::symbolParse(inputCodeManager, text, symbols, MenuContext, size, &color);
   return SymbolString(symbols, AlignmentCenter);

   //symbolStringSet.add(SymbolString(symbols, AlignmentCenter));
}

}

// Constructor
TeamDefUserInterface::TeamDefUserInterface(ClientGame *game) : 
   Parent(game),
   mMenuSubTitle(8),
   mMenuTitle("CONFIGURE TEAMS")
{
   

   InputCodeManager *inputCodeManager = getGame()->getSettings()->getInputCodeManager();

   mTopInstructions =  getSymbolString("For quick configuration, press [[Alt+1]] - [[Alt+9]] to specify number of teams",
                                             inputCodeManager, 18, Colors::menuHelpColor);

   // Text at the bottom of the screen
   mBottomInstructions1 =  getSymbolString("[[1]] - [[9]] selects a team preset for current slot",
                                           inputCodeManager, 16, Colors::menuHelpColor);
   mBottomInstructions2 =  getSymbolString("[[Enter]] edits team name | [[C]] shows Color Picker | [[M]] changes color entry mode",
                                          inputCodeManager, 16, Colors::menuHelpColor);
   mBottomInstructions3a = getSymbolString("[[R]] [[G]] [[B]] to change preset color (with or without [[Shift]])",
                                          inputCodeManager, 16, Colors::menuHelpColor);
   mBottomInstructions3b = getSymbolString("[[H]] to edit color hex value",
                                          inputCodeManager, 16, Colors::menuHelpColor);
   mBottomInstructions4 =  getSymbolString("[[Insert]] or [[+]] to insert team | [[Del]] or [[-]] to remove selected team",
                                          inputCodeManager, 16, Colors::menuHelpColor);

   mColorEntryMode = ColorEntryMode100;      // TODO: Get this from INI to make this setting persistent
}


// Destructor
TeamDefUserInterface::~TeamDefUserInterface()
{
   // Do nothing
}


static const U32 errorMsgDisplayTime = FOUR_SECONDS;
static const S32 fontsize = 19;
static const S32 fontgap = 12;
static const U32 yStart = UserInterface::vertMargin + 90;
static const U32 itemHeight = fontsize + 5;

void TeamDefUserInterface::onActivate()
{
   selectedIndex = 0;                        // First item selected when we begin
   mEditingTeam = mEditingColor = false;     // Not editing anything by default

   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();
   S32 teamCount = ui->getTeamCount();

   ui->mOldTeams.resize(teamCount);  // Avoid unnecessary reallocations

   for(S32 i = 0; i < teamCount; i++)
   {
      EditorTeam *team = ui->getTeam(i);

      ui->mOldTeams[i].color = *team->getColor();
      ui->mOldTeams[i].name = team->getName().getString();
   }

   // Display an intitial message to users
   errorMsgTimer.reset(errorMsgDisplayTime);
   errorMsg = "";
   Cursor::disableCursor();
}


void TeamDefUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(errorMsgTimer.update(timeDelta))
      errorMsg = "";
}


// TODO: Clean this up a bit...  this menu was two-cols before, and some of that garbage is still here...
void TeamDefUserInterface::render()
{
   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();

   FontManager::pushFontContext(MenuHeaderContext);
   glColor(Colors::green);
   drawCenteredUnderlinedString(vertMargin, 30, mMenuTitle);
   
   //mMenuSubTitle.render(canvasWidth / 2, vertMargin + 65, UI::AlignmentCenter); 
   drawCenteredString(canvasHeight - vertMargin - 20, 18, "Arrow Keys to choose | ESC to exit");

   glColor(Colors::white);

   S32 x = canvasWidth / 2;

   mTopInstructions.render(x, 83);

   S32 y = canvasHeight - vertMargin - 116;
   S32 gap = 28;

   mBottomInstructions1.render(x, y);
   y += gap;

   mBottomInstructions2.render(x, y);
   y += gap;

   if(mColorEntryMode != ColorEntryModeHex)
      mBottomInstructions3a.render(x, y);
   else
      mBottomInstructions3b.render(x, y);
   y += gap;

   mBottomInstructions4.render(x, y);

   FontManager::popFontContext();


   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   S32 size = ui->getTeamCount();

   if(selectedIndex >= size)
      selectedIndex = 0;


   // Draw the fixed teams
   glColor(Colors::NeutralTeamColor);
   drawCenteredStringf(yStart, fontsize, "Neutral Team (can't change)");
   glColor(Colors::HostileTeamColor);
   drawCenteredStringf(yStart + fontsize + fontgap, fontsize, "Hostile Team (can't change)");

   for(S32 j = 0; j < size; j++)
   {
      S32 i = j + 2;    // Take account of the two fixed teams (neutral & hostile)

      U32 y = yStart + i * (fontsize + fontgap);

      if(selectedIndex == j)       // Highlight selected item
         drawMenuItemHighlight(0, y - 2, canvasWidth, y + itemHeight + 2);

      if(j < ui->getTeamCount())
      {
         string numstr = "Team " + itos(j + 1) + ": ";
         string namestr = numstr + ui->getTeam(j)->getName().getString();
         
         string colorstr;

         const Color *color = ui->getGame()->getTeamColor(j);

         if(mColorEntryMode == ColorEntryModeHex)
            colorstr = "#" + ui->getTeam(j)->getHexColorEditor()->getString();
         else
         {
            F32 multiplier;

            if(mColorEntryMode == ColorEntryMode100)
               multiplier = 100;
            else if(mColorEntryMode == ColorEntryMode255)
               multiplier = 255;
            else
               TNLAssert(false, "Unknown entry mode!");

            colorstr = "(" + itos(S32(color->r * multiplier + 0.5)) + ", " + 
                             itos(S32(color->g * multiplier + 0.5)) + ", " +
                             itos(S32(color->b * multiplier + 0.5)) + ")";
         }
         
         static const string spacer1 = "  ";
         string nameColorStr = namestr + spacer1 + colorstr + " " + getEntryMessage();

         // Draw item text
         glColor(color);
         drawCenteredString(y, fontsize, nameColorStr.c_str());

         // Draw cursor if we're editing
         if(j == selectedIndex)
         {
            if(mEditingTeam)
            {
               S32 x = getCenteredStringStartingPos(fontsize, nameColorStr.c_str()) + 
                       getStringWidth(fontsize, numstr.c_str());
                       

               ui->getTeam(j)->getTeamNameEditor()->drawCursor(x, y, fontsize);
            }
            else if(mEditingColor)
            {
               S32 x = getCenteredStringStartingPos(fontsize, nameColorStr.c_str()) + 
                       getStringWidth(fontsize, namestr.c_str()) +
                       getStringWidth(fontsize, spacer1.c_str()) +
                       getStringWidth(fontsize, "#");

               ui->getTeam(j)->getHexColorEditor()->drawCursor(x, y, fontsize);
            }
         }
      }
   }

   if(errorMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (errorMsgTimer.getCurrent() < ONE_SECOND)
         alpha = (F32) errorMsgTimer.getCurrent() / ONE_SECOND;

      glColor(Colors::red, alpha);
      drawCenteredString(canvasHeight - vertMargin - 141, fontsize, errorMsg.c_str());
   }
}


// Run this as we're exiting the menu
void TeamDefUserInterface::onEscape()
{
   // Make sure there is at least one team left...
   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   ui->makeSureThereIsAtLeastOneTeam();
   ui->teamsHaveChanged();

   getUIManager()->reactivatePrevUI();
}


class Team;
string origName;
extern bool isPrintable(char c);

void TeamDefUserInterface::onTextInput(char ascii)
{
   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   if(mEditingTeam)
   {
      if(isPrintable(ascii))
         ui->getTeam(selectedIndex)->getTeamNameEditor()->addChar(ascii);
   }

   else if(mEditingColor)
   {
      if(isHex(ascii))
         ui->getTeam(selectedIndex)->getHexColorEditor()->addChar(ascii);
   }
}


bool TeamDefUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;

   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   // If we're editing, need to send keypresses to editor
   if(mEditingTeam)
   {
      if(inputCode == KEY_ENTER)          // Finish editing
      {
         mEditingTeam = false;
      }
      else if(inputCode == KEY_ESCAPE)    // Stop editing, and restore the original value
      {
         ui->getTeam(selectedIndex)->setName(origName.c_str());
         mEditingTeam = false;
      }
      else
         return ui->getTeam(selectedIndex)->getTeamNameEditor()->handleKey(inputCode);

      return true;
   }

   else if(mEditingColor)
   {
      if(inputCode == KEY_ENTER)          // Finish editing
      {
         mEditingColor = false;
         ui->getTeam(selectedIndex)->setColor(Color(ui->getTeam(selectedIndex)->getHexColorEditor()->getString()));
      }
      else if(inputCode == KEY_ESCAPE)    // Stop editing, and restore the original value
      {
         mEditingColor = false;
         ui->getTeam(selectedIndex)->setColor(*ui->getTeam(selectedIndex)->getColor());   // Will reset hexColorEditor. Ugly!
      }
      else
         return ui->getTeam(selectedIndex)->getHexColorEditor()->handleKey(inputCode);

      return true;
   }

   // Not editing, normal key processing follows

   if(inputCode == KEY_ENTER)
   {
      mEditingTeam = true;
      origName = ui->getTeam(selectedIndex)->getName().getString();
   }

   else if(inputCode == KEY_H)
   {
      if(mColorEntryMode != ColorEntryModeHex)
         return true;
      
      mEditingColor = true;
   }

   else if(inputCode == KEY_DELETE || inputCode == KEY_MINUS)            // Del or Minus - Delete current team
   {
      if(ui->getTeamCount() == 1) 
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "There must be at least one team";
         return true;
      }

      ui->removeTeam(selectedIndex);
      if(selectedIndex >= ui->getTeamCount())
         selectedIndex = ui->getTeamCount() - 1;
   }
  
   else if(inputCode == KEY_INSERT || inputCode == KEY_EQUALS)           // Ins or Plus (equals) - Add new item
   {
      S32 teamCount = ui->getTeamCount();

      if(teamCount >= Game::MAX_TEAMS)
      {
         errorMsgTimer.reset(errorMsgDisplayTime);
         errorMsg = "Too many teams for this interface";
         return true;
      }

      S32 presetIndex = teamCount % Game::MAX_TEAMS;

      EditorTeam *team = new EditorTeam(gTeamPresets[presetIndex]);
      ui->addTeam(team, teamCount);

      selectedIndex++;

      if(selectedIndex < 0)      // It can happen with too many deletes
         selectedIndex = 0;
   }

   else if(inputCode == KEY_R)
   {
      if(mColorEntryMode != ColorEntryModeHex)
         ui->getTeam(selectedIndex)->alterRed(getAmount());
   }

   else if(inputCode == KEY_G)
   {
      if(mColorEntryMode != ColorEntryModeHex)
         ui->getTeam(selectedIndex)->alterGreen(getAmount());
   }

   else if(inputCode == KEY_B)
   {
      if(mColorEntryMode != ColorEntryModeHex)
         ui->getTeam(selectedIndex)->alterBlue(getAmount());
   }

   else if(inputCode == KEY_C)  // Want a mouse button?   || inputCode == MOUSE_LEFT)
   {
      UIColorPicker *uiCol = getUIManager()->getUI<UIColorPicker>();
      *((Color *)uiCol) = *(ui->getTeam(selectedIndex)->getColor());
      getUIManager()->activate(uiCol);
   }

   else if(inputCode == KEY_M)      // Toggle ColorEntryMode
   {
      mColorEntryMode = ColorEntryMode(mColorEntryMode + 1);

      if(mColorEntryMode >= ColorEntryModeCount)
         mColorEntryMode = ColorEntryMode(0);
   }

   else if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)       // Quit
   {
      playBoop();
      onEscape();
   }
   else if(inputCode == KEY_UP || inputCode == BUTTON_DPAD_UP)        // Prev item
   {
      selectedIndex--;
      if(selectedIndex < 0)
         selectedIndex = ui->getTeamCount() - 1;
      playBoop();
      Cursor::disableCursor();

   }
   else if(inputCode == KEY_DOWN || inputCode == BUTTON_DPAD_DOWN)    // Next item
   {
      selectedIndex++;
      if(selectedIndex >= ui->getTeamCount())
         selectedIndex = 0;
      playBoop();
      Cursor::disableCursor();
   }

   // Keys 1-9 --> use team preset
   else if(inputCode >= KEY_1 && inputCode <= KEY_9)
   {
      // Replace all teams with # of teams based on presets
      if(InputCodeManager::checkModifier(KEY_ALT))
      {
         U32 count = (inputCode - KEY_1) + 1;
         ui->clearTeams();
         for(U32 i = 0; i < count; i++)
         {
            EditorTeam *team = new EditorTeam(gTeamPresets[i]);
            ui->addTeam(team);
         }
      }
      // Replace selection with preset of number pressed
      else
      {
         U32 index = (inputCode - KEY_1);
         ui->getTeam(selectedIndex)->setName(gTeamPresets[index].name);
         ui->getTeam(selectedIndex)->setColor(gTeamPresets[index].r, gTeamPresets[index].g, gTeamPresets[index].b);
      }
   }
   else
      return false;

   // A key was handled
   return true;
}


F32 TeamDefUserInterface::getAmount() const
{
   F32 s = InputCodeManager::checkModifier(KEY_SHIFT) ? -1.0f : 1.0f;
   return s / getColorBase();
}


F32 TeamDefUserInterface::getColorBase() const
{
   if(mColorEntryMode == ColorEntryMode100)
      return 100.0f;
   else if(mColorEntryMode == ColorEntryMode255)
      return 255.0f;
   else
      return 1;
}


const char *TeamDefUserInterface::getEntryMessage() const
{
   if(mColorEntryMode == ColorEntryMode100)
      return "[base 100]";
   else if(mColorEntryMode == ColorEntryMode255)
      return "[base 255]";
   else
      return "";
}


void TeamDefUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   Cursor::enableCursor();

   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();

   S32 teams = ui->getTeamCount();

   selectedIndex = (S32)((DisplayManager::getScreenInfo()->getMousePos()->y - yStart + 6) / (fontsize + fontgap)) - 2; 

   if(selectedIndex >= teams)
      selectedIndex = teams - 1;

   if(selectedIndex < 0)
      selectedIndex = 0;
}


void TeamDefUserInterface::onColorPicked(const Color &color)
{
   EditorUserInterface *ui = getUIManager()->getUI<EditorUserInterface>();
   ui->getTeam(selectedIndex)->setColor(color);
}



}

