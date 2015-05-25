//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIDiagnostics.h"

#include "UIMenus.h"
#include "UIManager.h"

#include "masterConnection.h"
#include "DisplayManager.h"
#include "SymbolShape.h"
#include "JoystickRender.h"
#include "Joystick.h"
#include "ClientGame.h"
#include "config.h"
#include "version.h"

#include "GameManager.h"
#include "ServerGame.h"          

#include "Colors.h"
#include "GameObjectRender.h"    // For drawCircle in badge rendering below
#include "SymbolShape.h"

#include "stringUtils.h"
#include "RenderUtils.h"

#include "tnl.h"

#include <cmath>


namespace Zap
{

static const char *pageHeaders[] = {
   "PLAYING",
   "FOLDERS",
   "HOSTING"
};

static const S32 NUM_PAGES = 3;



// Constructor
DiagnosticUserInterface::DiagnosticUserInterface(ClientGame *game, UIManager *uiManager) : 
   Parent(game, uiManager)
{
   mActive = false;
   mCurPage = 0;
}

// Destructor
DiagnosticUserInterface::~DiagnosticUserInterface()
{
   // Do nothing
}


void DiagnosticUserInterface::onActivate()
{
   mActive = true;
   mCurPage = 0;
}


void DiagnosticUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);
}


bool DiagnosticUserInterface::isActive() const
{
   return mActive;
}


void DiagnosticUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();  // Back to our previously scheduled program!
   mActive = false;
}


bool DiagnosticUserInterface::onKeyDown(InputCode inputCode)
{
   if(checkInputCode(BINDING_DIAG, inputCode))
   {
      mCurPage++;
      if(mCurPage >= NUM_PAGES)
         quit();
   }
   else if(checkInputCode(BINDING_LOBBYCHAT, inputCode))
   {
      // Do nothing -- no lobby chat from diagnostics screen... it would be perverse!
   }
   else if(Parent::onKeyDown(inputCode))
   { 
      // Do nothing -- key handled
   }
   else if(inputCode == KEY_ESCAPE)
      quit();                          // Quit the interface
   else
      return false;

   // A key was handled
   return true;
}


S32 findLongestString(F32 size, const Vector<string> &strings)
{
   F32 maxLen = 0;
   S32 longest = 0;

   for(S32 i = 0; i < strings.size(); i++)
   {
      F32 len = RenderUtils::getStringWidth(size, strings[i]);
      if(len > maxLen)
      {
         maxLen = len;
         longest = i;
      }
   }
   return longest;
}


static Vector<string>names;
static Vector<string>vals;

static S32 longestName;
static S32 nameWidth;
static S32 spaceWidth;
static S32 longestVal;
static S32 totLen;

void DiagnosticUserInterface::initFoldersBlock(FolderManager *folderManager, S32 textsize)
{
   const string levelDir = folderManager->getLevelDir();
   names.push_back("Level Dir:");
   vals.push_back(levelDir.empty() ? "<<Unresolvable>>" : levelDir);

   names.push_back("");
   vals.push_back("");

   names.push_back("INI Dir:");
   vals.push_back(folderManager->getIniDir());
                                            
   names.push_back("Log Dir:");             
   vals.push_back(folderManager->getLogDir());
                                            
   names.push_back("Lua Dir:");             
   vals.push_back(folderManager->getLuaDir());
      
   names.push_back("Robot Dir:");
   vals.push_back(folderManager->getRobotDir());
      
   names.push_back("Screenshot Dir:");
   vals.push_back(folderManager->getScreenshotDir());
      
   names.push_back("SFX Dirs:");
   vals.push_back(listToString(folderManager->getSfxDirs(), "; "));

   names.push_back("Music Dir:");
   vals.push_back(folderManager->getMusicDir());

   names.push_back("Fonts Dirs:");
   vals.push_back(listToString(folderManager->getFontDirs(), "; "));

   names.push_back("");
   vals.push_back("");

   names.push_back("Root Data Dir:");
   vals.push_back(folderManager->getRootDataDir() == "" ? "None specified" : folderManager->getRootDataDir());

   longestName = findLongestString((F32)textsize, names);
   longestVal  = findLongestString((F32)textsize, vals);

   nameWidth   = RenderUtils::getStringWidth(textsize, names[longestName]);
   spaceWidth  = RenderUtils::getStringWidth(textsize, " ");

   totLen = nameWidth + spaceWidth + RenderUtils::getStringWidth(textsize, vals[longestVal]);
}


S32 DiagnosticUserInterface::showFoldersBlock(FolderManager *folderManager, F32 textsize, S32 ypos, S32 gap)
{
   if(names.size() == 0)      // Lazy init
      initFoldersBlock(folderManager, (S32)textsize);

   for(S32 i = 0; i < names.size(); i++)
   {
      S32 xpos = (DisplayManager::getScreenInfo()->getGameCanvasWidth() - totLen) / 2;
      mGL->glColor(Colors::cyan);
      RenderUtils::drawString(xpos, ypos, (S32)textsize, names[i].c_str());
      xpos += nameWidth + spaceWidth;
      mGL->glColor(Colors::white);
      RenderUtils::drawString(xpos, ypos, (S32)textsize, vals[i].c_str());

      ypos += (S32)textsize + gap;
   }

   return ypos;
}


// This should be calculated only once, on build time
static const string buildDate = __DATE__;

// This is too long to use right now
//static const string buildDate = __DATE__ " " __TIME__;

S32 DiagnosticUserInterface::showVersionBlock(S32 ypos, S32 textsize, S32 gap)
{
   mGL->glColor(Colors::white);

   S32 x = RenderUtils::getCenteredStringStartingPosf(textsize, "M/C Ver: %d | C/S Ver: %d | Build: %s/%d | Date: %s | CPU: %s | OS: %s | Cmplr: %s",
           MASTER_PROTOCOL_VERSION, CS_PROTOCOL_VERSION, ZAP_GAME_RELEASE, BUILD_VERSION, TNL_CPU_STRING, TNL_OS_STRING, TNL_COMPILER_STRING, buildDate.c_str());

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "M/C Ver: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%d", MASTER_PROTOCOL_VERSION);

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | C/S Ver: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%d", CS_PROTOCOL_VERSION);

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | Build: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%d", BUILD_VERSION);
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "/");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", ZAP_GAME_RELEASE);

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | Date: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", buildDate.c_str());

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | CPU: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_CPU_STRING);

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | OS: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_OS_STRING);

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | Cmplr: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", TNL_COMPILER_STRING);

   return ypos + textsize + gap * 2;
}


S32 DiagnosticUserInterface::showNameDescrBlock(const string &hostName, const string &hostDescr, S32 ypos, S32 textsize, S32 gap)
{
   S32 x = RenderUtils::getCenteredStringStartingPosf(textsize, "Server Name: %s | Descr: %s", hostName.c_str(), hostDescr.c_str());

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "Server Name: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", hostName.c_str());

   mGL->glColor(Colors::white);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, " | Descr: ");
   mGL->glColor(Colors::yellow);
   x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%s", hostDescr.c_str());

   return ypos + textsize + gap;
}


S32 DiagnosticUserInterface::showMasterBlock(ClientGame *game, S32 textsize, S32 ypos, S32 gap, bool leftcol)
{
   RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, leftcol, "Master Srvr Addr:", "%s",
                                              game->getSettings()->getMasterServerList()->size() > 0 ? 
                                                         game->getSettings()->getMasterServerList()->get(0).c_str() : "None");

   ypos += textsize + gap;

   if(game->getConnectionToMaster() && game->getConnectionToMaster()->isEstablished())
   {
      mGL->glColor(Colors::MasterServerBlue);
      RenderUtils::drawCenteredString2Colf(ypos, textsize, leftcol, "Connected to [%s]",
                                             game->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      mGL->glColor(Colors::red);
      RenderUtils::drawCenteredString2Col(ypos, textsize, leftcol, "Not connected to Master Server" );
   }

   return ypos + textsize + gap;
}


void DiagnosticUserInterface::render() const
{
   // Draw title, subtitle, and footer
   mGL->glColor(Colors::red);
   RenderUtils::drawStringf(  3, 3, 25, "DIAGNOSTICS - %s", pageHeaders[mCurPage]);
   RenderUtils::drawStringf(625, 3, 25, "PAGE %d/%d",       mCurPage + 1, NUM_PAGES);
 
   RenderUtils::drawCenteredStringf(571, 20, "%s - next page  ESC exits", getInputCodeString(BINDING_DIAG));

   mGL->glColor(0.7f);
   RenderUtils::drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), 31);
   RenderUtils::drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), 569);

   S32 textsize = 14;

   if(mCurPage == 0)
   {
      string inputMode = mGameSettings->getInputCodeManager()->getInputModeString();

      mGL->glColor(Colors::red);
      RenderUtils::drawCenteredString(vertMargin + 37, 18, "Is something wrong?");

      S32 x, y;
      x = RenderUtils::getCenteredStringStartingPosf(textsize, "Can't control your ship? Check your input mode "
                                                  "(Options>Primary Input) [currently %s]", inputMode.c_str());
      mGL->glColor(Colors::green);
      y = vertMargin + 63;
      x += RenderUtils::drawStringAndGetWidth(x, y, textsize, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");

      mGL->glColor(Colors::red);
      x += RenderUtils::drawStringAndGetWidth(x, y, textsize, inputMode.c_str());

      mGL->glColor(Colors::green);
      RenderUtils::drawString(x, y, textsize, "]");

      // Box around something wrong? block
      mGL->glColor(Colors::cyan);
      RenderUtils::drawHollowRect(horizMargin, vertMargin + 27, DisplayManager::getScreenInfo()->getGameCanvasWidth() - horizMargin, vertMargin + 90);

      const S32 gap = 5;

      S32 ypos = showVersionBlock(120, textsize - 2, gap);

      mGL->glColor(Colors::white);

      textsize = 16;

      bool needToUpgrade = getUIManager()->getUI<MainMenuUserInterface>()->getNeedToUpgrade();

      RenderUtils::drawCenteredString2Colf(ypos, textsize, false, "%s", needToUpgrade ? "<<Update available>>" : "<<Current version>>");
      ypos += textsize + gap;

      ClientInfo *clientInfo = getGame()->getClientInfo();

      // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
      // If the server modifies a user name to make it unique, this will display the modified version.
      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, true, "Nickname:", "%s (%s)",
                                  clientInfo->getName().getString(), 
                                  clientInfo->isAuthenticated() ? 
                                       string("Verified - " + itos(clientInfo->getBadges())).c_str() : "Not verified");

      ypos += textsize + gap;

      showMasterBlock(getGame(), textsize, ypos, gap, false);

      ypos += textsize + gap;
      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, true, "Input Mode:", "%s", inputMode.c_str());
  
      ypos += textsize + gap;
      
      S32 index = GameSettings::UseJoystickNumber;

      bool joystickDetected = GameSettings::DetectedJoystickNameList.size() > 0;

      if(joystickDetected && getGame()->getInputMode() == InputModeKeyboard)
      {
         RenderUtils::drawCenteredString(400, textsize, "Joystick not enabled, you may set input mode to Joystick in option menu.");
         joystickDetected = false;
      }
      else if(!joystickDetected)
         RenderUtils::drawCenteredString2Col(ypos, textsize, true, "No joysticks detected");
      else
      {
         // Draw which profile we're using
         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, true, "Current Profile:", "%s", Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].name.c_str());

         // Draw the raw SDL detection string
         RenderUtils::drawCenteredStringPair2Colf(ypos + textsize + gap, textsize, true, Colors::magenta, Colors::cyan, "Autodetect String:", "%s",
               (U32(index) >= U32(GameSettings::DetectedJoystickNameList.size()) || 
                GameSettings::DetectedJoystickNameList[index] == "") ? 
                          "<None>" : GameSettings::DetectedJoystickNameList[index].c_str());
      }

      ypos += 6 * (textsize + gap);

      if(joystickDetected)
      {
         S32 x = 500;
         S32 y = 290;

         mGL->glColor(Colors::white);
         RenderUtils::drawString(x, y, textsize - 2, "Raw Analog Axis Values:");

         y += 25;

         for(S32 i = 0; i < Joystick::rawAxisCount; i++)
         {
            F32 a = Joystick::rawAxis[i];    // Range: -1 to 1
            if(fabs(a) > .1f)
            {
               mGL->glColor(Colors::cyan);
               S32 len = RenderUtils::drawStringAndGetWidthf(x, y, textsize - 2, "Axis %d", i);

               mGL->glColor(Colors::red);
               RenderUtils::drawHorizLine(x, x + len, y + textsize + 3);

               mGL->glColor(Colors::yellow);
               RenderUtils::drawHorizLine(x + len / 2, x + len / 2 + S32(a * F32(len / 2)), y + textsize + 3);

               x += len + 8;
            }
         }
      }

      // Key states
      mGL->glColor(Colors::yellow);
      S32 hpos = horizMargin;

      hpos += RenderUtils::drawStringAndGetWidth(hpos, ypos, textsize, "Keys down: ");

      mGL->glColor(Colors::red);
      for(U32 i = 0; i < MAX_INPUT_CODES; i++)
      {
         InputCode inputCode = InputCode(i);
         if(InputCodeManager::getState(inputCode))
         {
            UI::SymbolKey key = UI::SymbolKey(InputCodeManager::inputCodeToString(inputCode));
            key.render(hpos, ypos + textsize, UI::AlignmentLeft);
            hpos += key.getWidth() + 5;
         }
      }


      mGL->glColor(Colors::cyan);
      hpos += RenderUtils::drawStringAndGetWidth(hpos, ypos, textsize, " | ");

      mGL->glColor(Colors::yellow);
      hpos += RenderUtils::drawStringAndGetWidth(hpos, ypos, textsize, "Input string: ");

      mGL->glColor(Colors::magenta);

      string in = InputCodeManager::getCurrentInputString(KEY_NONE);

      if(in != "")
      {
         UI::SymbolShapePtr key = UI::SymbolString::getModifiedKeySymbol(in, NULL);

         key->render(hpos, ypos + textsize, UI::AlignmentLeft);
         hpos += key->getWidth() + 5;
      }


      if(joystickDetected)
      {
         mGL->glColor(Colors::magenta);
         ypos += textsize + gap;
         hpos = horizMargin;

         hpos += RenderUtils::drawStringAndGetWidthf(hpos, ypos, textsize - 2, "Raw Controller Input [%d]: ", GameSettings::UseJoystickNumber);

         for(U32 i = 0; i < 32; i++)  // there are 32 bit in U32
            if(Joystick::ButtonMask & BIT(i))
               hpos += RenderUtils::drawStringAndGetWidthf( hpos, ypos, textsize - 2, "(%d)", i ) + 5;

         // TODO render raw D-pad (SDL Hat)

         ypos += textsize + gap + 10;

         mGL->glColor(Colors::green);
         RenderUtils::drawCenteredString(ypos, textsize, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

         //////////
         // Draw joystick and button map
         hpos = 100;
         ypos = DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - 110;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(BUTTON_DPAD_UP),   InputCodeManager::getState(BUTTON_DPAD_DOWN),
               InputCodeManager::getState(BUTTON_DPAD_LEFT), InputCodeManager::getState(BUTTON_DPAD_RIGHT), 
               "DPad", "(Menu Nav)");
         hpos += 75;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(STICK_1_UP),   InputCodeManager::getState(STICK_1_DOWN),
               InputCodeManager::getState(STICK_1_LEFT), InputCodeManager::getState(STICK_1_RIGHT), 
               "L Stick", "(Move)");
         hpos += 75;

         JoystickRender::renderDPad(Point(hpos, ypos), 25, 
               InputCodeManager::getState(STICK_2_UP),   InputCodeManager::getState(STICK_2_DOWN),
               InputCodeManager::getState(STICK_2_LEFT), InputCodeManager::getState(STICK_2_RIGHT), 
               "R Stick", "(Fire)");
         hpos += 55;

         U32 joystickIndex = Joystick::SelectedPresetIndex;

         Vector<UI::SymbolShapePtr> symbols;

         S32 buttonCount = LAST_CONTROLLER_BUTTON - FIRST_CONTROLLER_BUTTON + 1;
         for(S32 i = 0; i < buttonCount; i++)
         {
            if(!Joystick::isButtonDefined(Joystick::SelectedPresetIndex, i))
               continue;

            symbols.push_back(UI::SymbolString::getControlSymbol(InputCode(i + FIRST_CONTROLLER_BUTTON)));
            if(i < buttonCount - 1)
               symbols.push_back(UI::SymbolString::getBlankSymbol(8));      // Provide a little breathing room
         }

         UI::SymbolString(symbols).render(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2 + 100, ypos + 50));

         for(U32 i = FIRST_CONTROLLER_BUTTON; i <= LAST_CONTROLLER_BUTTON; i++)
         {
            InputCode code = InputCode(i);
            const Color *color = InputCodeManager::getState(code) ? &Colors::red : NULL;

            // renderControllerButton() returns false if nothing is rendered
            if(JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, joystickIndex, code, color))
               hpos += 40;
         }
      }
   }
   else if(mCurPage == 1)
   {
      S32 ypos = vertMargin + 35;
      S32 textsize = 15;
      S32 gap = 5;

      RenderUtils::drawString(horizMargin, ypos, textsize, "Folders are either an absolute path or a path relative to the program execution folder");
      ypos += textsize + gap;
      RenderUtils::drawString(horizMargin, ypos, textsize, "or local folder, depending on OS.  If an entry is blank, Bitfighter will look for files");
      ypos += textsize + gap;
      RenderUtils::drawString(horizMargin, ypos, textsize, "in the program folder or local folder, depending on OS.");
      ypos += textsize + gap;
      ypos += textsize + gap;
      RenderUtils::drawString(horizMargin, ypos, textsize, "See the Command line parameters section of the wiki at bitfighter.org for more information.");
      ypos += textsize + gap;
      ypos += textsize + gap;

      mGL->glColor(Colors::red);
      RenderUtils::drawCenteredString(ypos, textsize, "Currently reading data and settings from:");
      ypos += textsize + gap + gap;

      FolderManager *folderManager = mGameSettings->getFolderManager();
      ypos = showFoldersBlock(folderManager, (F32)textsize, ypos, gap+2);
   }
   else if(mCurPage == 2)
   {
      S32 gap = 5;
      S32 textsize = 16;
      S32 smallText = 14;

      S32 ypos = vertMargin + 35;

      mGL->glColor(Colors::white);

      ypos += showNameDescrBlock(mGameSettings->getHostName(), mGameSettings->getHostDescr(), ypos, textsize, gap);

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, true, "Host Addr:", "%s", mGameSettings->getHostAddress().c_str());
      RenderUtils::drawCenteredStringPair2Colf(ypos, smallText, false, "Lvl Change PW:", "%s", mGameSettings->getLevelChangePassword() == "" ?
                                                                    "None - anyone can change" : mGameSettings->getLevelChangePassword().c_str());
      ypos += textsize + gap;

      
      RenderUtils::drawCenteredStringPair2Colf(ypos, smallText, false, "Admin PW:", "%s", mGameSettings->getAdminPassword() == "" ?
                                                                     "None - no one can get admin" : mGameSettings->getAdminPassword().c_str());
      ypos += textsize + gap;

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, "Server PW:", "%s", mGameSettings->getServerPassword() == "" ?
                                                                             "None needed to play" : mGameSettings->getServerPassword().c_str());

      ypos += textsize + gap;
      ypos += textsize + gap;

      S32 x = RenderUtils::getCenteredString2ColStartingPosf(textsize, false, "Max Players: %d", mGameSettings->getMaxPlayers());

      mGL->glColor(Colors::white);
      x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "Max Players: ");
      mGL->glColor(Colors::yellow);
      x += RenderUtils::drawStringAndGetWidthf(x, ypos, textsize, "%d", mGameSettings->getMaxPlayers());

      ypos += textsize + gap;

      GameConnection *conn = getGame()->getConnectionToServer();

      if(conn)
      {
         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, "Sim. Send Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     conn->getSimulatedSendLatency(), 
                                     conn->getSimulatedSendPacketLoss() * 100);

         ypos += textsize + gap;

         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, "Sim. Rcv. Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     conn->getSimulatedReceiveLatency(), 
                                     conn->getSimulatedReceivePacketLoss() * 100);
      }
      else     // No connection? Use settings in settings.
      {
         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, "Sim. Send Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     mGameSettings->getSimulatedLag(),
                                     mGameSettings->getSimulatedLoss() * 100);

         ypos += textsize + gap;
      }

      ypos += textsize + gap;
      

      // Dump out names of loaded levels...
      mGL->glColor(Colors::white);
      string allLevels = "Levels: ";

      if(!GameManager::getServerGame())
         allLevels += " >>> Level list won't be resolved until you start hosting <<<"; 
      else
         for(S32 i = 0; i < GameManager::getServerGame()->getLevelCount(); i++)
            allLevels += string(GameManager::getServerGame()->getLevelNameFromIndex(i).getString()) + "; ";

      U32 i, j, k;
      i = j = k = 0;
      
      for(j = 0; j < 4 && i < allLevels.length(); j++)
      {
         while(RenderUtils::getStringWidth(textsize - 6, allLevels.substr(k, i - k).c_str()) <
               DisplayManager::getScreenInfo()->getGameCanvasWidth() - 2 * horizMargin && i < allLevels.length()) 
         {
            i++;
         }

         RenderUtils::drawString(horizMargin, ypos, textsize - 6, allLevels.substr(k, i - k).c_str());
         k = i;
         ypos += textsize + gap - 5;
      }

      ypos += (textsize + gap) * (3 - j);


      // Temporary placeholder for badge testing -- centered at xpos, ypos, with radius of rad (i.e. 2 x rad across)
#ifdef TNL_DEBUG

      //drawDivetedTriangle(20,9);
      //drawGear(8,20,15,10,8, 5);

      for(S32 i = 0; i < 2; i++)
      {
         F32 x = (F32)horizMargin + 10;
         F32 y = 500.0f + 20.0f * i;

         F32 rad = 10;
         F32 smallSize = .6f;
            
         mGL->glPushMatrix();
         mGL->glScale(i ? smallSize : 1);
         y *= (i ? 1/smallSize : 1);

         
         F32 rm2 = rad - 2;
         F32 r3 = rad * .333f;
         F32 rm23 = rm2 * .333f;

         mGL->glColor(Colors::white);
         RenderUtils::drawPolygon(Point(x,y), 6, rm2, 0);
         mGL->glColor(Colors::red);
         RenderUtils::drawCircle(Point(x, y), rad);

         x += 3*rad;

         mGL->glColor(Colors::yellow);
         RenderUtils::drawPolygon(Point(x,y), 3, rm2, FloatTau/12);
         mGL->glColor(Colors::red);
         RenderUtils::drawCircle(Point(x, y), rad);

         x += 3*rad;
         mGL->glColor(Colors::green);
         RenderUtils::drawHollowRect(x - rad, y - r3,  x + rad, y + r3);
         RenderUtils::drawHollowRect(x - r3,  y - rad, x + r3,  y + rad);


         // Use rm2 and rm23 to make squares a little smaller to balance size of the circles

         x += 3*rad;
         mGL->glColor(Colors::red);
         RenderUtils::drawFilledRect(x - rm2, y - rm2, x + rm2, y + rm2);
         mGL->glColor(Colors::white);
         RenderUtils::drawFilledRect(x - rm23,  y - rm2, x + rm23, y - rm23);
         RenderUtils::drawFilledRect(x - rm23,  y + rm2, x + rm23, y + rm23);
         RenderUtils::drawFilledRect(x + rm2, y - rm23,  x + rm23, y + rm23);
         RenderUtils::drawFilledRect(x - rm2, y - rm23,  x - rm23, y + rm23);

         x += 3*rad;
         mGL->glColor(Colors::red);
         RenderUtils::drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2);
         RenderUtils::drawCircle(Point(x, y), rm2);
         mGL->glColor(Colors::orange67);
         RenderUtils::drawCircle(Point(x, y), rad / 2);

         x += 3*rad;
         mGL->glColor(Colors::red);
         RenderUtils::drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2);
         RenderUtils::drawCircle(Point(x, y), rm2);
         mGL->glColor(Colors::yellow);
         RenderUtils::drawFilledCircle(Point(x, y), rad / 2);
         mGL->glColor(Colors::orange67);
         RenderUtils::drawCircle(Point(x, y), rad / 2);


         x += 3*rad;
         mGL->glColor(Colors::red);
         RenderUtils::drawCircle(Point(x, y), rad);
         mGL->glColor(Colors::white);
         RenderUtils::drawCircle(Point(x, y), r3 * 2);
         mGL->glColor(Colors::red);
         RenderUtils::drawCircle(Point(x, y), r3);


         x += 3*rad;
         mGL->glColor(Colors::paleBlue);
         RenderUtils::drawPolygon(Point(x,y + r3), 3, rad * 1.2f, FloatTau/12);
         mGL->glColor(Colors::cyan);
         RenderUtils::drawPolygon(Point(x,y + r3), 3, rad * .6f, FloatTau/4);


         x += 3*rad;
         mGL->glColor(Colors::red);
         RenderUtils::drawCircle(Point(x, y), rad);
         mGL->glColor(Colors::white);
         RenderUtils::drawStar(Point(x,y), 7, rad - 1, rad/2);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, DEVELOPER_BADGE);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_TWENTY_FIVE_FLAGS);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_BBB_GOLD);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_BBB_SILVER);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_BBB_BRONZE);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_BBB_PARTICIPATION);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_LEVEL_DESIGN_WINNER);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_ZONE_CONTROLLER);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_RAGING_RABID_RABBIT);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_HAT_TRICK);

         x += 3*rad;
         GameObjectRender::renderBadge(x, y, rad, BADGE_LAST_SECOND_WIN);

         // Level design contest winner badge
         x += 3*rad;
         Vector<Point> points;
         points.push_back(Point(x - rm2, y - rm2));
         points.push_back(Point(x - rm2, y + rm2));
         points.push_back(Point(x + rm2, y + rm2));
         points.push_back(Point(x + rm2, y - rm2));
         GameObjectRender::renderWallFill(&points, Colors::wallFillColor, false);
         GameObjectRender::renderPolygonOutline(&points, Colors::blue);
         mGL->glColor(Colors::yellow);
         RenderUtils::drawStar(Point(x,y), 5, rad * .5f, rad * .25f);

         ///// After all badge rendering
         mGL->glPopMatrix();
      }
#endif // TNL_DEBUG
   }
}

};


