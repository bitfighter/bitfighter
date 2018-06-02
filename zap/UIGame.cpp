//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Use this for testing the scoreboard
//#define USE_DUMMY_PLAYER_SCORES

#include "UIGame.h"

#include "UIChat.h"
#include "UIInstructions.h"
#include "UIManager.h"
#include "UIMenus.h"

#include "barrier.h"
#include "BotNavMeshZone.h"
#include "ChatMessageDisplayer.h"
#include "ClientGame.h"
#include "Colors.h"
#include "Console.h"             // Our console object
#include "Cursor.h"
#include "DisplayManager.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "FontManager.h"
#include "GameManager.h"
#include "GameObjectRender.h"
#include "GameRecorderPlayback.h"
#include "gameType.h"
#include "GaugeRenderer.h"
#include "Intervals.h"
#include "projectile.h"          // For SpyBug
#include "ScoreboardRenderer.h"
#include "ServerGame.h"
#include "shipItems.h"           // For EngineerBuildObjects
#include "SoundSystem.h"
#include "robot.h"              
#include "voiceCodec.h"

// Resharper says these includes are not needed... but they are
#include "Level.h"               

#include "stringUtils.h"
#include "RenderUtils.h"
#include "GeomUtils.h"

#include <cmath>     // Needed to compile under Linux, OSX

namespace Zap
{

// Sizes and other things to help with positioning
static const S32 SRV_MSG_FONT_SIZE = 14;
static const S32 SRV_MSG_FONT_GAP = 4;
static const S32 CHAT_FONT_SIZE = 12;
static const S32 CHAT_FONT_GAP = 3;
static const S32 CHAT_WRAP_WIDTH = 700;            // Max width of chat messages displayed in-game
static const S32 SRV_MSG_WRAP_WIDTH = 750;


// Constructor
GameUserInterface::GameUserInterface(ClientGame *game, UIManager *uiManager) :
                  Parent(game, uiManager), 
                  mVoiceRecorder(game),  //    lines  topdown    wrap width          font size          line gap
                  mServerMessageDisplayer(game,  6,   true,   SRV_MSG_WRAP_WIDTH,  SRV_MSG_FONT_SIZE, SRV_MSG_FONT_GAP),
                  mChatMessageDisplayer  (game,  5,   false,  CHAT_WRAP_WIDTH,     CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mFpsRenderer(game),
                  mLevelInfoDisplayer(game),
                  mHelpItemManager(mGameSettings)
{
   mInScoreboardMode = false;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;

   mHelperManager.initialize(game);

   // Some debugging settings
   mDebugShowShipCoords   = false;
   mDebugShowObjectIds    = false;
   mShowDebugBots         = false;
   mDebugShowMeshZones    = false;

   mShrinkDelayTimer.setPeriod(500);

   mGotControlUpdate = false;
   
   mFiring = false;

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
   {
      mModPrimaryActivated[i] = false;
      mModSecondaryActivated[i] = false;
      mModuleDoubleTapTimer[i].setPeriod(DoubleClickTimeout);
   }
   
   mAnnouncementTimer.setPeriod(FIFTEEN_SECONDS);  
   mAnnouncement = "";

   mShowProgressBar = false;
   mProgressBarFadeTimer.setPeriod(ONE_SECOND);

   // Transition time between regular map and commander's map; in ms, higher = slower
   mCommanderZoomDelta.setPeriod(350);
   mInCommanderMap = false;

   prepareStars();
}


// Destructor  -- only runs when we're exiting to the OS
GameUserInterface::~GameUserInterface()
{
   // Do nothing
}


void GameUserInterface::onPlayerJoined()     { mHelperManager.onPlayerJoined();     }
void GameUserInterface::onPlayerQuit()       { mHelperManager.onPlayerQuit();       }
void GameUserInterface::quitEngineerHelper() { mHelperManager.quitEngineerHelper(); }  // When ship dies engineering
void GameUserInterface::exitHelper()         { mHelperManager.exitHelper();         }


void GameUserInterface::onGameOver()         
{ 
   mHelperManager.onGameOver();         
}


// This event gets run after the scoreboard display is finished
void GameUserInterface::onGameReallyAndTrulyOver()         
{ 
   mFxManager.onGameReallyAndTrulyOver();
   mHelperManager.onGameOver();         
}


void GameUserInterface::setAnnouncement(const string &message)
{
   mAnnouncement = message;
   mAnnouncementTimer.reset();
}


void GameUserInterface::onActivate()
{
   mDisableShipKeyboardInput = false;  // Make sure our ship controls are active
   mMissionOverlayActive = false;      // Turn off the mission overlay (if it was on)
   Cursor::disableCursor();            // Turn off cursor
   onMouseMoved();                     // Make sure ship pointed is towards mouse
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;

   clearDisplayers();

   // Clear out any walls we were using in a previous run
   Barrier::clearRenderItems();           // TODO: Should really go in an onDeactivate method, which we don't really have
   mLevelInfoDisplayer.clearDisplayTimer();

   mLoadoutIndicator.reset();
   mShowProgressBar = true;               // Causes screen to be black before level is loaded

   mHelperManager.reset();

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i]   = false;
      mModSecondaryActivated[i] = false;
   }

   mShutdownMode = None;

   getGame()->onGameUIActivated();
}


void GameUserInterface::addStartingHelpItemsToQueue()
{
   // Queue up some initial help messages for the new users
   mHelpItemManager.reset();
   mHelpItemManager.addInlineHelpItem(WelcomeItem);            // Hello, my name is Clippy!     

   if(getGame()->getInputMode() == InputModeKeyboard)          // Show help related to basic movement and shooting
      mHelpItemManager.addInlineHelpItem(ControlsKBItem);
   else
      mHelpItemManager.addInlineHelpItem(ControlsJSItem);

   mHelpItemManager.addInlineHelpItem(ModulesAndWeaponsItem);  // Point out loadout indicators

   mHelpItemManager.addInlineHelpItem(ControlsModulesItem);    // Show how to activate modules
   mHelpItemManager.addInlineHelpItem(ChangeWeaponsItem);      // Explain how to toggle weapons
   mHelpItemManager.addInlineHelpItem(CmdrsMapItem);           // Suggest viewing cmdrs map
   mHelpItemManager.addInlineHelpItem(ChangeConfigItem);       // Changing loadouts
   mHelpItemManager.addInlineHelpItem(GameModesItem);          // Use F2 to see current mission
   mHelpItemManager.addInlineHelpItem(GameTypeAndTimer);       // Point out clock and score in LR
   mHelpItemManager.addInlineHelpItem(EnergyGaugeItem);        // Show user the energy gauge
   mHelpItemManager.addInlineHelpItem(ViewScoreboardItem);     // Show ow to get the score
   mHelpItemManager.addInlineHelpItem(TryCloakItem);           // Recommend cloaking
   mHelpItemManager.addInlineHelpItem(TryTurboItem);           // Recommend turbo

   // And finally...
   mHelpItemManager.addInlineHelpItem(F1HelpItem);             // How to get Help
   
   if(getGame()->getBotCount() == 0)
      mHelpItemManager.addInlineHelpItem(AddBotsItem);         // Add some bots?
}


void GameUserInterface::onReactivate()
{
   mDisableShipKeyboardInput = false;
   Cursor::disableCursor();    // Turn off cursor

   if(!isChattingOrTypingCommand())
      getGame()->setBusyChatting(false);

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i]   = false;
      mModSecondaryActivated[i] = false;
   }

   onMouseMoved();   // Call onMouseMoved to get ship pointed at current cursor location
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;
}


// Called when level just beginning (called from GameConnection::onStartGhosting)
// We probably don't have a GameType yet, so we don't know what our level name will be
void GameUserInterface::onGameStarting()
{
   mDispWorldExtents.set(Point(0,0), 0);
   Barrier::clearRenderItems();

   addStartingHelpItemsToQueue();      // Do this here so if the helpItem manager gets turned on, items will start displaying next game

   mHelpItemManager.onGameStarting();
}


static char stringBuffer[256];

void GameUserInterface::displayErrorMessage(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);

   displayMessage(Colors::cmdChatColor, stringBuffer);
}


void GameUserInterface::onGameTypeChanged()
{
   mLevelInfoDisplayer.onGameTypeChanged();     // Tell mLevelInfoDisplayer there is a new GameType in town
}


void GameUserInterface::displaySuccessMessage(const char *format, ...)
{
   va_list args;

   va_start(args, format);
   vsnprintf(stringBuffer, sizeof(stringBuffer), format, args);
   va_end(args);

   displayMessage(Color(0.6, 1, 0.8), stringBuffer);
}


void GameUserInterface::displayMessage(const Color &msgColor, const char *message)
{
   // Ignore empty message
   if(strcmp(message, "") == 0)
      return;

   mServerMessageDisplayer.onChatMessageReceived(msgColor, message);
}


bool GameUserInterface::isShowingMissionOverlay() const
{
   return mMissionOverlayActive;
}


void GameUserInterface::startLoadingLevel(bool engineerEnabled)
{
   mShowProgressBar = true;             // Show progress bar

   resetLevelInfoDisplayTimer();        // Start displaying the level info, now that we have it
   pregameSetup(engineerEnabled);       // Now we know all we need to initialize our loadout options
}


void GameUserInterface::doneLoadingLevel()
{
   mShowProgressBar = false;
   mProgressBarFadeTimer.reset();
   mDispWorldExtents.set(getGame()->getWorldExtents());
}


// Limit shrinkage of extent window to reduce jerky effect of some distant object disappearing from view
static F32 rectify(F32 actual, F32 disp, bool isMax, bool waiting, bool loading, U32 timeDelta, Timer &shrinkDelayTimer)
{
   const F32 ShrinkRate = 2.0f;     // Pixels per ms

   F32 delta = actual - disp;

   // When loading or really close to actual, just return the actual extent
   if(fabs(delta) < 0.1 || loading)
      return actual;

   // If the display needs to grow, we do that without delay
   if((delta < 0 && !isMax) || (delta > 0 && isMax))
   {
      shrinkDelayTimer.reset();
      return actual;
   }

   // So if we are here, the actual extents are smaller than the display, and we need to contract.

   // We have a timer that gives us a little breathing room before we start contracting.  If waiting is true, no contraction.
   if(waiting)
      return disp;
   
   // If the extents are close to the display, snap to the extents, to avoid overshooting
   if(fabs(disp - actual) <= ShrinkRate * timeDelta)
      return actual;

   // Finally, contract display extents by our ShrinkRate
   return disp + (delta > 0 ? 1 : -1) * ShrinkRate * timeDelta;
}


// Limit shrinkage of extent window to reduce jerky effect of some distant object disappearing from view
void GameUserInterface::rectifyExtents(U32 timeDelta)
{
   const Rect *worldExtentRect = getGame()->getWorldExtents();

   mShrinkDelayTimer.update(timeDelta);

   bool waiting = mShrinkDelayTimer.getCurrent() > 0;

   mDispWorldExtents.max.x = rectify(worldExtentRect->max.x, mDispWorldExtents.max.x, true,  waiting, mShowProgressBar, timeDelta, mShrinkDelayTimer);
   mDispWorldExtents.max.y = rectify(worldExtentRect->max.y, mDispWorldExtents.max.y, true,  waiting, mShowProgressBar, timeDelta, mShrinkDelayTimer);
   mDispWorldExtents.min.x = rectify(worldExtentRect->min.x, mDispWorldExtents.min.x, false, waiting, mShowProgressBar, timeDelta, mShrinkDelayTimer);
   mDispWorldExtents.min.y = rectify(worldExtentRect->min.y, mDispWorldExtents.min.y, false, waiting, mShowProgressBar, timeDelta, mShrinkDelayTimer);
}


void GameUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   // Update some timers
   mShutdownTimer.update(timeDelta);
   mWrongModeMsgDisplay.update(timeDelta);
   mProgressBarFadeTimer.update(timeDelta);
   mCommanderZoomDelta.update(timeDelta);
   mLevelInfoDisplayer.idle(timeDelta);

   if(shouldRenderLevelInfo())
      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   else
      mInputModeChangeAlertDisplayTimer.update(timeDelta);

   if(mAnnouncementTimer.update(timeDelta))
      mAnnouncement = "";

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModuleDoubleTapTimer[i].update(timeDelta);

   // Messages
   bool chatting = isChatting();
   mServerMessageDisplayer.idle(timeDelta, chatting);
   mChatMessageDisplayer.idle(timeDelta, chatting);

   mFpsRenderer.idle(timeDelta);
   mConnectionStatsRenderer.idle(timeDelta, getGame()->getConnectionToServer());
   
   mHelperManager.idle(timeDelta);
   mVoiceRecorder.idle(timeDelta);
   mLevelListDisplayer.idle(timeDelta);

   mLoadoutIndicator.idle(timeDelta);

   // Processes sparks and teleporter effects -- 
   //    Do this even while suspended to make objects look normal while in /idling
   //    But not while playing back game recordings, idled in idleFxManager with custom timeDelta
   if(dynamic_cast<GameRecorderPlayback *>(getGame()->getConnectionToServer()) == NULL)
      mFxManager.idle(timeDelta);

   if(shouldCountdownHelpItemTimer())
      mHelpItemManager.idle(timeDelta, getGame());

   // Keep ship pointed towards mouse cmdrs map zoom transition
   if(mCommanderZoomDelta.getCurrent() > 0)               
      onMouseMoved();

   if(renderWithCommanderMap())
      rectifyExtents(timeDelta);
}


// Returns true if we can show an inline help item
bool GameUserInterface::shouldCountdownHelpItemTimer() const
{
   return getGame()->getClientInfo()->getShowLevelUpMessage() == NONE &&   // Levelup message not being shown
          !getGame()->isSpawnDelayed() &&                                  // No spawn-delay stuff going on
          getUIManager()->getCurrentUI() == this &&                        // No other UI being drawn on top
          !shouldRenderLevelInfo() &&                                      // F2 levelinfo is not displayed...
          !scoreboardIsVisible() &&                                        // Hide help when scoreboard is visible
          !mHelperManager.isHelperActive();                                // Disable help helpers are active
}


void GameUserInterface::resetInputModeChangeAlertDisplayTimer(U32 timeInMs)
{
   mInputModeChangeAlertDisplayTimer.reset(timeInMs);
}


#ifdef TNL_OS_WIN32
   extern void checkMousePos(S32 maxdx, S32 maxdy);
#endif


void GameUserInterface::toggleShowingShipCoords() { mDebugShowShipCoords = !mDebugShowShipCoords; }
void GameUserInterface::toggleShowingObjectIds()  { mDebugShowObjectIds  = !mDebugShowObjectIds;  }
void GameUserInterface::toggleShowingMeshZones()  { mDebugShowMeshZones  = !mDebugShowMeshZones;  }
void GameUserInterface::toggleShowDebugBots()     { mShowDebugBots       = !mShowDebugBots;       }


bool GameUserInterface::isShowingDebugShipCoords() const { return mDebugShowShipCoords; }


// Some FxManager passthrough functions
void GameUserInterface::clearSparks()
{
   mFxManager.clearSparks();
}


// Some FxManager passthrough functions
void GameUserInterface::clearDisplayers()
{
   // Clear out any lingering server or chat messages
   mServerMessageDisplayer.reset();
   mChatMessageDisplayer.reset();

   mConnectionStatsRenderer.reset();
}


// Only runs when playing back a saved game... why?
// Allows FxManager to pause or run in slow motion with custom timeDelta
void GameUserInterface::idleFxManager(U32 timeDelta)
{
   mFxManager.idle(timeDelta);
}


F32 GameUserInterface::getCommanderZoomFraction() const
{
   return mInCommanderMap ? 1 - mCommanderZoomDelta.getFraction() : mCommanderZoomDelta.getFraction();
}


// Make sure we are not in commander's map when connection to game server is established
void GameUserInterface::resetCommandersMap()
{
   mInCommanderMap = false;
   mCommanderZoomDelta.clear();
}


void GameUserInterface::emitBlast(const Point &pos, U32 size)
{
   mFxManager.emitBlast(pos, size);
}


void GameUserInterface::emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2)
{
   mFxManager.emitBurst(pos, scale, color1, color2);
}


void GameUserInterface::emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation)
{
   mFxManager.emitDebrisChunk(points, color, pos, vel, ttl, angle, rotation);
}


void GameUserInterface::emitTextEffect(const string &text, const Color &color, const Point &pos, bool relative)
{
   mFxManager.emitTextEffect(text, color, pos, relative);
}


void GameUserInterface::emitDelayedTextEffect(U32 delay, const string &text, const Color &color, const Point &pos, bool relative)
{
   mFxManager.emitDelayedTextEffect(delay, text, color, pos, relative);
}

void GameUserInterface::emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType)
{
   mFxManager.emitSpark(pos, vel, color, ttl, sparkType);
}


void GameUserInterface::emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors)
{
   mFxManager.emitExplosion(pos, size, colorArray, numColors);
}


void GameUserInterface::emitTeleportInEffect(const Point &pos, U32 type)
{
   mFxManager.emitTeleportInEffect(pos, type);
}


// Draw main game screen (client only)
void GameUserInterface::render() const
{
   if(!getGame()->isConnectedToServer())
   {
      static const SymbolString connecting("Connecting to server...", NULL, ErrorMsgContext, 30, Colors::white, false, AlignmentCenter);
      connecting.render(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, 290));

      if(getGame()->getConnectionToServer())
      {
         SymbolString stat(GameConnection::getConnectionStateString(getGame()->getConnectionToServer()->getConnectionState()), 
                            NULL, ErrorMsgContext, 16, Colors::green, false, AlignmentCenter);
         stat.render(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, 326));
      }

      static const SymbolString pressEsc("Press [[ESC]] to abort", NULL, ErrorMsgContext, 20, Colors::white, false, AlignmentCenter);
      pressEsc.render(Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, 366));

      return;
   }

   if(renderWithCommanderMap())
      renderGameCommander();
   else
      renderGameNormal();

   S32 level = NONE;
   //if(getGame()->getLocalRemoteClientInfo())    // Can happen when starting new level before all packets have arrived from server
      level = getGame()->getClientInfo()->getShowLevelUpMessage();

   if(level != NONE)
      renderLevelUpMessage(level);
   else if(getGame()->isSpawnDelayed())
      renderSuspendedMessage();
   
   // Fade inlineHelpItem in and out as chat widget appears or F2 levelInfo appears.
   // Don't completely hide help item when chatting -- it's jarring.  
   F32 helpItemAlpha = getBackgroundTextDimFactor(false);
   mHelpItemManager.renderMessages(getGame(), DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2.0f + 40, helpItemAlpha);

   if(dynamic_cast<GameRecorderPlayback *>(getGame()->getConnectionToServer()) == NULL)
      renderReticle();                    // Draw crosshairs if using mouse
   renderWrongModeIndicator();            // Try to avert confusion after player has changed btwn joystick and keyboard modes
   renderChatMsgs();                      // Render incoming chat and server msgs
   mLoadoutIndicator.render(getGame());   // Draw indicators for the various loadout items

   renderLevelListDisplayer();            // List of levels loaded while hosting
   renderProgressBar();                   // Status bar that shows progress of loading this level
   mVoiceRecorder.render();               // Indicator that someone is sending a voice msg

   mHelperManager.render();
   renderLostConnectionMessage();      // Renders message overlay if we're losing our connection to the server

   mFpsRenderer.render(DisplayManager::getScreenInfo()->getGameCanvasWidth());     // Display running average FPS
   mConnectionStatsRenderer.render(getGame()->getConnectionToServer());    

   GameType *gameType = getGame()->getGameType();

   if(gameType)
      gameType->renderInterfaceOverlay(DisplayManager::getScreenInfo()->getGameCanvasWidth(), 
                                       DisplayManager::getScreenInfo()->getGameCanvasHeight());
   renderLevelInfo();
   
   renderShutdownMessage();

   renderConsole();  // Rendered last, so it's always on top

#if 0
// Some code for outputting the position of the ship for finding good spawns
GameConnection *con = getGame()->getConnectionToServer();

if(con)
{
   BfObject *co = con->getControlObject();
   if(co)
   {
      Point pos = co->getActualPos() * F32(1 / 300.0f);
      RenderUtils::drawStringf(10, 550, 30, "%0.2g, %0.2g", pos.x, pos.y);
   }
}

if(mGotControlUpdate)
   RenderUtils::drawString(710, 10, 30, "CU");
#endif
}


void GameUserInterface::addInlineHelpItem(HelpItem item)
{
   mHelpItemManager.addInlineHelpItem(item);
}


void GameUserInterface::addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam)
{
   mHelpItemManager.addInlineHelpItem(objectType, objectTeam, playerTeam);
}


void GameUserInterface::removeInlineHelpItem(HelpItem item, bool markAsSeen)
{
   mHelpItemManager.removeInlineHelpItem(item, markAsSeen);
}


F32 GameUserInterface::getObjectiveArrowHighlightAlpha() const
{
   return mHelpItemManager.getObjectiveArrowHighlightAlpha();
}


void GameUserInterface::setShowingInGameHelp(bool showing)
{
   if(showing != mHelpItemManager.isEnabled())
      mHelpItemManager.setEnabled(showing);       // Tell the HelpItemManager that its enabled status has changed
}


bool GameUserInterface::isShowingInGameHelp() const
{
   return mHelpItemManager.isEnabled();
}


void GameUserInterface::resetInGameHelpMessages()
{
   mHelpItemManager.resetInGameHelpMessages();
}


// Returns true if player is composing a chat message
bool GameUserInterface::isChatting() const
{
   return mHelperManager.isComposingPlayerChat();
}


bool GameUserInterface::isChattingOrTypingCommand() const
{
   return mHelperManager.isHelperActive(HelperMenu::ChatHelperType);    // Used for entering chats or commands
}


void GameUserInterface::renderSuspendedMessage() const
{
   if(getGame()->inReturnToGameCountdown())
   {
      static string waitMsg[] = { "", "WILL RESPAWN", "IN BLAH BLAH SECONDS", "" };
      waitMsg[2] = "IN " + ftos(ceil(F32(getGame()->getReturnToGameDelay()) * MS_TO_SECONDS)) + " SECONDS";
      renderMsgBox(waitMsg,  ARRAYSIZE(waitMsg), Colors::white);
   }
   else
   {
      static string readyMsg[] = { "", "PRESS ANY", "KEY TO", "RESPAWN", "" };
      renderMsgBox(readyMsg, ARRAYSIZE(readyMsg), Colors::white);
   }
}


void GameUserInterface::renderLevelUpMessage(S32 newLevel) const
{
   static string msg[] = { "", 
                           "CONGRATULATIONS!",
                           "YOU HAVE BEEN PROMOTED TO",
                           "LEVEL XXX",                     // <== This line will be updated below
                           "PRESS ANY KEY TO CONTINUE"
                           "" };

   msg[3] = "LEVEL " + itos(newLevel);
   renderMsgBox(msg, ARRAYSIZE(msg), Colors::white);
}


// This is a helper for renderSuspendedMessage and renderLevelUpMessage.  It assumes that none of the messages
// will have [[key_bindings]] in them.  If this assumption changes, will need to replace the NULL below in the
// SymbolString() construction.
void GameUserInterface::renderMsgBox(const string *message, S32 msgLines, const Color &msgColor) const
{
   Vector<SymbolShapePtr> messages(msgLines);

   for(S32 i = 0; i < msgLines; i++)
      messages.push_back(SymbolShapePtr(new SymbolString(message[i], NULL, ErrorMsgContext, 30, msgColor, true)));

   // Use empty shared pointer instead of NULL
   renderMessageBox(shared_ptr<SymbolShape>(), shared_ptr<SymbolShape>(),
         messages.address(), messages.size(), -30, 2);
}


void GameUserInterface::renderLevelListDisplayer() const
{
   mLevelListDisplayer.render();
}


void GameUserInterface::renderLostConnectionMessage() const
{
   GameConnection *connection = getGame()->getConnectionToServer();

   if(connection && connection->lostContact())
   {
      //static string msg = "We have lost contact with the server; You can't play "
      //                    "until the connection has been re-established.\n\n"
      //                    "Trying to reconnect... [[SPINNER]]";
      //renderMessageBox("SERVER CONNECTION PROBLEMS", "", msg, -30);

      // Above: the old way of displaying connection problem

      // You may test this rendering by using /lag 0 100

      renderCenteredFancyBox(130, 54, 130, 10, Colors::red30, 0.75f, Colors::white);

      RenderUtils::drawStringc(430, 170, 30, Colors::white, "CONNECTION INTERRUPTED");

      const S32 x1 = 140;
      const S32 y1 = 142;

      RenderUtils::drawFilledRect(x1 + 1,  y1 + 20, 7, 30, Colors::black);
      RenderUtils::drawFilledRect(x1 + 11, y1 + 15, 7, 30, Colors::black);
      RenderUtils::drawFilledRect(x1 + 21, y1 + 10, 7, 30, Colors::black);
      RenderUtils::drawFilledRect(x1 + 31, y1 + 05, 7, 30, Colors::black);
      RenderUtils::drawFilledRect(x1 + 41, y1 + 00, 7, 30, Colors::black);

      RenderUtils::drawRect(x1 + 1,  y1 + 20, 7, 30, Colors::gray40);
      RenderUtils::drawRect(x1 + 11, y1 + 15, 7, 30, Colors::gray40);
      RenderUtils::drawRect(x1 + 21, y1 + 10, 7, 30, Colors::gray40);
      RenderUtils::drawRect(x1 + 31, y1 + 05, 7, 30, Colors::gray40);
      RenderUtils::drawRect(x1 + 41, y1 + 00, 7, 30, Colors::gray40);


      if((Platform::getRealMilliseconds() & 0x300) != 0) // Draw flashing red "X" on empty connection bars
      {
         static const F32 vertices[] = {x1 + 5, y1 - 5, x1 + 45, y1 + 35,  x1 + 5, y1 + 35, x1 + 45, y1 - 5 };
         RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH * 2.0f);
         RenderUtils::drawLines(vertices, 4, Colors::red);
         RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
      }
   }
}


void GameUserInterface::renderShutdownMessage() const
{
   if(mShutdownMode == None)
      return;

   if(mShutdownMode == ShuttingDown)
   {
      char timemsg[255];
      dSprintf(timemsg, sizeof(timemsg), "Server is shutting down in %d seconds.", (S32) (mShutdownTimer.getCurrent() / 1000));

      if(mShutdownInitiator)     // Local client intitiated the shutdown
      {
         string msg = string(timemsg) + "\n\nShutdown sequence intitated by you.\n\n" + mShutdownReason.getString();
         renderMessageBox("SERVER SHUTDOWN INITIATED", "Press [[Esc]] to cancel shutdown", msg, Colors::white, 7);
      }
      else                       // Remote user intiated the shutdown
      {
         char whomsg[255];
         dSprintf(whomsg, sizeof(whomsg), "Shutdown sequence initiated by %s.", mShutdownName.getString());

         string msg = string(timemsg) + "\n\n" + 
                      whomsg + "\n\n" + 
                      mShutdownReason.getString();
         renderMessageBox("SHUTDOWN INITIATED", "Press [[Esc]] to dismiss", msg, Colors::white, 7);
      }
   }
   else if(mShutdownMode == Canceled)
   {
      // Keep same number of messages as above, so if message changes, it will be a smooth transition
      string msg = "Server shutdown sequence canceled.\n\n"
                   "Play on!";

      renderMessageBox("SHUTDOWN CANCELED", "Press [[Esc]] to dismiss", msg, Colors::white, 7);
   }
}


void GameUserInterface::prepareStars()
{
   static const Color starYellow(1.0f, 1.0f, 0.7f);
   static const Color starBlue(0.7f, 0.7f, 1.0f);
   static const Color starRed(1.0f, 0.7f, 0.7f);
   static const Color starGreen(0.7f, 1.0f, 0.7f);
   static const Color starOrange(1.0f, 0.7f, 0.4f);

   // Default white-blue
   static const Color starColor(0.8f, 0.8f, 1.0f);

   // Create some random stars
   for(S32 i = 0; i < NumStars; i++)
   {
      // Positions
      mStars[i].set(TNL::Random::readF(), TNL::Random::readF());    // Between 0 and 1

      // Colors
      S32 starSeed = TNL::Random::readI(0, 100);

      if(starSeed < 2)
         mStarColors[i] = starGreen;
      else if(starSeed < 4)
         mStarColors[i] = starBlue;
      else if(starSeed < 6)
         mStarColors[i] = starRed;
      else if(starSeed < 8)
         mStarColors[i] = starOrange;
      else if(starSeed < 11)
         mStarColors[i] = starYellow;
      else
         mStarColors[i] = starColor;
   }

   // //Create some random hexagons
   //for(U32 i = 0; i < NumStars; i++)
   //{
   //   F32 x = TNL::Random::readF();
   //   F32 y = TNL::Random::readF();
   //   F32 ang = TNL::Random::readF() * Float2Pi;
   //   F32 size = TNL::Random::readF() * .1;


   //   for(S32 j = 0; j < 6; j++)
   //   {
   //      mStars[i * 6 + j].x = x + sin(ang + Float2Pi / 6 * j) * size;      // Between 0 and 1
   //      mStars[i * 6 + j].y = y + cos(ang + Float2Pi / 6 * j) * size;
   //   }
   //}
}


void GameUserInterface::shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator)
{
   mShutdownMode = ShuttingDown;
   mShutdownName = who;
   mShutdownReason = why;
   mShutdownInitiator = initiator;
   mShutdownTimer.reset(time * 1000);
}


void GameUserInterface::cancelShutdown()
{
   mShutdownMode = Canceled;
}


void GameUserInterface::showLevelLoadDisplay(bool show, bool fade)
{
   mLevelListDisplayer.showLevelLoadDisplay(show, fade);
}


void GameUserInterface::serverLoadedLevel(const string &levelName)
{
   mLevelListDisplayer.addLevelName(levelName);
}


// Draws level-load progress bar across the bottom of the screen
void GameUserInterface::renderProgressBar() const
{
   GameType *gt = getGame()->getGameType();
   if((mShowProgressBar || mProgressBarFadeTimer.getCurrent() > 0) && gt && gt->mObjectsExpected > 0)
   {
      // Outline
      const F32 left = 200;
      const F32 width = DisplayManager::getScreenInfo()->getGameCanvasWidth() - 2 * left;
      const F32 height = 10;

      // For some reason, there are occasions where the status bar doesn't progress all the way over during the load process.
      // The problem is that, for some reason, some objects do not add themselves to the loaded object counter, and this creates
      // a disconcerting effect, as if the level did not fully load.  Rather than waste any more time on this problem, we'll just
      // fill in the status bar while it's fading, to make it look like the level fully loaded.  Since the only thing that this
      // whole mechanism is used for is to display something to the user, this should work fine.
      F32 barWidth = mShowProgressBar ? S32((F32)width * (F32)gt->getObjectsLoaded() / (F32)gt->mObjectsExpected) : width;
      F32 alpha = mShowProgressBar ? 1 : mProgressBarFadeTimer.getFraction();

      for(S32 i = 1; i >= 0; i--)
      {
         F32 w = i ? width : barWidth;

         F32 vertices[] = {
               left,     F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin),
               left + w, F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin),
               left + w, F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - height),
               left,     F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - height)
         };

         if(i)
            RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, Colors::green, alpha);
         else
            RenderUtils::drawFilledLineLoop(vertices, ARRAYSIZE(vertices) / 2, Colors::green, alpha);
      }
   }
}


// Draw the reticle (i.e. the mouse cursor) if we are using keyboard/mouse
void GameUserInterface::renderReticle() const
{
   bool shouldRender = getGame()->getInputMode() == InputModeKeyboard &&   // Reticle in keyboard mode only
                       getUIManager()->isCurrentUI<GameUserInterface>();   // And not when a menu is active
   if(!shouldRender)
      return;

   Point offsetMouse = mMousePoint + Point(DisplayManager::getScreenInfo()->getGameCanvasWidth()  * 0.5f, 
                                           DisplayManager::getScreenInfo()->getGameCanvasHeight() * 0.5f);

#define RETICLE_COLOR Colors::green
#define COLOR_RGB RETICLE_COLOR.r, RETICLE_COLOR.g, RETICLE_COLOR.b
#define RETICLE_ALPHA 0.7f
   F32 crossHairs[] = {
      // Center cross-hairs
      offsetMouse.x - 15, offsetMouse.y,
      offsetMouse.x + 15, offsetMouse.y,
      offsetMouse.x,      offsetMouse.y - 15,
      offsetMouse.x,      offsetMouse.y + 15,
   };
   RenderUtils::drawLines(crossHairs, ARRAYSIZE(crossHairs) / 2, RETICLE_COLOR, RETICLE_ALPHA);

   // And the axes lines
   RenderUtils::drawLineGradient(offsetMouse.x - 30, offsetMouse.y, 0, offsetMouse.y,
         RETICLE_COLOR, RETICLE_ALPHA, RETICLE_COLOR, 0.0f);

   RenderUtils::drawLineGradient(offsetMouse.x + 30, offsetMouse.y,
         (F32)DisplayManager::getScreenInfo()->getGameCanvasWidth(), offsetMouse.y,
         RETICLE_COLOR, RETICLE_ALPHA, RETICLE_COLOR, 0.0f);

   RenderUtils::drawLineGradient(offsetMouse.x, offsetMouse.y - 30, offsetMouse.x, 0,
      RETICLE_COLOR, RETICLE_ALPHA, RETICLE_COLOR, 0.0f);

   RenderUtils::drawLineGradient(offsetMouse.x, offsetMouse.y + 30,
         offsetMouse.x, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight(),
      RETICLE_COLOR, RETICLE_ALPHA, RETICLE_COLOR, 0.0f);

#undef COLOR_RGB
#undef RETICLE_COLOR
}


void GameUserInterface::renderWrongModeIndicator() const
{
   if(mWrongModeMsgDisplay.getCurrent())
   {
      // Fade for last half second
      F32 alpha = mWrongModeMsgDisplay.getCurrent() < 500 ? mWrongModeMsgDisplay.getCurrent() / 500.0f : 1.0f;

      FontManager::pushFontContext(HelperMenuContext);
      RenderUtils::drawCenteredString_fixed(245, 20, Colors::red, alpha, "You are in joystick mode.");
      RenderUtils::drawCenteredString_fixed(270, 20, Colors::red, alpha, "You can change to Keyboard input with the Options menu.");
      FontManager::popFontContext();
   }
}


void GameUserInterface::onMouseDragged()
{
   TNLAssert(false, "Is this ever called?");  // Probably not!
   onMouseMoved();
}


void GameUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   mMousePoint.set(DisplayManager::getScreenInfo()->getMousePos()->x - DisplayManager::getScreenInfo()->getGameCanvasWidth()  / 2,
                   DisplayManager::getScreenInfo()->getMousePos()->y - DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2);

   if(mInCommanderMap)     // Ship not in center of the screen in cmdrs map.  Where is it?
   {
      Ship *ship = getGame()->getLocalPlayerShip();

      if(!ship)
         return;

      Point o = ship->getRenderPos();  // To avoid taking address of temporary
      Point p = worldToScreenPoint(&o, DisplayManager::getScreenInfo()->getGameCanvasWidth(), 
                                       DisplayManager::getScreenInfo()->getGameCanvasHeight());

      mCurrentMove.angle = atan2(mMousePoint.y + DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2 - p.y, 
                                 mMousePoint.x + DisplayManager::getScreenInfo()->getGameCanvasWidth()  / 2 - p.x);
   }

   else     // Ship is at center of the screen
      mCurrentMove.angle = atan2(mMousePoint.y, mMousePoint.x);
}


// Called from renderObjectiveArrow() & ship's onMouseMoved() when in commander's map
Point GameUserInterface::worldToScreenPoint(const Point *point,  S32 canvasWidth, S32 canvasHeight) const
{
   Ship *ship = getGame()->getLocalPlayerShip();

   if(!ship)
      return Point(0,0);

   Point position = ship->getRenderPos();    // Ship's location (which will be coords of screen's center)
   
   if(renderWithCommanderMap())              
   {
      F32 zoomFrac = getCommanderZoomFraction();
      const Rect *worldExtentRect = getGame()->getWorldExtents();

      Point worldExtents = worldExtentRect->getExtents();
      worldExtents.x *= canvasWidth  / F32(canvasWidth  - (UserInterface::horizMargin * 2));
      worldExtents.y *= canvasHeight / F32(canvasHeight - (UserInterface::vertMargin  * 2));


      F32 aspectRatio = worldExtents.x / worldExtents.y;
      F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);

      if(aspectRatio > screenAspectRatio)
         worldExtents.y *= aspectRatio / screenAspectRatio;
      else
         worldExtents.x *= screenAspectRatio / aspectRatio;


      Point offset = (worldExtentRect->getCenter() - position) * zoomFrac + position;
      Point visSize = getGame()->computePlayerVisArea(ship) * 2;
      Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

      Point visScale(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y );

      Point ret = (*point - offset) * visScale + Point((canvasWidth / 2), (canvasHeight / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = getGame()->computePlayerVisArea(ship);
      Point scaleFactor((canvasWidth / 2) / visExt.x, (canvasHeight / 2) / visExt.y);

      Point ret = (*point - position) * scaleFactor + Point((canvasWidth / 2), (canvasHeight / 2));
      return ret;
   }
}


// Returns true if we are either in the cmdrs map, or are transitioning
bool GameUserInterface::renderWithCommanderMap() const
{
   return mInCommanderMap || mCommanderZoomDelta.getCurrent() > 0;
}


// Is engineer enabled on this level?  Only set at beginning of level, not changed during game
void GameUserInterface::pregameSetup(bool engineerEnabled)
{
   mHelperManager.pregameSetup(engineerEnabled);
}


void GameUserInterface::setSelectedEngineeredObject(U32 objectType)
{
   mHelperManager.setSelectedEngineeredObject(objectType);
}


void GameUserInterface::activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd)
{
   mHelperManager.activateHelper(helperType, activatedWithChatCmd);
   playBoop();
}


// Used only for testing
bool GameUserInterface::isHelperActive(HelperMenu::HelperMenuType helperType) const
{
   return mHelperManager.isHelperActive(helperType);
}


// Used only for testing
const HelperMenu *GameUserInterface::getActiveHelper() const
{
   return mHelperManager.getActiveHelper();
}


void GameUserInterface::renderEngineeredItemDeploymentMarker(Ship *ship) const
{
   mHelperManager.renderEngineeredItemDeploymentMarker(ship);
}


// Runs on client
void GameUserInterface::dropItem()
{
   if(!getGame()->getConnectionToServer())
      return;

   Ship *ship = getGame()->getLocalPlayerShip();

   GameType *gt = getGame()->getGameType();
   if(!ship || !gt)
      return;

   if(!gt->isCarryingItems(ship))
   {
      displayErrorMessage("You don't have any items to drop!");
      return;
   }

   gt->c2sDropItem();
}


// Select next weapon
void GameUserInterface::chooseNextWeapon()
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sChooseNextWeapon();
}


void GameUserInterface::choosePrevWeapon()
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sChoosePrevWeapon();
}


// Select a weapon by its index
void GameUserInterface::selectWeapon(U32 indx)
{
   GameType *gameType = getGame()->getGameType();
   if(gameType)
      gameType->c2sSelectWeapon(indx);

   mHelpItemManager.removeInlineHelpItem(ChangeWeaponsItem, true);      // User has demonstrated this skill
}


void GameUserInterface::activateModule(S32 index)
{
   // Still active, just return
   if(!getGame() || !getGame()->getLocalPlayerShip() || mModPrimaryActivated[index])
      return;

   // Activate module primary component
   mModPrimaryActivated[index] = true;
   setModulePrimary(getGame()->getLocalPlayerShip()->getModule(index), true);

   // If the module secondary double-tap timer hasn't run out, activate the secondary component
   if(mModuleDoubleTapTimer[index].getCurrent() != 0)
      mModSecondaryActivated[index] = true;

   // Now reset the double-tap timer since we've just activate this module
   mModuleDoubleTapTimer[index].reset();

   // Player figured out how to activate their modules... skip related help
   mHelpItemManager.removeInlineHelpItem(ControlsModulesItem, true);           

   if(getGame()->getLocalPlayerShip()->getModule(index) == ModuleCloak)
      mHelpItemManager.removeInlineHelpItem(TryCloakItem, true);     // Already tried it!
   else if(getGame()->getLocalPlayerShip()->getModule(index) == ModuleBoost)
      mHelpItemManager.removeInlineHelpItem(TryTurboItem, true);     // Already tried it!
}


void GameUserInterface::toggleLevelRating()
{
   if(!getGame()->canRateLevel())      // Will display any appropriate error messages
      return;

   PersonalRating newRating = getGame()->toggleLevelRating();  // Change rating and get new value

   string msg = "Your rating: " + getPersonalRatingString(newRating);
   displaySuccessMessage(msg.c_str());

   mHelpItemManager.removeInlineHelpItem(RateThisLevel, true);             // Demonstrated ability to rate a level!
}


// Static method
string GameUserInterface::getPersonalRatingString(PersonalRating rating)
{
   if(rating == RatingGood)      return "+1";
   if(rating == RatingNeutral)   return "0";
   if(rating == RatingBad)       return "-1";

   return getTotalRatingString((S16)rating);    // Handles UnknownRating, Unrated
}


// Static method
string GameUserInterface::getTotalRatingString(S16 rating)
{
   if(rating == UnknownRating)      return "?";
   if(rating == Unrated)            return "Unrated";

   return (rating > 0 ? "+" : "") + itos(rating);
}


// A new loadout has arrived
void GameUserInterface::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   mLoadoutIndicator.newLoadoutHasArrived(loadout);
}


void GameUserInterface::setActiveWeapon(U32 weaponIndex)
{
   mLoadoutIndicator.setActiveWeapon(weaponIndex);
}


void GameUserInterface::updateLeadingPlayerAndScore()
{
   mTimeLeftRenderer.updateLeadingPlayerAndScore(getGame());
}


// Used?
void GameUserInterface::setModulePrimary(ShipModule module, bool isActive)
{
   mLoadoutIndicator.setModulePrimary(module, isActive);
}


void GameUserInterface::setModuleSecondary(ShipModule module, bool isActive)
{
   mLoadoutIndicator.setModuleSecondary(module, isActive);
}


// Returns the width of the current loadout, as rendered
S32 GameUserInterface::getLoadoutIndicatorWidth() const
{
   return mLoadoutIndicator.getWidth();
}


bool GameUserInterface::scoreboardIsVisible() const
{
   // GameType can be NULL when first starting up
   return mInScoreboardMode || getGame()->isGameOver();
}


Point GameUserInterface::getTimeLeftIndicatorWidthAndHeight() const
{
   return mTimeLeftRenderer.render(getGame()->getGameType(), scoreboardIsVisible(), getGame()->areTeamsLocked(), false);
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
bool GameUserInterface::onKeyDown(InputCode inputCode)
{
   string inputString = InputCodeManager::getCurrentInputString(inputCode);

   // Kind of hacky, but this will unsuspend and swallow the keystroke, which is what we want
   if(!mHelperManager.isHelperActive())
   {
      if(getGame()->getClientInfo()->getShowLevelUpMessage() != NONE)
      {
         getGame()->undelaySpawn();
         if(inputCode != KEY_ESCAPE)  // Don't swollow escape
            return true;
      }
      else if(getGame()->isSpawnDelayed())
      {
         // Allow scoreboard and the various chats while idle
         if(!checkInputCode(BINDING_LOBBYCHAT, inputString) &&
            !checkInputCode(BINDING_GLOBCHAT,  inputCode)   &&
            !checkInputCode(BINDING_TEAMCHAT,  inputCode)   &&
            !checkInputCode(BINDING_CMDCHAT,   inputCode)   &&
            !checkInputCode(BINDING_SCRBRD,    inputCode))
         {
            getGame()->undelaySpawn();
            if(inputCode != KEY_ESCAPE)  // Don't swollow escape: Lagged out and can't un-idle to bring up the menu?
               return true;
         }
      }
   }

   if(checkInputCode(BINDING_LOBBYCHAT, inputString))
      getGame()->setBusyChatting(true);

   if(Parent::onKeyDown(inputCode))    // Let parent try handling the key
      return true;

   if(GameManager::gameConsole->onKeyDown(inputCode))   // Pass the key on to the console for processing
      return true;

   if(checkInputCode(BINDING_HELP, inputString))   // Turn on help screen
   {
      playBoop();
      getGame()->setBusyChatting(true);

      // If we have a helper, let that determine what happens when the help key is pressed.  Otherwise, show help normally.
      if(mHelperManager.isHelperActive())
         mHelperManager.activateHelp(getUIManager());
      else
         getUIManager()->activate<InstructionsUserInterface>();

      mHelpItemManager.removeInlineHelpItem(F1HelpItem, true);    // User knows how to access help

      return true;
   }

   // Ctrl-/ toggles console window for the moment
   // Only open when there are no active helpers
   if(!mHelperManager.isHelperActive() && inputCode == KEY_SLASH && InputCodeManager::checkModifier(KEY_CTRL))
   {
      if(GameManager::gameConsole->isOk())        // Console is only not Ok if something bad has happened somewhere
         GameManager::gameConsole->toggleVisibility();

      return true;
   }

   if(checkInputCode(BINDING_MISSION, inputCode))    // F2
   {
      onMissionKeyPressed();
      return true;
   }

   if(inputCode == KEY_M && InputCodeManager::checkModifier(KEY_CTRL))        // Ctrl+M, for now, to cycle through message dispaly modes
   {
      toggleChatDisplayMode();
      return true;
   }

   // Disallow chat when a level is loading.  This is a workaround for disappearing chats during
   // level transitions.  The true fix is probably to move chats from the GameType and into the GameConnection
   if(!mShowProgressBar && mHelperManager.isHelperActive() && mHelperManager.processInputCode(inputCode))   // Will return true if key was processed
   {
      // Experimental, to keep ship from moving after entering a quick chat that has the same shortcut as a movement key
      InputCodeManager::setState(inputCode, false);
      return true;
   }

   // If we're not in a helper, and we apply the engineer module, then we can handle that locally by displaying a menu or message
   if(!mHelperManager.isHelperActive())
   {
      Ship *ship = getGame()->getLocalPlayerShip();
         
      if(ship)
      {
         if((checkInputCode(BINDING_MOD1, inputCode) && ship->getModule(0) == ModuleEngineer) ||
            (checkInputCode(BINDING_MOD2, inputCode) && ship->getModule(1) == ModuleEngineer))
         {
            string msg = EngineerModuleDeployer::checkResourcesAndEnergy(ship);  // Returns "" if ok, error message otherwise

            if(msg != "")
               displayErrorMessage(msg.c_str());
            else
               activateHelper(HelperMenu::EngineerHelperType);

            return true;
         }
      }
   }

#ifdef TNL_DEBUG
   // These commands only available in debug builds
   if(inputCode == KEY_H && InputCodeManager::checkModifier(KEY_SHIFT))     // Shift+H to show next real HelpItem
      mHelpItemManager.debugAdvanceHelpItem();

   if(inputCode == KEY_H && InputCodeManager::checkModifier(KEY_CTRL))     // Ctrl+H to show next dummy HelpItem
      mHelpItemManager.debugShowNextSampleHelpItem();

#endif

   if(!GameManager::gameConsole->isVisible())
   {
      if(!isChattingOrTypingCommand())
         return processPlayModeKey(inputCode);
   }

   return false;
}


// User has pressed F2
void GameUserInterface::onMissionKeyPressed()
{
   if(!mMissionOverlayActive)
   {
      mMissionOverlayActive = true;

      if(!mLevelInfoDisplayer.isDisplayTimerActive())
         mLevelInfoDisplayer.onActivated();

      mLevelInfoDisplayer.clearDisplayTimer();                    // Clear timer so releasing F2 will hide the display
      mHelpItemManager.removeInlineHelpItem(GameModesItem, true); // User seems to know about F2, unqueue help message
   }
}


void GameUserInterface::onMissionKeyReleased()
{
   mMissionOverlayActive = false;
   mLevelInfoDisplayer.onDeactivated();
}


void GameUserInterface::onTextInput(char ascii)
{
   if(GameManager::gameConsole->isVisible())
      GameManager::gameConsole->onKeyDown(ascii);

   mHelperManager.onTextInput(ascii);
}


// Helper function...
static void saveLoadoutPreset(ClientGame *game, const LoadoutTracker *loadout, S32 slot)
{
   game->getSettings()->setLoadoutPreset(loadout, slot);
   game->displaySuccessMessage(("Current loadout saved as preset " + itos(slot + 1)).c_str());
}


static void loadLoadoutPreset(ClientGame *game, S32 slot)
{
   LoadoutTracker loadout = game->getSettings()->getLoadoutPreset(slot);

   if(!loadout.isValid())
   {
      string msg = "Preset " + itos(slot + 1) + " is undefined -- to define it, try Ctrl+" + itos(slot + 1);
      game->displayErrorMessage(msg.c_str());
      return;
   }

   //GameType *gameType = game->getGameType();
   //if(!gameType)
   //   return;

   game->requestLoadoutPreset(slot);
}


bool checkInputCode(InputCode codeUserEntered, InputCode codeToActivateCommand)
{
   return codeUserEntered == codeToActivateCommand;
}


// Helper function -- checks input keys and sees if we should start chatting.  Returns true if entered chat mode, false if not.
bool GameUserInterface::checkEnterChatInputCode(InputCode inputCode)
{
   if(checkInputCode(BINDING_TEAMCHAT, inputCode))          // Start entering a team chat msg
      mHelperManager.activateHelper(ChatHelper::TeamChat);
   else if(checkInputCode(BINDING_GLOBCHAT, inputCode))     // Start entering a global chat msg
      mHelperManager.activateHelper(ChatHelper::GlobalChat);
   else if(checkInputCode(BINDING_CMDCHAT, inputCode))      // Start entering a command
      mHelperManager.activateHelper(ChatHelper::CmdChat);
   else
      return false;

   return true;
}


// Can only get here if we're not in chat mode
bool GameUserInterface::processPlayModeKey(InputCode inputCode)
{
   // The following keys are allowed in both play mode and in loadout or
   // engineering menu modes if not used in the loadout menu above
   // They are currently hardcoded, both here and in the instructions
   if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_ALT))          // Alt+] advances bots by one step if frozen
      EventManager::get()->addSteps(1);
   else if(inputCode == KEY_CLOSEBRACKET && InputCodeManager::checkModifier(KEY_CTRL))    // Ctrl+] advances bots by 10 steps if frozen
      EventManager::get()->addSteps(10);

   else if(checkInputCode(BINDING_LOAD_PRESET_1, inputCode))  // Loading loadout presets
      loadLoadoutPreset(getGame(), 0);
   else if(checkInputCode(BINDING_LOAD_PRESET_2, inputCode))
      loadLoadoutPreset(getGame(), 1);
   else if(checkInputCode(BINDING_LOAD_PRESET_3, inputCode))
      loadLoadoutPreset(getGame(), 2);

   else if(checkInputCode(BINDING_SAVE_PRESET_1, inputCode))  // Saving loadout presets
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 0);
   else if(checkInputCode(BINDING_SAVE_PRESET_2, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 1);
   else if(checkInputCode(BINDING_SAVE_PRESET_3, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 2);

   else if(checkInputCode(BINDING_MOD1, inputCode))
      activateModule(0);
   else if(checkInputCode(BINDING_MOD2, inputCode))
      activateModule(1);
   else if(checkInputCode(BINDING_FIRE, inputCode))
   {
      mFiring = true;
      mHelpItemManager.removeInlineHelpItem(ControlsKBItem, true, 0xFF - 1);     // Player has demonstrated knowledge of how to fire
   }
   else if(checkInputCode(BINDING_SELWEAP1, inputCode))
      selectWeapon(0);
   else if(checkInputCode(BINDING_SELWEAP2, inputCode))
      selectWeapon(1);
   else if(checkInputCode(BINDING_SELWEAP3, inputCode))
      selectWeapon(2);
   else if(checkInputCode(BINDING_FPS, inputCode))
   {
      if(InputCodeManager::checkModifier(KEY_CTRL))
         mConnectionStatsRenderer.toggleVisibility();
      else
         mFpsRenderer.toggleVisibility();
   }
   else if(checkInputCode(BINDING_ADVWEAP, inputCode))
      chooseNextWeapon();

   // By default, Handle mouse wheel. Users can change it in "Define Keys" option
   else if(checkInputCode(BINDING_ADVWEAP2, inputCode))
      chooseNextWeapon();
   else if(checkInputCode(BINDING_PREVWEAP, inputCode))
      choosePrevWeapon();
   else if(checkInputCode(BINDING_TOGGLE_RATING, inputCode))
      toggleLevelRating();

   else if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)
   {
      if(mShutdownMode == ShuttingDown)
      {
         if(mShutdownInitiator)
         {
            getGame()->getConnectionToServer()->c2sRequestCancelShutdown();
            mShutdownMode = Canceled;
         }
         else
            mShutdownMode = None;

         return true;
      }
      else if(mShutdownMode == Canceled)
      {
         mShutdownMode = None;
         return true;
      }

      playBoop();

      if(!getGame()->isConnectedToServer())     // Perhaps we're still joining?
      {
         getGame()->closeConnectionToGameServer();
         getUIManager()->reactivate(getUIManager()->getUI<MainMenuUserInterface>());      // Back to main menu
      }
      else
      {
         getGame()->setBusyChatting(true);
         getUIManager()->activate<GameMenuUserInterface>();
      }
   }
   else if(checkInputCode(BINDING_CMDRMAP, inputCode))
   {
      if(!mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap)
         return true;

      toggleCommanderMap();

      // Suppress key repeat by disabling cmdrs map until keyUp event is received
      mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = false;
      
      // Now that we've demonstrated use of cmdrs map, no need to tell player about it
      mHelpItemManager.removeInlineHelpItem(CmdrsMapItem, true);  
   }

   else if(checkInputCode(BINDING_SCRBRD, inputCode))
   {     // (braces needed)
      if(!mInScoreboardMode)    // We're activating the scoreboard
      {
         mInScoreboardMode = true;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(true);

         mHelpItemManager.removeInlineHelpItem(ViewScoreboardItem, true);  // User found the tab key!
      }
   }
   else if(checkInputCode(BINDING_TOGVOICE, inputCode))
   {     // (braces needed)
      if(!mVoiceRecorder.mRecordingAudio)  // Turning recorder on
         mVoiceRecorder.start();
   }

   // The following keys are only allowed when there are no helpers or when the top helper permits
   else if(mHelperManager.isChatAllowed())    
   {
      if(checkEnterChatInputCode(inputCode))
         return true;

      // These keys are only available when there is no helper active
      if(!mHelperManager.isHelperActive())
      {
         if(checkInputCode(BINDING_QUICKCHAT, inputCode))
            activateHelper(HelperMenu::QuickChatHelperType);
         else if(checkInputCode(BINDING_LOADOUT, inputCode))
            activateHelper(HelperMenu::LoadoutHelperType);
         else if(checkInputCode(BINDING_DROPITEM, inputCode))
            dropItem();
         // Check if the user is trying to use keyboard to move when in joystick mode
         else if(getGame()->getInputMode() == InputModeJoystick)
            checkForKeyboardMovementKeysInJoystickMode(inputCode);
      }
   }
   else
      return false;

   return true;
}


// Toggles commander's map activation status
void GameUserInterface::toggleCommanderMap()
{
   mInCommanderMap = !mInCommanderMap;
   mCommanderZoomDelta.invert();

   if(mInCommanderMap)
      playSoundEffect(SFXUICommUp);
   else
      playSoundEffect(SFXUICommDown);

   getGame()->setUsingCommandersMap(mInCommanderMap);
}


SFXHandle GameUserInterface::playSoundEffect(U32 profileIndex, F32 gain) const
{
   return getUIManager()->playSoundEffect(profileIndex, gain);
}


// Show a message if the user starts trying to play with keyboard in joystick mode
void GameUserInterface::checkForKeyboardMovementKeysInJoystickMode(InputCode inputCode)
{
   if(checkInputCode(BINDING_UP,    inputCode) ||
      checkInputCode(BINDING_DOWN,  inputCode) ||
      checkInputCode(BINDING_LEFT,  inputCode) ||
      checkInputCode(BINDING_RIGHT, inputCode))
         mWrongModeMsgDisplay.reset(THREE_SECONDS);
}


// This is a bit complicated to explain... basically, when chatRelated is true, it won't apply a dimming factor
// when entering a chat message.  When false, it will.
F32 GameUserInterface::getBackgroundTextDimFactor(bool chatRelated) const
{
   F32 helperManagerFactor = chatRelated ? 
         mHelperManager.getDimFactor() : 
         MAX(mHelperManager.getFraction(), UI::DIM_LEVEL);

   // Hide help message when scoreboard is visible
   if(mInScoreboardMode)
      helperManagerFactor = 0;

   return MIN(helperManagerFactor, mLevelInfoDisplayer.getFraction());
}


void GameUserInterface::renderChatMsgs() const
{
   bool announcementActive = (mAnnouncementTimer.getCurrent() != 0);

   F32 alpha = 1; // getBackgroundTextDimFactor(true);

   // Only fade in chat messages when dealing with player chat
   F32 chatComposerFadein = mHelperManager.isOrWasComposingPlayerChat() ? 1 - mHelperManager.getFraction() : 0;

   mChatMessageDisplayer.render(IN_GAME_CHAT_DISPLAY_POS, chatComposerFadein, isChatting(), announcementActive, alpha);
   mServerMessageDisplayer.render(messageMargin, 0, false, false, alpha);

   if(announcementActive)
      renderAnnouncement(IN_GAME_CHAT_DISPLAY_POS);
}


void GameUserInterface::renderAnnouncement(S32 pos) const
{
   const S32 fontsize = 16;

   RenderUtils::lineWidth(RenderUtils::LINE_WIDTH_4);
   RenderUtils::drawStringAndGetWidth_fixed(horizMargin, pos + fontsize, fontsize, Colors::red, ("*** " + mAnnouncement + " ***").c_str());
   RenderUtils::lineWidth(RenderUtils::DEFAULT_LINE_WIDTH);
}


void GameUserInterface::onKeyUp(InputCode inputCode)
{
   // These keys works in any mode!  And why not??

   if(checkInputCode(BINDING_MISSION, inputCode))    // F2
      onMissionKeyReleased();

   else if(checkInputCode(BINDING_MOD1, inputCode))
   {
      mModPrimaryActivated[0] = false;
      mModSecondaryActivated[0] = false;

      if(getGame()->getLocalPlayerShip())    // Sometimes false if in hit any key to continue mode
         setModulePrimary(getGame()->getLocalPlayerShip()->getModule(0), false);
   }
   else if(checkInputCode(BINDING_MOD2, inputCode))
   {
      mModPrimaryActivated[1] = false;
      mModSecondaryActivated[1] = false;

      if(getGame()->getLocalPlayerShip())    // Sometimes false if in hit any key to continue mode
         setModulePrimary(getGame()->getLocalPlayerShip()->getModule(1), false);
   }
   else if(checkInputCode(BINDING_FIRE, inputCode))
      mFiring = false;
   else if(checkInputCode(BINDING_SCRBRD, inputCode))
   {     // (braces required)
      if(mInScoreboardMode)     // We're turning scoreboard off
      {
         mInScoreboardMode = false;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(false);
      }
   }
   else if(checkInputCode(BINDING_TOGVOICE, inputCode))
   {     // (braces required)
      if(mVoiceRecorder.mRecordingAudio)  // Turning recorder off
         mVoiceRecorder.stop();
   }
   else if(checkInputCode(BINDING_CMDRMAP, inputCode))
      mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;
}


void GameUserInterface::receivedControlUpdate(bool recvd)
{
   mGotControlUpdate = recvd;
}


bool GameUserInterface::isInScoreboardMode()
{
   return mInScoreboardMode;
}


static void joystickUpdateMove(ClientGame *game, GameSettings *settings, Move *theMove)
{
   // Set the move coordinates to the joystick normalized values
   theMove->x = game->normalizedAxesValues[SDL_CONTROLLER_AXIS_LEFTX];
   theMove->y =  game->normalizedAxesValues[SDL_CONTROLLER_AXIS_LEFTY];

   // Same with the shooting coordinates
   Point p(game->normalizedAxesValues[SDL_CONTROLLER_AXIS_RIGHTX],
         game->normalizedAxesValues[SDL_CONTROLLER_AXIS_RIGHTY]);

   F32 fact =  p.len();

   // TODO pull out these constants and put them into the INI?
   if(fact > 0.66f)        // It requires a large movement to actually fire...
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = true;
   }
   else if(fact > 0.25)    // ...but you can change aim with a smaller one
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = false;
   }
   else
      theMove->fire = false;
}


// Return current move (actual move processing in ship.cpp)
// Will also transform move into "relative" mode if needed
// At the end, all input supplied here will be overwritten if
// we are using a game controller.  What a mess!
Move *GameUserInterface::getCurrentMove()
{
   Move *move = &mCurrentMove;

   if(!mDisableShipKeyboardInput && getUIManager()->isCurrentUI<GameUserInterface>() && !GameManager::gameConsole->isVisible())
   {
      // Some helpers (like TeamShuffle) like to disable movement when they are active
      if(mHelperManager.isMovementDisabled())
      {
         mCurrentMove.x = 0;
         mCurrentMove.y = 0;
      }
      else
      {
         mCurrentMove.x = F32((InputCodeManager::getState(getInputCode(mGameSettings, BINDING_RIGHT)) ? 1 : 0) -
                              (InputCodeManager::getState(getInputCode(mGameSettings, BINDING_LEFT))  ? 1 : 0));

         mCurrentMove.y = F32((InputCodeManager::getState(getInputCode(mGameSettings, BINDING_DOWN))  ? 1 : 0) -
                              (InputCodeManager::getState(getInputCode(mGameSettings, BINDING_UP))    ? 1 : 0));
      }

      // If player is moving, do not show move instructions
      if(mCurrentMove.y > 0 || mCurrentMove.x > 0)
         mHelpItemManager.removeInlineHelpItem(ControlsKBItem, true, 1);


      mCurrentMove.fire = mFiring;

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      {
         mCurrentMove.modulePrimary[i]   = mModPrimaryActivated[i];
         mCurrentMove.moduleSecondary[i] = mModSecondaryActivated[i];
      }
   }
   else
   {
      mCurrentMove.x = 0;
      mCurrentMove.y = 0;

      mCurrentMove.fire = mFiring;     // should be false?

      for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      {
         mCurrentMove.modulePrimary[i] = false;
         mCurrentMove.moduleSecondary[i] = false;
      }
   }

   // Using relative controls -- all turning is done relative to the direction of the ship, so
   // we need to udate the move a little
   if(mGameSettings->getSetting<RelAbs>(IniKey::ControlMode) == Relative)
   {
      mTransformedMove = mCurrentMove;    // Copy move

      Point moveDir(mCurrentMove.x, -mCurrentMove.y);

      Point angleDir(cos(mCurrentMove.angle), sin(mCurrentMove.angle));

      Point rightAngleDir(-angleDir.y, angleDir.x);
      Point newMoveDir = angleDir * moveDir.y + rightAngleDir * moveDir.x;

      mTransformedMove.x = newMoveDir.x;
      mTransformedMove.y = newMoveDir.y;

      // Sanity checks
      mTransformedMove.x = min( 1.0f, mTransformedMove.x);
      mTransformedMove.y = min( 1.0f, mTransformedMove.y);
      mTransformedMove.x = max(-1.0f, mTransformedMove.x);
      mTransformedMove.y = max(-1.0f, mTransformedMove.y);

      move = &mTransformedMove;
   }

   // But wait! There's more!
   // Overwrite theMove if we're using joystick (also does some other essential joystick stuff)
   // We'll also run this while in the menus so if we enter keyboard mode accidentally, it won't
   // kill the joystick.  The design of combining joystick input and move updating really sucks.
   if(getGame()->getInputMode() == InputModeJoystick || getUIManager()->isCurrentUI<OptionsMenuUserInterface>())
      joystickUpdateMove(getGame(), mGameSettings, move);

   return move;
}


void GameUserInterface::resetLevelInfoDisplayTimer()
{
   if(!mLevelInfoDisplayer.isActive())
      mLevelInfoDisplayer.onActivated();

   mLevelInfoDisplayer.resetDisplayTimer();
}


// Constructor
GameUserInterface::VoiceRecorder::VoiceRecorder(ClientGame *game)
{
   mRecordingAudio = false;
   mMaxAudioSample = 0;
   mMaxForGain = 0;
   mVoiceEncoder = new SpeexVoiceEncoder;

   mGame = game;

   mWantToStopRecordingAudio = false;
}


GameUserInterface::VoiceRecorder::~VoiceRecorder()
{
   stopNow();
}


void GameUserInterface::VoiceRecorder::idle(U32 timeDelta)
{
   if(mRecordingAudio)
   {
      if(mVoiceAudioTimer.update(timeDelta))
      {
         mVoiceAudioTimer.reset(VoiceAudioSampleTime);
         process();
      }
   }
}


void GameUserInterface::VoiceRecorder::render() const
{
   if(mRecordingAudio)
   {
      static const F32 x = 10;
      static const F32 y = 130;

      static const F32 height = 15;
      static const F32 width = 100;
      static F32 halfWidth = width/2;

      F32 amt = mMaxAudioSample / F32(0x7FFF);
      F32 level = amt * width;

      // Render low/high volume lines
      F32 vertices[] = {
            x,         y,
            x,         y + height,
            x + width, y,
            x + width, y + height
      };
      RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, Colors::white);

      Color c1 = Colors::green;
      Color c2 = Colors::yellow;
      Color c3 = Colors::red;
      NVGpaint gradient1 = nvgLinearGradient(nvg, x, y, x+halfWidth, y,
            nvgRGBAf(c1.r, c1.g, c1.b, 1.0), nvgRGBAf(c2.r, c2.g, c2.b, 1.0f));
      NVGpaint gradient2 = nvgLinearGradient(nvg, x+halfWidth, y, x+width, y,
            nvgRGBAf(c2.r, c2.g, c2.b, 1.0f), nvgRGBAf(c3.r, c3.g, c3.b, 1.0));

      nvgBeginPath(nvg);
      nvgRect(nvg, x, y, level, height);
      nvgFillPaint(nvg, gradient1);
      nvgFill(nvg);

      if(level > 50)
      {
         nvgBeginPath(nvg);
         nvgRect(nvg, x+halfWidth, y, level-halfWidth, height);
         nvgFillPaint(nvg, gradient2);
         nvgFill(nvg);
      }
   }
}


void GameUserInterface::VoiceRecorder::start()
{
   if(!(mGame->getConnectionToServer() && mGame->getConnectionToServer()->mVoiceChatEnabled))
   {
      mGame->displayErrorMessage("!!! Voice chat not allowed on this server");
      return;
   }

   mWantToStopRecordingAudio = 0; // linux repeadedly sends key-up / key-down when only holding key down (that was in GLUT, may )
   if(!mRecordingAudio)
   {
      mRecordingAudio = SoundSystem::startRecording();
      if(!mRecordingAudio)
         return;

      mUnusedAudio = new ByteBuffer(0);
      mRecordingAudio = true;
      mMaxAudioSample = 0;
      mVoiceAudioTimer.reset(FirstVoiceAudioSampleTime);

      // trim the start of the capture buffer:
      SoundSystem::captureSamples(mUnusedAudio);
      mUnusedAudio->resize(0);
   }
}

void GameUserInterface::VoiceRecorder::stopNow()
{
   if(mRecordingAudio)
   {
      process();

      mRecordingAudio = false;
      SoundSystem::stopRecording();
      mVoiceSfx = NULL;
      mUnusedAudio = NULL;
   }
}
void GameUserInterface::VoiceRecorder::stop()
{
   if(mWantToStopRecordingAudio == 0)
      mWantToStopRecordingAudio = 2;
}


void GameUserInterface::VoiceRecorder::process()
{
   if(!(mGame->getConnectionToServer() && mGame->getConnectionToServer()->mVoiceChatEnabled))
      stop();

   if(mWantToStopRecordingAudio != 0)
   {
      mWantToStopRecordingAudio--;
      if(mWantToStopRecordingAudio == 0)
      {
         stopNow();
         return;
      }
   }
   U32 preSampleCount = mUnusedAudio->getBufferSize() / 2;
   SoundSystem::captureSamples(mUnusedAudio);

   U32 sampleCount = mUnusedAudio->getBufferSize() / 2;
   if(sampleCount == preSampleCount)
      return;

   S16 *samplePtr = (S16 *) mUnusedAudio->getBuffer();
   mMaxAudioSample = 0;

   for(U32 i = preSampleCount; i < sampleCount; i++)
   {
      if(samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = samplePtr[i];
      else if(-samplePtr[i] > mMaxAudioSample)
         mMaxAudioSample = -samplePtr[i];
   }
   mMaxForGain = U32(mMaxForGain * 0.95f);
   S32 boostedMax = mMaxAudioSample + 2048;

   if(boostedMax > mMaxForGain)
      mMaxForGain = boostedMax;

   if(mMaxForGain > MaxDetectionThreshold)
   {
      // Apply some gain to the buffer:
      F32 gain = 0x7FFF / F32(mMaxForGain);
      for(U32 i = preSampleCount; i < sampleCount; i++)
      {
         F32 sample = gain * samplePtr[i];
         if(sample > 0x7FFF)
            samplePtr[i] = 0x7FFF;
         else if(sample < -0x7FFF)
            samplePtr[i] = -0x7FFF;
         else
            samplePtr[i] = S16(sample);
      }
      mMaxAudioSample = U32(mMaxAudioSample * gain);
   }

   ByteBufferPtr sendBuffer = mVoiceEncoder->compressBuffer(mUnusedAudio);

   if(sendBuffer.isValid())
   {
      GameType *gameType = mGame->getGameType();

      if(gameType && sendBuffer->getBufferSize() < 1024)      // Don't try to send too big
         gameType->c2sVoiceChat(mGame->getSettings()->getSetting<YesNo>(IniKey::VoiceEcho), sendBuffer);
   }
}



static const U32 Gap = 3;        // Small gap for use between various UI elements


void GameUserInterface::renderBasicInterfaceOverlay() const
{
   GameType *gameType = getGame()->getGameType();

   // Progress meter for file upload and download
   if(getGame()->getConnectionToServer())
   {
      F32 progress = getGame()->getConnectionToServer()->getFileProgressMeter();
      if(progress != 0)
      {
         RenderUtils::drawFilledRect(25, 200, progress * (DisplayManager::getScreenInfo()->getGameCanvasWidth() - 50), 10, Colors::yellow);
         RenderUtils::drawRect(25, 200, DisplayManager::getScreenInfo()->getGameCanvasWidth() - 50,                    10, Colors::yellow);
      }
   }
   
   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
      renderInputModeChangeAlert();

   bool showScore = scoreboardIsVisible();

   if(showScore && getGame()->getTeamCount() > 0)      // How could teamCount be 0?
      ScoreboardRenderer::renderScoreboard(getGame());
   
   // Render timer and associated doodads in the lower-right corner (includes teams-locked indicator)
   mTimeLeftRenderer.render(gameType, showScore, getGame()->areTeamsLocked(), true);
   
   renderTalkingClients();
   renderDebugStatus();
}


bool GameUserInterface::shouldRenderLevelInfo() const 
{
   return mLevelInfoDisplayer.isActive() || mMissionOverlayActive;
}


void GameUserInterface::renderLevelInfo() const
{
   // Level Info requires gametype.  It can be NULL when switching levels
   if(getGame()->getGameType() == NULL)
      return;

   if(shouldRenderLevelInfo())
      mLevelInfoDisplayer.render();
}


// Display alert about input mode changing
void GameUserInterface::renderInputModeChangeAlert() const
{
   F32 alpha = 1;

   if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
      alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

   RenderUtils::drawCenteredStringf_fixed(vertMargin + 150, 20, Colors::paleRed, alpha, "Input mode changed to %s",
                       getGame()->getInputMode() == InputModeJoystick ? "Joystick" : "Keyboard");
}


void GameUserInterface::renderTalkingClients() const
{
   S32 y = 170;
   Game *game = getGame();

   for(S32 i = 0; i < game->getClientCount(); i++)
   {
      ClientInfo *client = game->getClientInfo(i);

      if(client->getVoiceSFX()->isPlaying())
      {
         const S32 TEXT_HEIGHT = 20;

         RenderUtils::drawString_fixed(10, y, TEXT_HEIGHT, game->getTeamColor(client->getTeamIndex()), client->getName().getString());
         y += TEXT_HEIGHT + 5;
      }
   }
}


void GameUserInterface::renderDebugStatus() const
{
   // When bots are frozen, render large pause icon in lower left
   if(EventManager::get()->isPaused())
   {
      const S32 PAUSE_HEIGHT = 30;
      const S32 PAUSE_WIDTH  = 10;
      const S32 PAUSE_GAP = 6;
      const S32 BOX_INSET = 5;

      const S32 TEXT_SIZE = 15;
      const char *TEXT = "STEP: Alt-], Ctrl-]";

      // Draw box
      S32 x = DisplayManager::getScreenInfo()->getGameCanvasWidth() - horizMargin - 2 * (PAUSE_WIDTH + PAUSE_GAP) - BOX_INSET - RenderUtils::getStringWidth(TEXT_SIZE, TEXT);
      S32 y = vertMargin + PAUSE_HEIGHT;

      // Draw Pause symbol
      RenderUtils::drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP;
      RenderUtils::drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP + BOX_INSET;

      y -= (PAUSE_HEIGHT - TEXT_SIZE) / 2 + 1;
      RenderUtils::drawString_fixed(x, y, TEXT_SIZE, Colors::white, TEXT);
   }
}


// Show server-side object ids... using illegal reachover to obtain them!
void GameUserInterface::renderObjectIds() const
{
   TNLAssert(getGame()->isTestServer(), "Will crash on non server!");
   if(getGame()->isTestServer())
      return;

   const Vector<DatabaseObject *> *objects = Game::getServerLevel()->findObjects_fast();

   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));
      static const S32 height = 13;

      // ForceFields don't have a geometry.  When I gave them one, they just rendered the ID at the
      // exact same location as their owning projector - so we'll just skip them
      if(obj->getObjectTypeNumber() == ForceFieldTypeNumber)
         continue;

      S32 id = obj->getUserAssignedId();
      S32 width = RenderUtils::getStringWidthf(height, "[%d]", id);

      F32 x = obj->getPos().x;
      F32 y = obj->getPos().y + height;

      RenderUtils::drawFilledRect(x - 1, y - height - 1, width + 2, height + 2, Colors::black);
      RenderUtils::drawStringf_fixed(x, y, height, Colors::gray70, "[%d]", id);
   }
}


//void GameUserInterface::saveAlreadySeenLevelupMessageList()
//{
//   mGameSettings->setSetting("LevelupItemsAlreadySeenList",
//                                                                getAlreadySeenLevelupMessageString());
//}


//void GameUserInterface::loadAlreadySeenLevelupMessageList()
//{
//   setAlreadySeenLevelupMessageString(
//         mGameSettings->getSetting<string>("LevelupItemsAlreadySeenList")
//   );
//}


const string GameUserInterface::getAlreadySeenLevelupMessageString() const
{
   return IniSettings::bitArrayToIniString(mAlreadySeenLevelupMsg, UserSettings::LevelCount);
}


// Takes a string; we'll mark a message as being seen every time we encounter a 'Y'
void GameUserInterface::setAlreadySeenLevelupMessageString(const string &vals)
{
   IniSettings::iniStringToBitArray(vals, mAlreadySeenLevelupMsg, UserSettings::LevelCount);
}


void GameUserInterface::onChatMessageReceived(const Color &msgColor, const char *format, ...)
{
   // Ignore empty message
   if(!strcmp(format, ""))
      return;

   static char buffer[MAX_CHAT_MSG_LENGTH];

   va_list args;

   va_start(args, format);
   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   mChatMessageDisplayer.onChatMessageReceived(msgColor, buffer);      // Our basic chat stream
}


// Set which chat message display mode we're in (Ctrl-M)
void GameUserInterface::toggleChatDisplayMode()
{
   mChatMessageDisplayer.toggleDisplayMode();
}


// Return message being composed in in-game chat
const char *GameUserInterface::getChatMessage() const
{
   return mHelperManager.getChatMessage();
}


HelpItemManager *GameUserInterface::getHelpItemManager()
{
   return &mHelpItemManager;
}


// Some reusable containers --> will probably need to become non-static if we have more than one clientGame active
static Point screenSize, visSize, visExt;
static Vector<DatabaseObject *> rawRenderObjects;
static Vector<BfObject *> renderObjects;
static Vector<BotNavMeshZone *> renderZones;


static void fillRenderZones()
{
   renderZones.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderZones.push_back(static_cast<BotNavMeshZone *>(rawRenderObjects[i]));
}


// Fills renderZones for drawing botNavMeshZones
static void populateRenderZones(ClientGame *game, const Rect *extentRect = NULL)
{
   rawRenderObjects.clear();

   if(extentRect)
      game->getBotZoneDatabase().findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects, *extentRect);
   else
      game->getBotZoneDatabase().findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects);

   fillRenderZones();
}


static void renderBotPaths(ClientGame *game, Vector<BfObject *> &renderObjects)
{
   ServerGame *serverGame = game->getServerGame();

   if(serverGame)
      for(S32 i = 0; i < serverGame->getBotCount(); i++)
         renderObjects.push_back(serverGame->getBot(i));
}


static bool renderSortCompare(BfObject * const &a, BfObject* const &b)
{
   return (a->getRenderSortValue() < b->getRenderSortValue());
}


// Note: With the exception of renderCommander, this function cannot be called if ship is NULL.  If it is never
// called with a NULL ship from renderCommander in practice, we can get rid of the caching of lastRenderPos (which will
// fail here if we ever have more than one UIGame instance.  If the following assert never trips, we can get rid of the 
// cached value, and perhaps the whole function itself.
Point GameUserInterface::getShipRenderPos() const
{
   static Point lastRenderPos;

   Ship *ship = getGame()->getLocalPlayerShip();

   //TNLAssert(ship, "Expected a valid ship here!");    // <== see comment above!  This has tripped, rarely, not sure why.

   if(ship)
      lastRenderPos = ship->getRenderPos();

   return lastRenderPos;
}


void GameUserInterface::renderGameNormal() const
{
   // Start of the level, we only show progress bar
   if(mShowProgressBar)
      return;

   // Here we determine if we have a control ship.
   // If not (like after we've been killed), we'll still render the current position and things
   Ship *ship = getGame()->getLocalPlayerShip();

   if(!ship)     // If we don't know where the ship is, we can't render in this mode
      return;

   visExt = getGame()->computePlayerVisArea(ship);

   mGL->pushMatrix();
   nvgSave(nvg);

   static const Point center(DisplayManager::getScreenInfo()->getGameCanvasWidth()  / 2,
                             DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2);

   mGL->glTranslate(center);       // Put (0,0) at the center of the screen
   nvgTranslate(nvg, center.x, center.y);

   // These scaling factors are different when changing the visible area by equiping the sensor module
   Point negShipPos = getShipRenderPos() * -1;
   mGL->glScale(center.x / visExt.x, center.y / visExt.y);
   mGL->glTranslate(negShipPos);
   nvgScale(nvg, center.x / visExt.x, center.y / visExt.y);
   nvgTranslate(nvg, negShipPos.x, negShipPos.y);

   GameObjectRender::renderStars(mStars, mStarColors, NumStars, 1.0, getShipRenderPos(), visExt * 2);

   // Render all the objects the player can see
   screenSize.set(visExt);
   Rect extentRect(getShipRenderPos() - screenSize, getShipRenderPos() + screenSize);

   // Fill rawRenderObjects with anything within extentRect (our visibility extent)
   rawRenderObjects.clear();
   getGame()->getLevel()->findObjects((TestFunc)isAnyObjectType, rawRenderObjects, extentRect);    

   // Cast objects in rawRenderObjects and put them in renderObjects
   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   // Normally a big no-no, we'll access the server's bot zones directly if we are running locally 
   // so we can visualize them without bogging the game down with the normal process of transmitting 
   // zones from server to client.  The result is that we can only see zones on our local server.
   if(mDebugShowMeshZones)
      populateRenderZones(getGame(), &extentRect);

   if(mShowDebugBots)
      renderBotPaths(getGame(), renderObjects);

   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 i = -1; i < 2; i++)
   {
      if(mDebugShowMeshZones)
         for(S32 j = 0; j < renderZones.size(); j++)
            renderZones[j]->renderLayer(i);

      for(S32 j = 0; j < renderObjects.size(); j++)
         renderObjects[j]->renderLayer(i);

      Barrier::renderEdges(mGameSettings, i);    // Render wall edges

      mFxManager.render(i, getCommanderZoomFraction(), getShipRenderPos());
   }

   S32 team = NONE;
   if(getGame()->getLocalRemoteClientInfo())
      team = getGame()->getLocalRemoteClientInfo()->getTeamIndex();
   renderInlineHelpItemOutlines(team, getBackgroundTextDimFactor(false));

   FxTrail::renderTrails();

   getUIManager()->getUI<GameUserInterface>()->renderEngineeredItemDeploymentMarker(ship);

   // Again, we'll be accessing the server's data directly so we can see server-side item ids directly on the client.  Again,
   // the result is that we can only see zones on our local server.
   if(mDebugShowObjectIds)
      renderObjectIds();

   mGL->popMatrix();
   nvgRestore(nvg);

   // Render current ship's energy
   if(ship)
   {
      EnergyGaugeRenderer::render(ship->mEnergy);   
      HealthGaugeRenderer::render(ship->mHealth);
   }

   // Render any screen-linked special effects, outside the matrix transformations
   mFxManager.renderScreenEffects();


   //renderOverlayMap();     // Draw a floating overlay map
}


void GameUserInterface::renderInlineHelpItemOutlines(S32 playerTeam, F32 alpha) const
{
   if(!HelpItemManager::shouldRender(getGame()))
      return;

   // Render a highlight/outline around any objects in our highlight type list, for help
   static Vector<const Vector<Point> *> polygons;
   polygons.clear();

   const Vector<HighlightItem> *itemsToHighlight = mHelpItemManager.getItemsToHighlight();      

   for(S32 i = 0; i < itemsToHighlight->size(); i++)
      for(S32 j = 0; j < renderObjects.size(); j++)
         if(itemsToHighlight->get(i).type == renderObjects[j]->getObjectTypeNumber() && 
                                             renderObjects[j]->shouldRender())
         {
            HighlightItem::Whose whose = itemsToHighlight->get(i).whose;

            S32 team = renderObjects[j]->getTeam();

            if( whose == HighlightItem::Any ||
               (whose == HighlightItem::Team && team == playerTeam) ||
               (whose == HighlightItem::TorNeut && (team == playerTeam || team == TEAM_NEUTRAL)) ||
               (whose == HighlightItem::Enemy && ((team >= 0 && team != playerTeam) || team == TEAM_HOSTILE)) ||
               (whose == HighlightItem::Neutral && team == TEAM_NEUTRAL) ||
               (whose == HighlightItem::Hostile && team == TEAM_HOSTILE) )

               polygons.push_back(renderObjects[j]->getOutline());
         }

#ifdef TNL_DEBUG
   if(getGame()->showAllObjectOutlines())
   {
      static Vector<U8> itemTypes;     // List of all items that are highlighted by our help system

      // Lazily initialize list
      if(itemTypes.size() == 0)
      {
#define HELP_TABLE_ITEM(a, itemType, c, d, e, f, g) \
         if(itemType != UnknownTypeNumber) \
            itemTypes.push_back(itemType);
         HELP_ITEM_TABLE
#undef HELP_TABLE_ITEM
      }

      fillVector.clear();
      getGame()->getLevel()->findObjects(itemTypes, fillVector, *getGame()->getWorldExtents());
      polygons.clear();
      for(S32 i = 0; i < fillVector.size(); i++)
         if(static_cast<BfObject *>(fillVector[i])->shouldRender())
            polygons.push_back(fillVector[i]->getOutline());
   }
#endif

   if(polygons.size() > 0)
   {
      Vector<Vector<Point> > outlines;

      offsetPolygons(polygons, outlines, HIGHLIGHTED_OBJECT_BUFFER_WIDTH);

      for(S32 j = 0; j < outlines.size(); j++)
         GameObjectRender::renderPolygonOutline(&outlines[j], Colors::green, alpha);
   }
}


void GameUserInterface::renderGameCommander() const
{
   // Start of the level, we only show progress bar
   if(mShowProgressBar)
      return;

   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();

   GameType *gameType = getGame()->getGameType();
   
   static Point worldExtents;    // Reuse this point to avoid construction/destruction cost
   worldExtents = mDispWorldExtents.getExtents();

   worldExtents.x *= canvasWidth  / F32(canvasWidth  - 2 * horizMargin);
   worldExtents.y *= canvasHeight / F32(canvasHeight - 2 * vertMargin);

   F32 aspectRatio = worldExtents.x / worldExtents.y;
   F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);

   if(aspectRatio > screenAspectRatio)
      worldExtents.y *= aspectRatio / screenAspectRatio;
   else
      worldExtents.x *= screenAspectRatio / aspectRatio;

   Ship *ship = getGame()->getLocalPlayerShip();

   visSize = ship ? getGame()->computePlayerVisArea(ship) * 2 : worldExtents;


   mGL->pushMatrix();
   nvgSave(nvg);

   // Put (0,0) at the center of the screen
   mGL->glTranslate(DisplayManager::getScreenInfo()->getGameCanvasWidth() * 0.5f,
               DisplayManager::getScreenInfo()->getGameCanvasHeight() * 0.5f);
   nvgTranslate(nvg, DisplayManager::getScreenInfo()->getGameCanvasWidth() * 0.5f,
               DisplayManager::getScreenInfo()->getGameCanvasHeight() * 0.5f);    

   F32 zoomFrac = getCommanderZoomFraction();

   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;
   mGL->glScale(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y);
   nvgScale(nvg, canvasWidth / modVisSize.x, canvasHeight / modVisSize.y);

   Point offset = (mDispWorldExtents.getCenter() - getShipRenderPos()) * zoomFrac + getShipRenderPos();
   mGL->glTranslate(-offset.x, -offset.y);
   nvgTranslate(nvg, -offset.x, -offset.y);

   // zoomFrac == 1.0 when fully zoomed out to cmdr's map
   GameObjectRender::renderStars(mStars, mStarColors, NumStars, 1 - zoomFrac, offset, modVisSize);

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects.  Note that this no longer captures
   // walls -- those will be rendered separately.
   rawRenderObjects.clear();

   if(ship && ship->hasModule(ModuleSensor))
      getGame()->getLevel()->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, rawRenderObjects);
   else
      getGame()->getLevel()->findObjects((TestFunc)isVisibleOnCmdrsMapType, rawRenderObjects);

   renderObjects.clear();

   // Copy rawRenderObjects into renderObjects
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   // Add extra bots if we're showing them
   if(mShowDebugBots)
      renderBotPaths(getGame(), renderObjects);

   // If we're drawing bot zones, get them now (put them in the renderZones vector)
   if(mDebugShowMeshZones)
      populateRenderZones(getGame());

   if(ship)
   {
      // Get info about the current player
      if(gameType)
      {
         S32 playerTeam = ship->getTeam();
         const Color &teamColor = ship->getColor();

         for(S32 i = 0; i < renderObjects.size(); i++)
         {
            // Render ship visibility range, and that of our teammates
            if(isShipType(renderObjects[i]->getObjectTypeNumber()))
            {
               Ship *otherShip = static_cast<Ship *>(renderObjects[i]);

               // Get team of this object
               S32 otherShipTeam = otherShip->getTeam();
               if((otherShipTeam == playerTeam && gameType->isTeamGame()) || otherShip == ship)  // On our team (in team game) || the ship is us
               {
                  Point p = otherShip->getRenderPos();
                  Point visExt = getGame()->computePlayerVisArea(otherShip);

                  RenderUtils::drawFilledRect(p.x - visExt.x, p.y - visExt.y, 2 * visExt.x, 2 * visExt.y, teamColor * zoomFrac * 0.35f);
               }
            }
         }

         const Vector<DatabaseObject *> *spyBugs = getGame()->getLevel()->findObjects_fast(SpyBugTypeNumber);

         // Render spy bug visibility range second, so ranges appear above ship scanner range
         for(S32 i = 0; i < spyBugs->size(); i++)
         {
            SpyBug *sb = static_cast<SpyBug *>(spyBugs->get(i));

            if(sb->isVisibleToPlayer(playerTeam, gameType->isTeamGame()))
            {
               GameObjectRender::renderSpyBugVisibleRange(sb->getRenderPos(), teamColor);
               
               // Draw a marker in the middle
               Color color = teamColor * 0.8f;
               RenderUtils::drawCircle(sb->getRenderPos(), 2, color);
            }
         }
      }
   }

   // Now render the objects themselves
   renderObjects.sort(renderSortCompare);

   if(mDebugShowMeshZones)
      for(S32 i = 0; i < renderZones.size(); i++)
         renderZones[i]->renderLayer(0);

   // First pass
   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->renderLayer(0);

   // Second pass
   Barrier::renderEdges(mGameSettings, 1);    // Render wall edges

   if(mDebugShowMeshZones)
      for(S32 i = 0; i < renderZones.size(); i++)
         renderZones[i]->renderLayer(1);

   for(S32 i = 0; i < renderObjects.size(); i++)
   {
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->renderLayer(1);
   }

   getUIManager()->getUI<GameUserInterface>()->renderEngineeredItemDeploymentMarker(ship);

   mGL->popMatrix();
   nvgRestore(nvg);


   // Render current ship's energy
   if(ship)
   {
      EnergyGaugeRenderer::render(ship->mEnergy);
      HealthGaugeRenderer::render(ship->mHealth);
   }

   // Render any screen-linked special effects, outside the matrix transformations
   mFxManager.renderScreenEffects();
}



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// This is a test of a partial map overlay to assist in navigation
// still needs work, and early indications are that it is not
// a beneficial addition to the game.

//void GameUserInterface::renderOverlayMap()
//{
//   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
//   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();
//
//   Ship *ship = getShip(game->getConnectionToServer());
//
//   Point position = ship->getRenderPos();
//
//   S32 mapWidth = canvasWidth / 4;
//   S32 mapHeight = canvasHeight / 4;
//   S32 mapX = UserInterface::horizMargin;        // This may need to the the UL corner, rather than the LL one
//   S32 mapY = canvasHeight - UserInterface::vertMargin - mapHeight;
//   F32 mapScale = 0.1f;
//
//   F32 vertices[] = {
//         mapX, mapY,
//         mapX, mapY + mapHeight,
//         mapX + mapWidth, mapY + mapHeight,
//         mapX + mapWidth, mapY
//   };
//   mGL->renderVertexArray(vertices, 4, GLOPT::LineLoop);
//
//
//   mGL->glEnable(GL_SCISSOR_BOX);                    // Crop to overlay map display area
//   mGL->glScissor(mapX, mapY + mapHeight, mapWidth, mapHeight);  // Set cropping window
//
//   mGL->glPushMatrix();   // Set scaling and positioning of the overlay
//
//   glTranslate(mapX + mapWidth / 2.f, mapY + mapHeight / 2.f);          // Move map off to the corner
//   glScale(mapScale);                                     // Scale map
//   glTranslate(-position.x, -position.y);                           // Put ship at the center of our overlay map area
//
//   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
//   Rect mapBounds(position, position);
//   mapBounds.expand(Point(mapWidth * 2, mapHeight * 2));      //TODO: Fix
//
//   rawRenderObjects.clear();
//   if(/*ship->isModulePrimaryActive(ModuleSensor)*/true)
//      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, rawRenderObjects);
//   else
//      mGameObjDatabase->findObjects((TestFunc)isVisibleOnCmdrsMapType, rawRenderObjects);
//
//   renderObjects.clear();
//   for(S32 i = 0; i < rawRenderObjects.size(); i++)
//      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));
//
//
//   renderObjects.sort(renderSortCompare);
//
//   for(S32 i = 0; i < renderObjects.size(); i++)
//      renderObjects[i]->render(0);
//
//   for(S32 i = 0; i < renderObjects.size(); i++)
//      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
// //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
//         renderObjects[i]->render(1);
//
//   mGL->glPopMatrix();
//   mGL->glDisable(GL_SCISSOR_BOX);     // Stop cropping
//}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void GameUserInterface::renderSuspended() const
{
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2 - 3 * (textHeight + textGap) + textHeight;

   RenderUtils::drawCenteredString_fixed(ypos, textHeight, Colors::yellow, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   RenderUtils::drawCenteredString_fixed(ypos, textHeight, Colors::yellow, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   RenderUtils::drawCenteredString_fixed(ypos, textHeight, Colors::yellow, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   RenderUtils::drawCenteredString_fixed(ypos, textHeight, Colors::yellow, "Press <SPACE> to resume playing now");
}


////////////////////////////////////////
////////////////////////////////////////

LevelListDisplayer::LevelListDisplayer()
{
   mLevelLoadDisplayFadeTimer.setPeriod(ONE_SECOND);
   mLevelLoadDisplay = true;
   mLevelLoadDisplayTotal = 0;
}


void LevelListDisplayer::idle(U32 timeDelta)
{
   if(mLevelLoadDisplayFadeTimer.update(timeDelta))
      clearLevelLoadDisplay();
}


// Shows the list of levels loaded when hosting a game
// If we want the list to fade out, pass true for fade, or pass false to make it disapear instantly
// fade param has no effect when show is true
void LevelListDisplayer::showLevelLoadDisplay(bool show, bool fade)
{
   mLevelLoadDisplay = show;

   if(!show)
   {
      if(fade)
         mLevelLoadDisplayFadeTimer.reset();
      else
         mLevelLoadDisplayFadeTimer.clear();
   }
}


void LevelListDisplayer::clearLevelLoadDisplay()
{
   mLevelLoadDisplayNames.clear();
   mLevelLoadDisplayTotal = 0;
}


void LevelListDisplayer::render() const
{
   if(mLevelLoadDisplay || mLevelLoadDisplayFadeTimer.getCurrent() > 0)
   {
      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         F32 alpha = (1.4f - ((F32)(mLevelLoadDisplayNames.size() - i) / 10.f)) * (mLevelLoadDisplay ? 1 : mLevelLoadDisplayFadeTimer.getFraction());

         FontManager::setFontColor(Colors::white, alpha);
         RenderUtils::drawStringf(100, DisplayManager::getScreenInfo()->getGameCanvasHeight() - (mLevelLoadDisplayNames.size() - i) * 20 ,
                                  15, "%s", mLevelLoadDisplayNames[i].c_str());
      }
   }
}


void LevelListDisplayer::addLevelName(const string &levelName)
{
   render();
   addProgressListItem("Loaded level " + levelName + "...");
}


// Add bit of text to progress item, and manage the list
void LevelListDisplayer::addProgressListItem(string item)
{
   static const S32 MaxItems = 15;

   mLevelLoadDisplayNames.push_back(item);

   mLevelLoadDisplayTotal++;

   // Keep the list from growing too long:
   if(mLevelLoadDisplayNames.size() > MaxItems)
      mLevelLoadDisplayNames.erase(0);
}


};

