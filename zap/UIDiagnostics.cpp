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

#include <cmath>
#include "FontManager.h"


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
   string inputString = InputCodeManager::getCurrentInputString(inputCode);

   if(checkInputCode(BINDING_DIAG, inputString))
   {
      mCurPage++;
      if(mCurPage >= NUM_PAGES)
         quit();
   }
   else if(checkInputCode(BINDING_LOBBYCHAT, inputString))
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


S32 findLongestString(S32 size, const Vector<string> &strings)
{
   S32 maxLen = 0;
   S32 longest = 0;

   for(S32 i = 0; i < strings.size(); i++)
   {
      S32 len = RenderUtils::getStringWidth(size, strings[i]);
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

   longestName = findLongestString(textsize, names);
   longestVal  = findLongestString(textsize, vals);

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
      RenderUtils::drawString_fixed(xpos, ypos, (S32)textsize, Colors::cyan, names[i].c_str());
      xpos += nameWidth + spaceWidth;
      RenderUtils::drawString_fixed(xpos, ypos, (S32)textsize, Colors::white,vals[i].c_str());

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
   S32 x = RenderUtils::getCenteredStringStartingPosf(textsize, "M/C Ver: %d | C/S Ver: %d | Build: %s/%d | Date: %s | CPU: %s | OS: %s | Cmplr: %s",
           MASTER_PROTOCOL_VERSION, CS_PROTOCOL_VERSION, ZAP_GAME_RELEASE, BUILD_VERSION, TNL_CPU_STRING, TNL_OS_STRING, TNL_COMPILER_STRING, buildDate.c_str());

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white, "M/C Ver: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%d", MASTER_PROTOCOL_VERSION);

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | C/S Ver: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%d", CS_PROTOCOL_VERSION);

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | Build: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%d", BUILD_VERSION);
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "/");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", ZAP_GAME_RELEASE);

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | Date: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", buildDate.c_str());

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | CPU: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", TNL_CPU_STRING);

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | OS: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", TNL_OS_STRING);

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | Cmplr: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", TNL_COMPILER_STRING);

   return ypos + textsize + gap * 2;
}


S32 DiagnosticUserInterface::showNameDescrBlock(const string &hostName, const string &hostDescr, S32 ypos, S32 textsize, S32 gap)
{
   S32 x = RenderUtils::getCenteredStringStartingPosf(textsize, "Server Name: %s | Descr: %s", hostName.c_str(), hostDescr.c_str());

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  "Server Name: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", hostName.c_str());

   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::white,  " | Descr: ");
   x += RenderUtils::drawStringAndGetWidthf_fixed(x, ypos, textsize, Colors::yellow, "%s", hostDescr.c_str());

   return ypos + textsize + gap;
}


S32 DiagnosticUserInterface::showMasterBlock(ClientGame *game, S32 textsize, S32 ypos, S32 gap, bool leftcol)
{
   RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, leftcol, "Master Srvr Addr:", "%s",
                                              game->getSettings()->getMasterServerList()->size() > 0 ? 
                                                         game->getSettings()->getMasterServerList()->get(0).c_str() : "None");

   ypos += 2 * textsize + gap;

   if(game->getConnectionToMaster() && game->getConnectionToMaster()->isEstablished())
   {
      RenderUtils::drawCenteredString2Colf(ypos, textsize, Colors::MasterServerBlue, leftcol, "Connected to [%s]",
                                             game->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
      RenderUtils::drawCenteredString2Col(ypos, textsize, Colors::red, leftcol, "Not connected to Master Server");

   return ypos + textsize + gap;
}


string DiagnosticUserInterface::resolvePlaylist(GameSettings *gameSettings) const
{
   Game *game = getGame();    // Could this be NULL?
   TNLAssert(game, "No game?");

   if(game)
   {
      string playlist = game->getPlaylist();

      if(playlist.empty())
         playlist = "Not using a playlist";

      return playlist;
   }

   return gameSettings->isUsingPlaylist() ? gameSettings->getPlaylistFile() : "Not using a playlist";
}


void DiagnosticUserInterface::render() const
{
   // Draw title, subtitle, and footer
   RenderUtils::drawStringf_fixed(  3, 28, 25, Colors::red, "DIAGNOSTICS - %s", pageHeaders[mCurPage]);
   RenderUtils::drawStringf_fixed(625, 28, 25, Colors::red, "PAGE %d/%d", mCurPage + 1, NUM_PAGES);
 
   RenderUtils::drawCenteredStringf_fixed(591, 20, Colors::red, "%s - next page  ESC exits", getSpecialBindingString(BINDING_DIAG).c_str());

   RenderUtils::drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), 31,  Colors::gray70);
   RenderUtils::drawHorizLine(0, DisplayManager::getScreenInfo()->getGameCanvasWidth(), 569, Colors::gray70);

   S32 textsize = 14;

   if(mCurPage == 0)
   {
      string inputMode = mGameSettings->getInputCodeManager()->getInputModeString();

      RenderUtils::drawCenteredString_fixed(vertMargin + 55, 18, Colors::red, "Is something wrong?");

      S32 x, y;
      x = RenderUtils::getCenteredStringStartingPosf(textsize, "Can't control your ship? Check your input mode "
                                                    "(Options>Primary Input) [currently %s]", inputMode.c_str());
      y = vertMargin + 63 + textsize;
      x += RenderUtils::drawStringAndGetWidth_fixed(x, y, textsize, Colors::green, "Can't control your ship? Check your input mode (Options>Primary Input) [currently ");
      x += RenderUtils::drawStringAndGetWidth_fixed(x, y, textsize, Colors::red, inputMode.c_str());

      RenderUtils::drawString_fixed(x, y, textsize, Colors::green, "]");

      // Box around something wrong? block
      RenderUtils::drawHollowRect(horizMargin, vertMargin + 27, DisplayManager::getScreenInfo()->getGameCanvasWidth() - horizMargin, vertMargin + 90, Colors::cyan);

      const S32 gap = 5;

      S32 ypos = showVersionBlock(122, textsize - 2, gap) + textsize;

      textsize = 16;

      bool needToUpgrade = getUIManager()->getUI<MainMenuUserInterface>()->getNeedToUpgrade();

      RenderUtils::drawCenteredString2Colf(ypos, textsize, Colors::white, false, "%s", needToUpgrade ? "<<Update available>>" : "<<Current version>>");
      ypos += textsize + gap;

      ClientInfo *clientInfo = getGame()->getClientInfo();

      // This following line is a bit of a beast, but it will return a valid result at any stage of being in or out of a game.
      // If the server modifies a user name to make it unique, this will display the modified version.
      RenderUtils::drawCenteredStringPair2Colf(ypos - textsize, textsize, true, Colors::white, Colors::cyan, "Nickname:", "%s (%s)",
                                  clientInfo->getName().getString(), 
                                  clientInfo->isAuthenticated() ? 
                                       string("Verified - " + itos(clientInfo->getBadges())).c_str() : "Not verified");

      showMasterBlock(getGame(), textsize, ypos, gap, false);

      ypos += (textsize + gap) * 2;
      RenderUtils::drawCenteredStringPair2Colf(ypos - textsize, textsize, true, Colors::white, Colors::cyan, "Input Mode:", "%s", inputMode.c_str());
  
      ypos += textsize + gap;
      
      S32 index = GameSettings::UseControllerIndex;

      bool joystickDetected = GameSettings::DetectedControllerList.size() > 0;

      if(joystickDetected && getGame()->getInputMode() == InputModeKeyboard)
      {
         RenderUtils::drawCenteredString_fixed(414, textsize, Colors::cyan, "Joystick not enabled, you may set input mode to Joystick in option menu.");
         joystickDetected = false;
      }
      else if(!joystickDetected)
         RenderUtils::drawCenteredString2Col(ypos, textsize, Colors::cyan, true, "No joysticks detected");
      else
      {
         RenderUtils::drawCenteredString2Col(ypos + textsize + gap, textsize, Colors::magenta, true, "Autodetect String:");

         const char *controllerName = "<None>";
         if(GameSettings::UseControllerIndex >= 0)
            controllerName = GameSettings::DetectedControllerList[index].c_str();

         RenderUtils::drawCenteredString2Col(ypos + 2 * (textsize + gap), textsize, Colors::cyan, true, controllerName);
      }

      ypos += 6 * (textsize + gap);

      if(joystickDetected)
      {
         S32 x = 500;
         S32 y = 300;

         RenderUtils::drawString_fixed(x, y, textsize - 2, Colors::white, "Raw Analog Axis Values:");

         y += 25;

         for(S32 i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
         {
            F32 a = (F32)Joystick::rawAxesValues[i] / (F32)S16_MAX;    // Range: -1 to 1
            if(fabs(a) > .1f)
            {
               S32 len = RenderUtils::drawStringAndGetWidthf_fixed(x, y, textsize - 2, Colors::cyan, "Axis %d", i);

               RenderUtils::drawHorizLine(x, x + len, y + 5, Colors::red);
               RenderUtils::drawHorizLine(x + len / 2, x + len / 2 + S32(a * F32(len / 2)), y + 5, Colors::yellow);

               x += len + 8;
            }
         }
      }

      // Key states
      S32 hpos = horizMargin + textsize;

      hpos += RenderUtils::drawStringAndGetWidth_fixed(hpos, ypos, textsize, Colors::yellow, "Keys down: ");

      for(U32 i = 0; i < MAX_INPUT_CODES; i++)
      {
         InputCode inputCode = InputCode(i);
         if(InputCodeManager::getState(inputCode))
         {
            SymbolKey key = SymbolKey(InputCodeManager::inputCodeToString(inputCode), Colors::red);
            key.render(hpos, ypos, AlignmentLeft);
            hpos += key.getWidth() + 5;
         }
      }


      // Render input strings
      hpos += RenderUtils::drawStringAndGetWidth_fixed(hpos, ypos, textsize, Colors::cyan, " | ");
      hpos += RenderUtils::drawStringAndGetWidth_fixed(hpos, ypos, textsize, Colors::yellow, "Input string: ");

      InputCode inputCode;
      for(U32 i = 0; i < MAX_INPUT_CODES; i++)
      {
         inputCode = InputCode(i);
         if(InputCodeManager::getState(inputCode))
            break;
      }

      string in = InputCodeManager::getCurrentInputString(inputCode);

      if(in != "")
      {
         SymbolShapePtr key = SymbolString::getModifiedKeySymbol(in, Colors::magenta);

         key->render(hpos, ypos, UI::AlignmentLeft);
         hpos += key->getWidth() + 5;
      }


      if(joystickDetected)
      {
         ypos += textsize + gap + 10;

         RenderUtils::drawCenteredString_fixed(ypos, textsize, Colors::green, "Hint: If you're having joystick problems, check your controller's 'mode' button.");

         //////////
         // Draw DPad and controller axis
         hpos = 60;
         ypos = DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - 110;

         JoystickRender::renderDPad(Point(hpos, ypos),
               InputCodeManager::getState(BUTTON_DPAD_UP),   InputCodeManager::getState(BUTTON_DPAD_DOWN),
               InputCodeManager::getState(BUTTON_DPAD_LEFT), InputCodeManager::getState(BUTTON_DPAD_RIGHT), 
               "DPad", "(Menu Nav)");
         hpos += 65;

         JoystickRender::renderDPad(Point(hpos, ypos),
               InputCodeManager::getState(STICK_1_UP),   InputCodeManager::getState(STICK_1_DOWN),
               InputCodeManager::getState(STICK_1_LEFT), InputCodeManager::getState(STICK_1_RIGHT), 
               "L Stick", "(Move)");
         hpos += 65;

         JoystickRender::renderDPad(Point(hpos, ypos),
               InputCodeManager::getState(STICK_2_UP),   InputCodeManager::getState(STICK_2_DOWN),
               InputCodeManager::getState(STICK_2_LEFT), InputCodeManager::getState(STICK_2_RIGHT), 
               "R Stick", "(Fire)");
         hpos += 55;

         // Render controller buttons
         for(U32 i = FIRST_CONTROLLER_BUTTON; i <= LAST_CONTROLLER_BUTTON; i++)
         {
            InputCode code = InputCode(i);
            const Color &color = InputCodeManager::getState(code) ? Colors::red : Colors::white;

            // renderControllerButton() returns false if nothing is rendered
            if(JoystickRender::renderControllerButton((F32)hpos, (F32)ypos, code, &color))
               hpos += 33;
         }


         // Render buttons parsed as symbol strings (is this needed?)
         Vector<SymbolShapePtr> symbols;
         symbols.push_back(SymbolShapePtr(new SymbolString("SymbolStrings:   ", NULL, HelpContext, 16, Colors::yellow, false)));

         S32 buttonCount = LAST_CONTROLLER_BUTTON - FIRST_CONTROLLER_BUTTON + 1;
         for(S32 i = 0; i < buttonCount; i++)
         {
            symbols.push_back(SymbolString::getControlSymbol(InputCode(i + FIRST_CONTROLLER_BUTTON), Colors::white));
            if(i < buttonCount - 1)
               symbols.push_back(SymbolString::getBlankSymbol(8));      // Provide a little breathing room
         }

         SymbolString(symbols).render(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2 + 60, ypos + 50));
      }
   }
   else if(mCurPage == 1)
   {
      S32 ypos = vertMargin + 50;
      S32 textsize = 15;
      S32 gap = 5;

      FontManager::setFontColor(Colors::gray70);

      RenderUtils::drawString_fixed(horizMargin, ypos, textsize, "Folders are either an absolute path or a path relative to the program execution folder");
      ypos += textsize + gap;
      RenderUtils::drawString_fixed(horizMargin, ypos, textsize, "or local folder, depending on OS.  If an entry is blank, Bitfighter will look for files");
      ypos += textsize + gap;
      RenderUtils::drawString_fixed(horizMargin, ypos, textsize, "in the program folder or local folder, depending on OS.");
      ypos += textsize * 2 + gap;
      RenderUtils::drawString_fixed(horizMargin, ypos, textsize, "See the Command line parameters section of the wiki at bitfighter.org for more information.");
      ypos += (textsize + gap) * 2;

      RenderUtils::drawCenteredString_fixed(ypos, textsize, Colors::red, "Currently reading data and settings from:");
      ypos += textsize + gap + gap;

      FolderManager *folderManager = mGameSettings->getFolderManager();
      ypos = showFoldersBlock(folderManager, (F32)textsize, ypos, gap+2);
   }
   else if(mCurPage == 2)
   {
      S32 gap = 5;
      S32 textsize = 16;
      S32 smallText = 14;

      S32 ypos = vertMargin + 19;

      ypos += showNameDescrBlock(mGameSettings->getHostName(), mGameSettings->getHostDescr(), ypos + textsize, textsize, gap);

      //ypos += textsize;

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize,                           true,  Colors::white, Colors::cyan, 
                                               "Host Addr:", "%s", mGameSettings->getHostAddress().c_str());
      RenderUtils::drawCenteredStringPair2Colf(ypos, smallText - (textsize - smallText), false, Colors::white, Colors::cyan,
                                               "Lvl Change PW:", "%s", mGameSettings->getLevelChangePassword() == "" ?
                                                                    "None - anyone can change" : mGameSettings->getLevelChangePassword().c_str());
      ypos += textsize + gap;

      
      RenderUtils::drawCenteredStringPair2Colf(ypos, smallText, false, Colors::white, Colors::cyan, "Admin PW:", "%s", mGameSettings->getAdminPassword() == "" ?
                                                                     "None - no one can get admin" : mGameSettings->getAdminPassword().c_str());
      ypos += textsize + gap;

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, Colors::white, Colors::cyan, "Server PW:", "%s", mGameSettings->getServerPassword() == "" ?
                                                                             "None needed to play" : mGameSettings->getServerPassword().c_str());

      ypos += textsize + gap;
      ypos += textsize + gap;

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, Colors::white, Colors::cyan, "Max Players:", "%d", mGameSettings->getMaxPlayers());

      ypos += textsize + gap;

      GameConnection *conn = getGame()->getConnectionToServer();

      RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, true, Colors::white, Colors::cyan, "Playlist File:", "%s", resolvePlaylist(mGameSettings).c_str());
      
      if(conn)
      {
         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, Colors::white, Colors::cyan, "Sim. Send Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     conn->getSimulatedSendLatency(), 
                                     conn->getSimulatedSendPacketLoss() * 100);

         ypos += textsize + gap;

         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, Colors::white, Colors::cyan, "Sim. Rcv. Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     conn->getSimulatedReceiveLatency(), 
                                     conn->getSimulatedReceivePacketLoss() * 100);
      }
      else     // No connection? Use settings in settings.
      {
         RenderUtils::drawCenteredStringPair2Colf(ypos, textsize, false, Colors::white, Colors::cyan, "Sim. Send Lag/Pkt. Loss:", "%dms/%2.0f%%",
                                     mGameSettings->getSimulatedLag(),
                                     mGameSettings->getSimulatedLoss() * 100);
         ypos += textsize + gap;
      }

      ypos += textsize + gap;
      
      // Dump out names of loaded levels...
      string allLevels = "Levels: ";

      if(!GameManager::getServerGame())
         allLevels += " >>> Level list won't be resolved until you start hosting <<<"; 
      else
         for(S32 i = 0; i < GameManager::getServerGame()->getLevelCount(); i++)
            allLevels += string(GameManager::getServerGame()->getLevelNameFromIndex(i).getString()) + "; ";

      U32 i, j, k;
      i = j = k = 0;
      
      ypos += textsize - 6;
      for(j = 0; j < 4 && i < allLevels.length(); j++)
      {
         while(RenderUtils::getStringWidth(textsize - 6, allLevels.substr(k, i - k).c_str()) <
               DisplayManager::getScreenInfo()->getGameCanvasWidth() - 2 * horizMargin && i < allLevels.length()) 
         {
            i++;
         }

         RenderUtils::drawString_fixed(horizMargin, ypos, textsize - 6, Colors::white, allLevels.substr(k, i - k).c_str());
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

         RenderUtils::drawPolygon(Point(x, y), 6, rm2, 0, Colors::white);
         RenderUtils::drawCircle(Point(x, y), rad, Colors::red);

         x += 3 * rad;

         RenderUtils::drawPolygon(Point(x, y), 3, rm2, FloatTau / 12, Colors::yellow);
         RenderUtils::drawCircle(Point(x, y), rad, Colors::red);

         x += 3 * rad;
         RenderUtils::drawHollowRect(x - rad, y - r3,  x + rad, y + r3,  Colors::green);
         RenderUtils::drawHollowRect(x - r3,  y - rad, x + r3,  y + rad, Colors::green);

         // Use rm2 and rm23 to make squares a little smaller to balance size of the circles
         x += 3 * rad;
         RenderUtils::drawFilledRect(x - rm2, y - rm2, x + rm2, y + rm2, Colors::red);
         RenderUtils::drawFilledRect(x - rm23, y - rm2,  x + rm23, y - rm23, Colors::white);
         RenderUtils::drawFilledRect(x - rm23, y + rm2,  x + rm23, y + rm23, Colors::white);
         RenderUtils::drawFilledRect(x + rm2,  y - rm23, x + rm23, y + rm23, Colors::white);
         RenderUtils::drawFilledRect(x - rm2,  y - rm23, x - rm23, y + rm23, Colors::white);

         x += 3 * rad;
         RenderUtils::drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2, Colors::red);
         RenderUtils::drawCircle(Point(x, y), rm2,     Colors::red);
         RenderUtils::drawCircle(Point(x, y), rad / 2, Colors::orange67);

         x += 3 * rad;
         RenderUtils::drawHollowRect(x - rm2, y - rm2, x + rm2, y + rm2, Colors::red);
         RenderUtils::drawCircle(      Point(x, y), rm2, Colors::red);
         RenderUtils::drawFilledCircle(Point(x, y), rad / 2, Colors::yellow);
         RenderUtils::drawCircle(      Point(x, y), rad / 2, Colors::orange67);


         x += 3*rad;
         RenderUtils::drawCircle(Point(x, y), rad,    Colors::red);
         RenderUtils::drawCircle(Point(x, y), r3 * 2, Colors::white);
         RenderUtils::drawCircle(Point(x, y), r3,     Colors::red);


         x += 3*rad;
         RenderUtils::drawPolygon(Point(x, y + r3), 3, rad * 1.2f, FloatTau / 12, Colors::paleBlue);
         RenderUtils::drawPolygon(Point(x, y + r3), 3, rad * .6f, FloatTau / 4, Colors::cyan);


         x += 3*rad;
         RenderUtils::drawCircle(Point(x, y), rad, Colors::red);
         RenderUtils::drawStar(Point(x,y), 7, rad - 1, rad/2, Colors::white);

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
         RenderUtils::drawStar(Point(x, y), 5, rad * .5f, rad * .25f, Colors::yellow);

         ///// After all badge rendering
         mGL->glPopMatrix();
      }
#endif // TNL_DEBUG
   }
}

};


