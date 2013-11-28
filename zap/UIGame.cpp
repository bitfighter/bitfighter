//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Use this for testing the scoreboard
//#define USE_DUMMY_PLAYER_SCORES

#include "UIGame.h"

#include "UIMenus.h"
#include "UIInstructions.h"
#include "UIChat.h"
#include "UIManager.h"

#include "gameType.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "shipItems.h"           // For EngineerBuildObjects
#include "gameObjectRender.h"
#include "BotNavMeshZone.h"
#include "projectile.h"          // For SpyBug
#include "robot.h"

#include "EnergyGaugeRenderer.h"

#include "Console.h"             // Our console object
#include "ScreenInfo.h"
#include "ClientGame.h"
#include "Colors.h"
#include "Cursor.h"
#include "ScissorsManager.h"
#include "voiceCodec.h"
#include "SoundSystem.h"
#include "FontManager.h"
#include "Intervals.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"
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
GameUserInterface::GameUserInterface(ClientGame *game) : 
                  Parent(game), 
                  mVoiceRecorder(game),  //   lines expr  topdown   wrap width          font size          line gap
                  mServerMessageDisplayer(game,  6, true,  true,  SRV_MSG_WRAP_WIDTH, SRV_MSG_FONT_SIZE, SRV_MSG_FONT_GAP),
                  mChatMessageDisplayer1 (game,  5, true,  false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer2 (game,  5, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mChatMessageDisplayer3 (game, 24, false, false, CHAT_WRAP_WIDTH,    CHAT_FONT_SIZE,    CHAT_FONT_GAP),
                  mFpsRenderer(game),
                  mHelpItemManager(game->getSettings())
{
   mInScoreboardMode = false;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;

   mHelperManager.initialize(game);

   mMessageDisplayMode = ShortTimeout;

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
void GameUserInterface::onGameOver()         { mHelperManager.onGameOver();         }
void GameUserInterface::quitEngineerHelper() { mHelperManager.quitEngineerHelper(); }  // When ship dies engineering
void GameUserInterface::exitHelper()         { mHelperManager.exitHelper();         }


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


   // Clear out any lingering server or chat messages
   mServerMessageDisplayer.reset();
   mChatMessageDisplayer1.reset();
   mChatMessageDisplayer2.reset();
   mChatMessageDisplayer3.reset();

   // Clear out any walls we were using in a previous run
   Barrier::clearRenderItems();           // TODO: Should really go in an onDeactivate method, which we don't really have
   mLevelInfoDisplayer.clearDisplayTimer();

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
  
   mHelperManager.reset();

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i]   = false;
      mModSecondaryActivated[i] = false;
   }

   mShutdownMode = None;

   getGame()->onGameUIActivated();
}


void GameUserInterface::onReactivate()
{
   mDisableShipKeyboardInput = false;
   Cursor::disableCursor();    // Turn off cursor

   if(!isChatting())
      getGame()->setBusyChatting(false);

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      mModPrimaryActivated[i]   = false;
      mModSecondaryActivated[i] = false;
   }

   onMouseMoved();   // Call onMouseMoved to get ship pointed at current cursor location
}


// Called when level just beginning (called from GameConnection::onStartGhosting)
// We probably don't have a GameType yet, so we don't know what our level name will be
void GameUserInterface::onGameStarting()
{
   mDispWorldExtents.set(Point(0,0), 0);
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
   mInputModeChangeAlertDisplayTimer.update(timeDelta);
   mWrongModeMsgDisplay.update(timeDelta);
   mProgressBarFadeTimer.update(timeDelta);
   mCommanderZoomDelta.update(timeDelta);
   mLevelInfoDisplayer.idle(timeDelta);

   if(mAnnouncementTimer.update(timeDelta))
      mAnnouncement = "";

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      mModuleDoubleTapTimer[i].update(timeDelta);

   // Messages
   mServerMessageDisplayer.idle(timeDelta);
   mChatMessageDisplayer1.idle(timeDelta);
   mChatMessageDisplayer2.idle(timeDelta);
   mChatMessageDisplayer3.idle(timeDelta);

   mFpsRenderer.idle(timeDelta);
   
   mHelperManager.idle(timeDelta);
   mVoiceRecorder.idle(timeDelta);
   mLevelListDisplayer.idle(timeDelta);

   mLoadoutIndicator.idle(timeDelta);

   mFxManager.idle(timeDelta);      // Processes sparks and teleporter effects

   if(shouldCountdownHelpItemTimer())
      mHelpItemManager.idle(timeDelta, getGame());

   // Update mShipPos... track this so that we can keep a fix on the ship location even if it subsequently dies
   Ship *ship = getGame()->getLocalPlayerShip();

   if(ship)
      mShipPos.set(ship->getRenderPos());     // Get the player's ship position

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


void GameUserInterface::emitTextEffect(const string &text, const Color &color, const Point &pos)
{
   mFxManager.emitTextEffect(text, color, pos);
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
void GameUserInterface::render()
{
   if(!getGame()->isConnectedToServer())
   {
      glColor(Colors::white);
      drawCenteredString(260, 30, "Connecting to server...");

      glColor(Colors::green);
      if(getGame()->getConnectionToServer())
         drawCenteredString(310, 16, GameConnection::getConnectionStateString(getGame()->getConnectionToServer()->getConnectionState()));

      glColor(Colors::white);
      drawCenteredString(346, 20, "Press <ESC> to abort");

      return;
   }

   TNLAssert(getUIManager()->isCurrentUI<GameUserInterface>() || getUIManager()->cameFrom<GameUserInterface>(), "Then why are we rendering???");

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
   mHelpItemManager.renderMessages(getGame(), gScreenInfo.getGameCanvasHeight() / 2.0f + 40, helpItemAlpha);

   renderReticle();                       // Draw crosshairs if using mouse
   renderWrongModeIndicator();            // Try to avert confusion after player has changed btwn joystick and keyboard modes
   renderChatMsgs();                      // Render incoming chat and server msgs
   mLoadoutIndicator.render(getGame());   // Draw indicators for the various loadout items

   renderLevelListDisplayer();            // List of levels loaded while hosting
   renderProgressBar();                   // Status bar that shows progress of loading this level
   mVoiceRecorder.render();               // Indicator that someone is sending a voice msg

   mFpsRenderer.render(gScreenInfo.getGameCanvasWidth());     // Display running average FPS

   mHelperManager.render();

   GameType *gameType = getGame()->getGameType();

   if(gameType)
      gameType->renderInterfaceOverlay(gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());

   renderLevelInfo();

   renderLostConnectionMessage();      // Renders message overlay if we're losing our connection to the server
   
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
      drawStringf(10, 550, 30, "%0.2g, %0.2g", pos.x, pos.y);
   }
}

if(mGotControlUpdate)
   drawString(710, 10, 30, "CU");
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
   return mHelperManager.isHelperActive(HelperMenu::ChatHelperType);
}


void GameUserInterface::renderSuspendedMessage() const
{
   static string waitMsg[] = { "", 
                               "WILL RESPAWN",
                               "IN BLAH BLAH SECONDS",    // <== This line will be updated below
                               "",
                               "" };

   static string readyMsg[] = { "", 
                                "PRESS ANY",
                                "KEY TO",
                                "RESPAWN",
                                "" };

   static const S32 DisplayStyle = 2;
   static const S32 VertOffset = -30;

   if(getGame()->inReturnToGameCountdown())
   {
      waitMsg[2] = "IN " + ftos(ceil(F32(getGame()->getReturnToGameDelay()) / 1000.0f)) + " SECONDS";
      renderMessageBox("", "", waitMsg,  ARRAYSIZE(waitMsg),  VertOffset, DisplayStyle);
   }
   else
      renderMessageBox("", "", readyMsg, ARRAYSIZE(readyMsg), VertOffset, DisplayStyle);
}


void GameUserInterface::renderLevelUpMessage(S32 newLevel) const
{
   static string msg[] = { "", 
                           "CONGRATULATIONS!",
                           "YOU HAVE BEEN PROMOTED TO",
                           "LEVEL XXX",                     // <== This line will be updated below
                           "PRESS ANY KEY TO CONTINUE"
                           "" };

   static const S32 DisplayStyle = 2;
   static const S32 VertOffset = -30;

   msg[3] = "LEVEL " + itos(newLevel);
   renderMessageBox("", "", msg,  ARRAYSIZE(msg),  VertOffset, DisplayStyle);
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
      static string msg[] = { "", 
                              "We may have lost contact with the server...", 
                              "",
                              " You can't play until the connection has been re-established ", 
                              "",
                              "Trying to reconnect now..."};

      renderMessageBox("SERVER CONNECTION PROBLEMS", "", msg, 5, -30);
   }
}


void GameUserInterface::renderShutdownMessage() const
{
   if(mShutdownMode == None)
      return;

   else if(mShutdownMode == ShuttingDown)
   {
      char timemsg[255];
      dSprintf(timemsg, sizeof(timemsg), "Server is shutting down in %d seconds.", (S32) (mShutdownTimer.getCurrent() / 1000));

      if(mShutdownInitiator)     // Local client intitiated the shutdown
      {
         string msg[] = { "", timemsg, "", "Shutdown sequence intitated by you.", "", mShutdownReason.getString(), "" };
         renderMessageBox("SERVER SHUTDOWN INITIATED", "Press [ESC] to cancel shutdown", msg, 7);
      }
      else                       // Remote user intiated the shutdown
      {
         char whomsg[255];
         dSprintf(whomsg, sizeof(whomsg), "Shutdown sequence initiated by %s.", mShutdownName.getString());

         string msg[] = { "", timemsg, "", whomsg, "", mShutdownReason.getString(), "" };
         renderMessageBox("SHUTDOWN INITIATED", "Press [ESC] to dismiss", msg, 7);
      }
   }
   else if(mShutdownMode == Canceled)
   {
      // Keep same number of messages as above, so if message changes, it will be a smooth transition
      string msg[] = { "", "", "Server shutdown sequence canceled.", "", "Play on!", "", "" };     

      renderMessageBox("SHUTDOWN CANCELED", "Press [ESC] to dismiss", msg, 7);
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
      TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

      glColor(Colors::green, mShowProgressBar ? 1 : mProgressBarFadeTimer.getFraction());

      // Outline
      const F32 left = 200;
      const F32 width = gScreenInfo.getGameCanvasWidth() - 2 * left;
      const F32 height = 10;

      // For some reason, there are occasions where the status bar doesn't progress all the way over during the load process.
      // The problem is that, for some reason, some objects do not add themselves to the loaded object counter, and this creates
      // a disconcerting effect, as if the level did not fully load.  Rather than waste any more time on this problem, we'll just
      // fill in the status bar while it's fading, to make it look like the level fully loaded.  Since the only thing that this
      // whole mechanism is used for is to display something to the user, this should work fine.
      F32 barWidth = mShowProgressBar ? S32((F32) width * (F32) getGame()->mObjectsLoaded / (F32) gt->mObjectsExpected) : width;

      for(S32 i = 1; i >= 0; i--)
      {
         F32 w = i ? width : barWidth;

         F32 vertices[] = {
               left,     F32(gScreenInfo.getGameCanvasHeight() - vertMargin),
               left + w, F32(gScreenInfo.getGameCanvasHeight() - vertMargin),
               left + w, F32(gScreenInfo.getGameCanvasHeight() - vertMargin - height),
               left,     F32(gScreenInfo.getGameCanvasHeight() - vertMargin - height)
         };
         renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
      }
   }
}


// Draw the reticle (i.e. the mouse cursor) if we are using keyboard/mouse
void GameUserInterface::renderReticle() const
{
   bool shouldRender = getGame()->getInputMode() == InputModeKeyboard &&   // Reticle in keyboard mode only
                       getUIManager()->isCurrentUI<GameUserInterface>();   // And not when a menu is active
   if(shouldRender)
   {
      Point offsetMouse = mMousePoint + Point(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);

      F32 vertices[] = {
            // Center cross-hairs
            offsetMouse.x - 15, offsetMouse.y,
            offsetMouse.x + 15, offsetMouse.y,
            offsetMouse.x, offsetMouse.y - 15,
            offsetMouse.x, offsetMouse.y + 15,

            // Large axes lines
            0, offsetMouse.y,
            offsetMouse.x - 30, offsetMouse.y,

            offsetMouse.x + 30, offsetMouse.y,
            (F32)gScreenInfo.getGameCanvasWidth(), offsetMouse.y,

            offsetMouse.x, 0,
            offsetMouse.x, offsetMouse.y - 30,

            offsetMouse.x, offsetMouse.y + 30,
            offsetMouse.x, (F32)gScreenInfo.getGameCanvasHeight(),
      };

#define RETICLE_COLOR Colors::green
#define COLOR_RGB RETICLE_COLOR.r, RETICLE_COLOR.g, RETICLE_COLOR.b      

      static F32 colors[] = {
      //    R,G,B   aplha
         COLOR_RGB, 0.7f,
         COLOR_RGB, 0.7f,
         COLOR_RGB, 0.7f,
         COLOR_RGB, 0.7f,

         COLOR_RGB, 0.0f,
         COLOR_RGB, 0.7f,

         COLOR_RGB, 0.7f,
         COLOR_RGB, 0.0f,

         COLOR_RGB, 0.0f,
         COLOR_RGB, 0.7f,

         COLOR_RGB, 0.7f,
         COLOR_RGB, 0.0f,
      };

#undef COLOR_RGB
#undef RETICLE_COLOR

      renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_LINES);
   }
}


void GameUserInterface::renderWrongModeIndicator() const
{
   if(mWrongModeMsgDisplay.getCurrent())
   {
      // Fade for last half second
      F32 alpha = mWrongModeMsgDisplay.getCurrent() < 500 ? mWrongModeMsgDisplay.getCurrent() / 500.0f : 1.0f;

      glColor(Colors::red, alpha);
      FontManager::pushFontContext(HelperMenuContext);
      drawCenteredString(225, 20, "You are in joystick mode.");
      drawCenteredString(250, 20, "You can change to Keyboard input with the Options menu.");
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

   mMousePoint.set(gScreenInfo.getMousePos()->x - gScreenInfo.getGameCanvasWidth()  / 2,
                   gScreenInfo.getMousePos()->y - gScreenInfo.getGameCanvasHeight() / 2);

   if(mInCommanderMap)     // Ship not in center of the screen in cmdrs map.  Where is it?
   {
      Ship *ship = getGame()->getLocalPlayerShip();

      if(!ship)
         return;

      Point o = ship->getRenderPos();  // To avoid taking address of temporary
      Point p = worldToScreenPoint(&o, gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());

      mCurrentMove.angle = atan2(mMousePoint.y + gScreenInfo.getGameCanvasHeight() / 2 - p.y, 
                                 mMousePoint.x + gScreenInfo.getGameCanvasWidth()  / 2 - p.x);
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


void GameUserInterface::renderEngineeredItemDeploymentMarker(Ship *ship)
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
      displayMessage(Colors::red, "You don't have any items to drop!");
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
   return mInScoreboardMode || (getGame()->getGameType() && getGame()->getGameType()->isGameOver());
}


Point GameUserInterface::getTimeLeftIndicatorWidthAndHeight() const
{
   return mTimeLeftRenderer.render(getGame()->getGameType(), scoreboardIsVisible(), false);
}


// Key pressed --> take action!
// Handles all keypress events, including mouse clicks and controller button presses
bool GameUserInterface::onKeyDown(InputCode inputCode)
{
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
         if(!checkInputCode(InputCodeManager::BINDING_OUTGAMECHAT, inputCode)  &&
            !checkInputCode(InputCodeManager::BINDING_GLOBCHAT,    inputCode)  &&
            !checkInputCode(InputCodeManager::BINDING_TEAMCHAT,    inputCode)  &&
            !checkInputCode(InputCodeManager::BINDING_CMDCHAT,     inputCode)  &&
            !checkInputCode(InputCodeManager::BINDING_SCRBRD,      inputCode))
         {
            getGame()->undelaySpawn();
            if(inputCode != KEY_ESCAPE)  // Don't swollow escape: Lagged out and can't un-idle to bring up the menu?
               return true;
         }
      }
   }

   if(checkInputCode(InputCodeManager::BINDING_OUTGAMECHAT, inputCode))
      getGame()->setBusyChatting(true);

   if(Parent::onKeyDown(inputCode))    // Let parent try handling the key
      return true;

#ifndef BF_NO_CONSOLE
   if(gConsole.onKeyDown(inputCode))   // Pass the key on to the console for processing
      return true;
#endif

   if(checkInputCode(InputCodeManager::BINDING_HELP, inputCode))   // Turn on help screen
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
#ifndef BF_NO_CONSOLE
      if(gConsole.isOk())                 // Console is only not Ok if something bad has happened somewhere
         gConsole.toggleVisibility();
#endif

      return true;
   }

   if(checkInputCode(InputCodeManager::BINDING_MISSION, inputCode))    // F2
   {
      onMissionKeyPressed();

      return true;
   }

   if(inputCode == KEY_M && InputCodeManager::checkModifier(KEY_CTRL))        // Ctrl+M, for now, to cycle through message dispaly modes
   {
      toggleChatDisplayMode();
      return true;
   }

   if(mHelperManager.isHelperActive() && mHelperManager.processInputCode(inputCode))   // Will return true if key was processed
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
         if((checkInputCode(InputCodeManager::BINDING_MOD1, inputCode) && ship->getModule(0) == ModuleEngineer) ||
            (checkInputCode(InputCodeManager::BINDING_MOD2, inputCode) && ship->getModule(1) == ModuleEngineer))
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
   if(inputCode == KEY_H && InputCodeManager::checkModifier(KEY_CTRL))     // Ctrl+H to show next HelpItem
      mHelpItemManager.debugShowNextHelpItem();
#endif

#ifndef BF_NO_CONSOLE
   if(!gConsole.isVisible())
#endif
   {
      if(!isChatting())
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
#ifndef BF_NO_CONSOLE
   if(gConsole.isVisible())
      gConsole.onKeyDown(ascii);
#endif
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

   GameType *gameType = game->getGameType();
   if(!gameType)
      return;
   

   GameConnection *conn = game->getConnectionToServer();
   if(!conn)
      return;

   if(game->getSettings()->getIniSettings()->mSettings.getVal<YesNo>("VerboseHelpMessages"))
      game->displayShipDesignChangedMessage(loadout, "Preset same as the current design");

   // Request loadout even if it was the same -- if I have loadout A, with on-deck loadout B, and I enter a new loadout
   // that matches A, it would be better to have loadout remain unchanged if I entered a loadout zone.
   // Tell server loadout has changed.  Server will activate it when we enter a loadout zone.
   conn->c2sRequestLoadout(loadout.toU8Vector());    
}


bool checkInputCode(InputCode codeUserEntered, InputCode codeToActivateCommand)
{
   return codeUserEntered == codeToActivateCommand;
}


// Helper function -- checks input keys and sees if we should start chatting.  Returns true if entered chat mode, false if not.
bool GameUserInterface::checkEnterChatInputCode(InputCode inputCode)
{
   if(checkInputCode(InputCodeManager::BINDING_TEAMCHAT, inputCode))          // Start entering a team chat msg
      mHelperManager.activateHelper(ChatHelper::TeamChat);
   else if(checkInputCode(InputCodeManager::BINDING_GLOBCHAT, inputCode))     // Start entering a global chat msg
      mHelperManager.activateHelper(ChatHelper::GlobalChat);
   else if(checkInputCode(InputCodeManager::BINDING_CMDCHAT, inputCode))      // Start entering a command
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

   else if(checkInputCode(InputCodeManager::BINDING_LOAD_PRESET_1, inputCode))  // Loading loadout presets
      loadLoadoutPreset(getGame(), 0);
   else if(checkInputCode(InputCodeManager::BINDING_LOAD_PRESET_2, inputCode))
      loadLoadoutPreset(getGame(), 1);
   else if(checkInputCode(InputCodeManager::BINDING_LOAD_PRESET_3, inputCode))
      loadLoadoutPreset(getGame(), 2);

   else if(checkInputCode(InputCodeManager::BINDING_SAVE_PRESET_1, inputCode))  // Saving loadout presets
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 0);
   else if(checkInputCode(InputCodeManager::BINDING_SAVE_PRESET_2, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 1);
   else if(checkInputCode(InputCodeManager::BINDING_SAVE_PRESET_3, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 2);

   else if(checkInputCode(InputCodeManager::BINDING_MOD1, inputCode))       
      activateModule(0);
   else if(checkInputCode(InputCodeManager::BINDING_MOD2, inputCode))       
      activateModule(1);
   else if(checkInputCode(InputCodeManager::BINDING_FIRE, inputCode))
   {
      mFiring = true;
      mHelpItemManager.removeInlineHelpItem(ControlsKBItem, true, 0xFF - 1);     // Player has demonstrated knowledge of how to fire
   }
   else if(checkInputCode(InputCodeManager::BINDING_SELWEAP1, inputCode))
      selectWeapon(0);
   else if(checkInputCode(InputCodeManager::BINDING_SELWEAP2, inputCode))
      selectWeapon(1);
   else if(checkInputCode(InputCodeManager::BINDING_SELWEAP3, inputCode))
      selectWeapon(2);
   else if(checkInputCode(InputCodeManager::BINDING_FPS, inputCode))
      mFpsRenderer.toggleVisibility();
   else if(checkInputCode(InputCodeManager::BINDING_ADVWEAP, inputCode))
      chooseNextWeapon();

   // By default, Handle mouse wheel. Users can change it in "Define Keys" option
   else if(checkInputCode(InputCodeManager::BINDING_ADVWEAP2, inputCode))
      chooseNextWeapon();
   else if(checkInputCode(InputCodeManager::BINDING_PREVWEAP, inputCode))
      choosePrevWeapon();
   else if(checkInputCode(InputCodeManager::BINDING_TOGGLE_RATING, inputCode))
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
         getUIManager()->activate<MainMenuUserInterface>();      // Back to main menu
      }
      else
      {
         getGame()->setBusyChatting(true);
         getUIManager()->activate<GameMenuUserInterface>();
      }
   }     
   else if(checkInputCode(InputCodeManager::BINDING_CMDRMAP, inputCode))
   {
      toggleCommanderMap();
      
      // Now that we've demonstrated use of cmdrs map, no need to tell player about it
      mHelpItemManager.removeInlineHelpItem(CmdrsMapItem, true);  
   }

   else if(checkInputCode(InputCodeManager::BINDING_SCRBRD, inputCode))
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
   else if(checkInputCode(InputCodeManager::BINDING_TOGVOICE, inputCode))
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
         if(checkInputCode(InputCodeManager::BINDING_QUICKCHAT, inputCode))
            activateHelper(HelperMenu::QuickChatHelperType);
         else if(checkInputCode(InputCodeManager::BINDING_LOADOUT, inputCode))
            activateHelper(HelperMenu::LoadoutHelperType);
         else if(checkInputCode(InputCodeManager::BINDING_DROPITEM, inputCode))
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
   if(checkInputCode(InputCodeManager::BINDING_UP,    inputCode) ||
      checkInputCode(InputCodeManager::BINDING_DOWN,  inputCode) ||
      checkInputCode(InputCodeManager::BINDING_LEFT,  inputCode) ||
      checkInputCode(InputCodeManager::BINDING_RIGHT, inputCode))
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


// Display proper chat queue based on mMessageDisplayMode.  These displayers are configured in the constructor. 
void GameUserInterface::renderChatMsgs() const
{
   bool chatDisabled = !mHelperManager.isChatAllowed();
   bool announcementActive = (mAnnouncementTimer.getCurrent() != 0);

   F32 alpha = 1; // getBackgroundTextDimFactor(true);

   if(mMessageDisplayMode == ShortTimeout)
      mChatMessageDisplayer1.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);
   else if(mMessageDisplayMode == ShortFixed)
      mChatMessageDisplayer2.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);
   else
      mChatMessageDisplayer3.render(IN_GAME_CHAT_DISPLAY_POS, chatDisabled, announcementActive, alpha);

   mServerMessageDisplayer.render(messageMargin, chatDisabled, false, alpha);

   if(announcementActive)
      renderAnnouncement(IN_GAME_CHAT_DISPLAY_POS);
}


void GameUserInterface::renderAnnouncement(S32 pos) const
{
   glColor(Colors::red);
   glLineWidth(gLineWidth4);

   S32 x = drawStringAndGetWidth(UserInterface::horizMargin, pos, 16, "*** ");
   x += drawStringAndGetWidth(UserInterface::horizMargin + x, pos, 16, mAnnouncement.c_str());
   drawString(UserInterface::horizMargin + x, pos, 16, " ***");

   glLineWidth(gDefaultLineWidth);
}


void GameUserInterface::onKeyUp(InputCode inputCode)
{
   // These keys works in any mode!  And why not??

   if(checkInputCode(InputCodeManager::BINDING_MISSION, inputCode))    // F2
      onMissionKeyReleased();

   else if(checkInputCode(InputCodeManager::BINDING_MOD1, inputCode))
   {
      mModPrimaryActivated[0] = false;
      mModSecondaryActivated[0] = false;

      if(getGame()->getLocalPlayerShip())    // Sometimes false if in hit any key to continue mode
         setModulePrimary(getGame()->getLocalPlayerShip()->getModule(0), false);
   }
   else if(checkInputCode(InputCodeManager::BINDING_MOD2, inputCode))
   {
      mModPrimaryActivated[1] = false;
      mModSecondaryActivated[1] = false;

      if(getGame()->getLocalPlayerShip())    // Sometimes false if in hit any key to continue mode
         setModulePrimary(getGame()->getLocalPlayerShip()->getModule(1), false);
   }
   else if(checkInputCode(InputCodeManager::BINDING_FIRE, inputCode))
      mFiring = false;
   else if(checkInputCode(InputCodeManager::BINDING_SCRBRD, inputCode))
   {     // (braces required)
      if(mInScoreboardMode)     // We're turning scoreboard off
      {
         mInScoreboardMode = false;
         GameType *gameType = getGame()->getGameType();
         if(gameType)
            gameType->c2sRequestScoreboardUpdates(false);
      }
   }
   else if(checkInputCode(InputCodeManager::BINDING_TOGVOICE, inputCode))
   {     // (braces required)
      if(mVoiceRecorder.mRecordingAudio)  // Turning recorder off
         mVoiceRecorder.stop();
   }
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
   // One of each of left/right axis and up/down axis should be 0 by this point
   // but let's guarantee it..   why?
   theMove->x = game->mJoystickInputs[JoystickMoveAxesRight] - 
                game->mJoystickInputs[JoystickMoveAxesLeft];
   theMove->x = MAX(theMove->x, -1);
   theMove->x = MIN(theMove->x, 1);
   theMove->y =  game->mJoystickInputs[JoystickMoveAxesDown] - 
                 game->mJoystickInputs[JoystickMoveAxesUp];
   theMove->y = MAX(theMove->y, -1);
   theMove->y = MIN(theMove->y, 1);

   //logprintf(
   //      "Joystick axis values. Move: Left: %f, Right: %f, Up: %f, Down: %f\nShoot: Left: %f, Right: %f, Up: %f, Down: %f ",
   //      mJoystickInputs[MoveAxesLeft],  mJoystickInputs[MoveAxesRight],
   //      mJoystickInputs[MoveAxesUp],    mJoystickInputs[MoveAxesDown],
   //      mJoystickInputs[ShootAxesLeft], mJoystickInputs[ShootAxesRight],
   //      mJoystickInputs[ShootAxesUp],   mJoystickInputs[ShootAxesDown]
   //      );

   //logprintf(
   //         "Move values. Move: Left: %f, Right: %f, Up: %f, Down: %f",
   //         theMove->left, theMove->right,
   //         theMove->up, theMove->down
   //         );


   //logprintf("XY from shoot axes. x: %f, y: %f", x, y);


   Point p(game->mJoystickInputs[JoystickShootAxesRight] - 
           game->mJoystickInputs[JoystickShootAxesLeft], 
                             game->mJoystickInputs[JoystickShootAxesDown]  - 
                             game->mJoystickInputs[JoystickShootAxesUp]);

   F32 fact =  p.len();

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

#ifndef BF_NO_CONSOLE
   if(!mDisableShipKeyboardInput && !gConsole.isVisible())
#else
   if(!mDisableShipKeyboardInput)
#endif
   {
      // Some helpers (like TeamShuffle) like to disable movement when they are active
      if(mHelperManager.isMovementDisabled())
      {
         mCurrentMove.x = 0;
         mCurrentMove.y = 0;
      }
      else
      {
         GameSettings *settings = getGame()->getSettings();

         mCurrentMove.x = F32((InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_RIGHT)) ? 1 : 0) - 
                              (InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_LEFT))  ? 1 : 0));

         mCurrentMove.y = F32((InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_DOWN))  ? 1 : 0) -  
                              (InputCodeManager::getState(getInputCode(settings, InputCodeManager::BINDING_UP))    ? 1 : 0));
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
   if(getGame()->getSettings()->getIniSettings()->mSettings.getVal<RelAbs>("ControlMode") == Relative)
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
      joystickUpdateMove(getGame(), getGame()->getSettings(), move);

   return move;
}


void GameUserInterface::resetLevelInfoDisplayTimer()
{
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
      F32 amt = mMaxAudioSample / F32(0x7FFF);
      U32 totalLineCount = 50;

      // Render low/high volume lines
      glColor(1, 1 ,1);
      F32 vertices[] = {
            10.0f,                        130.0f,
            10.0f,                        145.0f,
            F32(10 + totalLineCount * 2), 130.0f,
            F32(10 + totalLineCount * 2), 145.0f
      };
      renderVertexArray(vertices, ARRAYSIZE(vertices)/2, GL_LINES);

      F32 halfway = totalLineCount * 0.5f;
      F32 full = amt * totalLineCount;

      // Total items possible is totalLineCount (50)
      static F32 colorArray[400];   // 2 * 4 color components per item
      static F32 vertexArray[200];  // 2 * 2 vertex components per item

      // Render recording volume
      for(U32 i = 1; i < full; i++)  // start at 1 to not show
      {
         if(i < halfway)
         {
            colorArray[8*(i-1)]     = i / halfway;
            colorArray[(8*(i-1))+1] = 1;
            colorArray[(8*(i-1))+2] = 0;
            colorArray[(8*(i-1))+3] = 1;
            colorArray[(8*(i-1))+4] = i / halfway;
            colorArray[(8*(i-1))+5] = 1;
            colorArray[(8*(i-1))+6] = 0;
            colorArray[(8*(i-1))+7] = 1;
         }
         else
         {
            colorArray[8*(i-1)]     = 1;
            colorArray[(8*(i-1))+1] = 1 - (i - halfway) / halfway;
            colorArray[(8*(i-1))+2] = 0;
            colorArray[(8*(i-1))+3] = 1;
            colorArray[(8*(i-1))+4] = 1;
            colorArray[(8*(i-1))+5] = 1 - (i - halfway) / halfway;
            colorArray[(8*(i-1))+6] = 0;
            colorArray[(8*(i-1))+7] = 1;
         }

         vertexArray[4*(i-1)]     = F32(10 + i * 2);
         vertexArray[(4*(i-1))+1] = F32(130);
         vertexArray[(4*(i-1))+2] = F32(10 + i * 2);
         vertexArray[(4*(i-1))+3] = F32(145);
      }

      renderColorVertexArray(vertexArray, colorArray, S32(full*2), GL_LINES);
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
         gameType->c2sVoiceChat(mGame->getSettings()->getIniSettings()->mSettings.getVal<YesNo>("VoiceEcho"), sendBuffer);
   }
}


#ifdef USE_DUMMY_PLAYER_SCORES

S32 getDummyTeamCount() { return 2; }     // Teams
S32 getDummyMaxPlayers() { return 5; }    // Players per team

// Create a set of fake player scores for testing the scoreboard -- fill scores
void getDummyPlayerScores(ClientGame *game, Vector<ClientInfo *> &scores)
{
   ClientInfo *clientInfo;

   S32 teams = getDummyTeamCount();

   for(S32 i = 0; i < getDummyMaxPlayers(); i++)
   {
      string name = "PlayerName-" + itos(i);

      clientInfo = new RemoteClientInfo(game, name, false, 0, ((i+1) % 4) > 0, i, i % 3, ClientInfo::ClientRole(i % 4), false, false);

      clientInfo->setScore(i * 3);
      clientInfo->setAuthenticated((i % 2), 0, (i % 3) > 0);
      clientInfo->setPing(100 * i + 10);
      clientInfo->setTeamIndex(i % teams);

      scores.push_back(clientInfo);
   }
}
#endif


static const char *botSymbol = "B";
static const char *levelChangerSymbol = "+";
static const char *adminSymbol = "@";

static void renderScoreboardLegend(S32 humans, U32 scoreboardTop, U32 totalHeight)
{
   const S32 LegendSize = 12;     
   const S32 LegendGap  =  3;    // Space between scoreboard and legend
   const S32 legendPos  = scoreboardTop + totalHeight + LegendGap + LegendSize;

   // Create a standard legend; only need to swap out the Humans count, which is the first chunk -- this should work even if
   // there are multiple players running in the same session -- the humans count should be the same regardless!
   static Vector<SymbolShapePtr> symbols;
   static S32 lastHumans = S32_MIN;
   if(symbols.size() == 0)
   {
      string legend = " | " + string(adminSymbol) + " = Admin | " + 
                      levelChangerSymbol + " = Can Change Levels | " + botSymbol + " = Bot |";

      symbols.push_back(SymbolShapePtr());    // Placeholder, will be replaced with humans count below
      symbols.push_back(SymbolShapePtr(new SymbolText(legend, LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" Idle Player", LegendSize, ScoreboardContext, &Colors::idlePlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText(" | ", LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor)));
      symbols.push_back(SymbolShapePtr(new SymbolText("Player on Rampage", LegendSize, ScoreboardContext, &Colors::streakPlayerNameColor)));
   }

   // Rebuild the humans symbol, if the number of humans has changed
   if(humans != lastHumans)
   {
      const string humanStr = itos(humans) + " Human" + (humans != 1 ? "s" : "");
      symbols[0] = SymbolShapePtr(new SymbolText(humanStr, LegendSize, ScoreboardContext, &Colors::standardPlayerNameColor));
      lastHumans = humans;
   }

   UI::SymbolString symbolString(symbols);
   symbolString.render(gScreenInfo.getGameCanvasWidth() / 2, legendPos, AlignmentCenter);
}


static void renderPlayerSymbolAndSetColor(ClientInfo *player, S32 x, S32 y, S32 size)
{
   // Figure out what color to use to render player name, and set it
   if(player->isSpawnDelayed())
      glColor(Colors::idlePlayerNameColor);
   else if(player->getKillStreak() >= UserInterface::StreakingThreshold)
      glColor(Colors::streakPlayerNameColor);
   else
      glColor(Colors::standardPlayerNameColor);


   // Figure out how much room we need to leave for our player symbol (@, +, etc.)
   x -= getStringWidth(size, adminSymbol) + 3;  // Use admin symbol as it's the widest; 3 provides a bit of whitespace

   // Mark of the bot
   if(player->isRobot())
      drawString(x, y, size, botSymbol);

   // Admin mark
   else if(player->isAdmin())
      drawString(x, y, size, adminSymbol);

   // Level changer mark
   else if(player->isLevelChanger())
      drawString(x, y, size, levelChangerSymbol);
}


void GameUserInterface::renderScoreboard()
{
   // This is probably not needed... if gameType were NULL, we'd have crashed and burned long ago
   GameType *gameType = getGame()->getGameType();
   TNLAssert(gameType, "This assert added 8/20/2013; can probably be removed as gameType will never be NULL here!");
   if(!gameType)
      return;

   const bool isTeamGame = gameType->isTeamGame();

#ifdef USE_DUMMY_PLAYER_SCORES
   S32 maxTeamPlayers = getDummyMaxPlayers();
   S32 teams = isTeamGame ? getDummyTeamCount() : 1;
#else
   getGame()->countTeamPlayers();

   const S32 teams = isTeamGame ? getGame()->getTeamCount() : 1;
   S32 maxTeamPlayers = 0;

   // Check to make sure at least one team has at least one player...
   for(S32 i = 0; i < teams; i++)
   {
      Team *team = (Team *)getGame()->getTeam(i);

      if(!isTeamGame)
         maxTeamPlayers += team->getPlayerBotCount();

      else if(team->getPlayerBotCount() > maxTeamPlayers)
         maxTeamPlayers = team->getPlayerBotCount();
   }
#endif
   // ...if not, then go home!
   if(maxTeamPlayers == 0)
      return;

   static const U32 gap = 3;  // Small gap for use between various UI elements

   const U32 canvasHeight = gScreenInfo.getGameCanvasHeight();
   const U32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   const U32 drawableWidth = canvasWidth - horizMargin * 2;
   const U32 columnCount = min(teams, 2);
   const U32 teamWidth = drawableWidth / columnCount;
   const U32 teamHeaderHeight = isTeamGame ? 40 : 2;

   const U32 numTeamRows = (teams + 1) >> 1;

   static const U32 ColHeaderTextSize = 10;

   const U32 colHeaderHeight = isTeamGame ? ColHeaderTextSize - 3: ColHeaderTextSize + 2;

   const U32 desiredHeight = (canvasHeight - vertMargin * 2) / numTeamRows;
   const U32 maxHeight     = MIN(30, (desiredHeight - teamHeaderHeight) / maxTeamPlayers);

   const U32 sectionHeight = teamHeaderHeight + (maxHeight * maxTeamPlayers) + (2 * gap) + 10;
   const U32 totalHeight   = sectionHeight * numTeamRows - 10  + (isTeamGame ? 0 : 4);    // 4 provides a gap btwn bottom name and legend

   const U32 scoreboardTop = (canvasHeight - totalHeight) / 2;

   // Vertical scale ratio to maximum line height
   const F32 scaleRatio = ((F32)maxHeight) / 30.f;

   const S32 playerFontSize = S32(maxHeight * 0.75f);
   const S32 teamFontSize = 24;
   const S32 symbolFontSize = S32(playerFontSize * 0.75f);

   // Outer scoreboard box
   drawFilledFancyBox(horizMargin - gap, scoreboardTop - (2 * gap),
                     (canvasWidth - horizMargin) + gap, scoreboardTop + totalHeight + 23,
                     13, Colors::black, 0.85f, Colors::blue);

   FontManager::pushFontContext(ScoreboardContext);

   for(S32 i = 0; i < teams; i++)
   {
      const S32 yt = scoreboardTop + (i >> 1) * sectionHeight;    // Top edge of team render area
      const S32 xl = horizMargin + gap + (i & 1) * teamWidth;     // Left edge of team render area
      const S32 xr = (xl + teamWidth) - (2 * gap);                // Right edge of team render area

      // Team header
      if(isTeamGame)     
      {
         // First the box
         const Color *teamColor = getGame()->getTeamColor(i);
         const S32 headerBoxHeight = teamFontSize + 2 * gap;
         drawFilledFancyBox(xl, yt, xr, yt + headerBoxHeight, 10, *teamColor, 0.6f, *teamColor);

         // Then the team name & score
         FontManager::pushFontContext(ScoreboardHeadlineContext);
         glColor(Colors::white);
         drawString (xl + 40,  yt + 2, teamFontSize, getGame()->getTeamName(i).getString());
         drawStringf(xr - 140, yt + 2, teamFontSize, "%d", ((Team *)(getGame()->getTeam(i)))->getScore());
         FontManager::popFontContext();
      }

      // Now for player scores.  First build a list.  Then sort it.  Then display it.
      Vector<ClientInfo *> playerScores;

#ifdef USE_DUMMY_PLAYER_SCORES      // For testing purposes only!
      getDummyPlayerScores(getGame(), playerScores);
#else
      gameType->getSortedPlayerScores(i, playerScores);     // Fills playerScores for team i
#endif

      S32 curRowY = yt + teamHeaderHeight + 1;              // Advance y coord to below team display, if there is one

      const S32 x = xl + 40;     // + 40 to align with team name in team game
      const S32 colHeaderYPos = isTeamGame ? curRowY + 3 : curRowY + 8;

      S32 maxscorelen = -1;
      S32 maxkdlen    = -1;
      S32 maxpinglen  = -1;

      // Horiz offsets from the right for rendering score components
      static const S32 ScoreOff = 160;    // Solo game only
      static const S32 KdOff   = 85;
      static const S32 PingOff = 60;

      // Leave a gap for the colHeader... not sure yet of the exact xpos... will figure that out and render in this slot later
      if(playerScores.size() > 0)
         curRowY += colHeaderHeight;

      FontManager::pushFontContext(ScoreboardContext);
      for(S32 j = 0; j < playerScores.size(); j++)
      {
         static const S32 vertAdjustFact = (playerFontSize - symbolFontSize) / 2 - 1;

         renderPlayerSymbolAndSetColor(playerScores[j], x, curRowY + vertAdjustFact + 2, symbolFontSize);

         S32 nameWidth = drawStringAndGetWidth(x, curRowY, playerFontSize, playerScores[j]->getName().getString());

         S32 kdlen = drawStringfr(xr - KdOff, curRowY, playerFontSize, "%2.2f", playerScores[j]->getRating());
         maxkdlen = max(kdlen, maxkdlen);

         S32 pinglen = drawStringAndGetWidthf(xr - PingOff, curRowY, playerFontSize, "%d", playerScores[j]->getPing());
         maxpinglen = max(pinglen, maxpinglen);

         if(!isTeamGame)
         {            
            S32 scorelen = drawStringfr(xr - ScoreOff, curRowY, playerFontSize, "%d", playerScores[j]->getScore());
            maxscorelen = max(scorelen, maxscorelen);
         }

         // Circle back and render the badges now that all the rendering with the name color is finished
         renderBadges(playerScores[j], x + nameWidth + 10 + gap, curRowY + (maxHeight / 2), scaleRatio);

         curRowY += maxHeight;
      }
      FontManager::popFontContext();

      // Go back and render the column headers, now that we know the widths.  These will be different for team and solo games.

      if(playerScores.size() > 0)
      {
         glColor(Colors::gray50);

         drawString_fixed(x, colHeaderYPos, (S32)ColHeaderTextSize, "Name");
         drawStringc(xr - (KdOff    + maxkdlen    / 2), colHeaderYPos, (S32)ColHeaderTextSize, "Threat Level");
         drawStringc(xr - (PingOff  - maxpinglen  / 2), colHeaderYPos, (S32)ColHeaderTextSize, "Ping");

         // Solo games need one more header
         if(!isTeamGame)
            drawStringc(xr - (ScoreOff + maxscorelen / 2), colHeaderYPos, (S32)ColHeaderTextSize, "Score");
      }

#ifdef USE_DUMMY_PLAYER_SCORES
      playerScores.deleteAndClear();      // Clean up
#endif
   }

   renderScoreboardLegend(getGame()->getPlayerCount(), scoreboardTop, totalHeight);

   FontManager::popFontContext();
}


void GameUserInterface::renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio)
{
   // Default to vector font for badges
   FontManager::pushFontContext(OldSkoolContext);

   F32 badgeRadius = 10.f * scaleRatio;
   S32 badgeOffset = S32(2 * badgeRadius) + 5;
   F32 badgeBackgroundEdgeSize = 2 * badgeRadius + 2.f;

   bool hasBBBBadge = false;

   for(S32 i = 0; i < BADGE_COUNT; i++)
   {
      MeritBadges badge = MeritBadges(i);    // C++ enums can be rather tedious...

      if(clientInfo->hasBadge(badge))
      {
         // Test for BBB badges.  We're only going to show the most valued one
         if(badge == BADGE_BBB_GOLD || badge == BADGE_BBB_SILVER || badge == BADGE_BBB_BRONZE || badge == BADGE_BBB_PARTICIPATION)
         {
            // If we've already got one, don't draw this badge.  This assumes the value of the badges decrease
            // with each iteration
            if(hasBBBBadge)
               continue;

            hasBBBBadge = true;
         }

         // Draw badge border
         glColor(Colors::gray20);
         drawRoundedRect(Point(x,y), badgeBackgroundEdgeSize, badgeBackgroundEdgeSize, 3.f);

         renderBadge((F32)x, (F32)y, badgeRadius, badge);
         x += badgeOffset;
      }
   }

   FontManager::popFontContext();
}


void GameUserInterface::renderBasicInterfaceOverlay()
{
   GameType *gameType = getGame()->getGameType();

   // Progress meter for file upload and download
   if(getGame()->getConnectionToServer())
   {
      F32 progress = getGame()->getConnectionToServer()->getFileProgressMeter();
      if(progress != 0)
      {
         glColor(Colors::yellow);
         drawRect(25.f, 200.f, progress * (gScreenInfo.getGameCanvasWidth()-50) + 25.f, 210.f, GL_TRIANGLE_FAN);
         drawRect(25, 200, gScreenInfo.getGameCanvasWidth()-25, 210, GL_LINE_LOOP);
      }
   }
   
   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
      renderInputModeChangeAlert();

   bool showScore = scoreboardIsVisible();

   if(showScore && getGame()->getTeamCount() > 0)      // How could teamCount be 0?
      renderScoreboard();
   
   // Render timer and associated doodads in the lower-right corner
   mTimeLeftRenderer.render(gameType, showScore, true);

   renderTalkingClients();
   renderDebugStatus();
}


bool GameUserInterface::shouldRenderLevelInfo() const 
{
   return mLevelInfoDisplayer.isActive() || mMissionOverlayActive;
}


void GameUserInterface::renderLevelInfo() 
{
   // Level Info requires gametype.  It can be NULL when switching levels
   if(getGame()->getGameType() == NULL)
      return;

   S32 teamCount = getGame()->getTeamCount();

   if(shouldRenderLevelInfo())
   {
      mLevelInfoDisplayer.render(getGame()->getGameType(), teamCount, getGame()->getLevelDatabaseId() > 0);
      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }
}


// Display alert about input mode changing
void GameUserInterface::renderInputModeChangeAlert() const
{
   F32 alpha = 1;

   if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
      alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   glColor(Colors::paleRed, alpha);
   drawCenteredStringf(vertMargin + 130, 20, "Input mode changed to %s", 
                       getGame()->getInputMode() == InputModeJoystick ? "Joystick" : "Keyboard");
}


void GameUserInterface::renderTalkingClients() const
{
   S32 y = 150;

   for(S32 i = 0; i < getGame()->getClientCount(); i++)
   {
      ClientInfo *client = ((Game *)getGame())->getClientInfo(i);

      if(client->getVoiceSFX()->isPlaying())
      {
         const S32 TEXT_HEIGHT = 20;

         glColor( getGame()->getTeamColor(client->getTeamIndex()) );
         drawString(10, y, TEXT_HEIGHT, client->getName().getString());
         y += TEXT_HEIGHT + 5;
      }
   }
}


void GameUserInterface::renderDebugStatus() const
{
   // When bots are frozen, render large pause icon in lower left
   if(EventManager::get()->isPaused())
   {
      glColor(Colors::white);

      const S32 PAUSE_HEIGHT = 30;
      const S32 PAUSE_WIDTH = 10;
      const S32 PAUSE_GAP = 6;
      const S32 BOX_INSET = 5;

      const S32 TEXT_SIZE = 15;
      const char *TEXT = "STEP: Alt-], Ctrl-]";

      S32 x, y;

      // Draw box
      x = gScreenInfo.getGameCanvasWidth() - horizMargin - 2 * (PAUSE_WIDTH + PAUSE_GAP) - BOX_INSET - getStringWidth(TEXT_SIZE, TEXT);
      y = vertMargin + PAUSE_HEIGHT;

      // Draw Pause symbol
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP;
      drawFilledRect(x, y, x + PAUSE_WIDTH, y - PAUSE_HEIGHT, Colors::black, Colors::white);

      x += PAUSE_WIDTH + PAUSE_GAP + BOX_INSET;

      y -= TEXT_SIZE + (PAUSE_HEIGHT - TEXT_SIZE) / 2 + 1;
      drawString(x, y, TEXT_SIZE, TEXT);
   }
}


// Show server-side object ids... using illegal reachover to obtain them!
void GameUserInterface::renderObjectIds() const
{
   TNLAssert(Game::isLocalTestServer(), "Will crash on non server!");
   if(!Game::isLocalTestServer())
      return;

   const Vector<DatabaseObject *> *objects = Game::getServerGameObjectDatabase()->findObjects_fast();

   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(objects->get(i));
      static const S32 height = 13;

      // ForceFields don't have a geometry.  When I gave them one, they just rendered the ID at the
      // exact same location as their owning projector - so we'll just skip them
      if(obj->getObjectTypeNumber() == ForceFieldTypeNumber)
         continue;

      S32 id = obj->getUserAssignedId();
      S32 width = getStringWidthf(height, "[%d]", id);

      F32 x = obj->getPos().x;
      F32 y = obj->getPos().y;

      glColor(Colors::black);
      drawFilledRect(x - 1, y - 1, x + width + 1, y + height + 1);

      glColor(Colors::gray70);
      drawStringf(x, y, height, "[%d]", id);
   }
}


void GameUserInterface::saveAlreadySeenLevelupMessageList()
{
   getGame()->getSettings()->getIniSettings()->mSettings.setVal("LevelupItemsAlreadySeenList", 
                                                                getAlreadySeenLevelupMessageString());
}


void GameUserInterface::loadAlreadySeenLevelupMessageList()
{
   setAlreadySeenLevelupMessageString(
         getGame()->getSettings()->getIniSettings()->mSettings.getVal<string>("LevelupItemsAlreadySeenList")
   );
}


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

   mChatMessageDisplayer1.onChatMessageReceived(msgColor, buffer);      // Standard chat stream
   mChatMessageDisplayer2.onChatMessageReceived(msgColor, buffer);      // Short, non-expiring chat stream
   mChatMessageDisplayer3.onChatMessageReceived(msgColor, buffer);      // Long, non-expiring chat stream
}


// Set which chat message display mode we're in (Ctrl-M)
void GameUserInterface::toggleChatDisplayMode()
{
   S32 m = mMessageDisplayMode + 1;

   if(m >= MessageDisplayModes)
      m = 0;

   mMessageDisplayMode = MessageDisplayMode(m);
}


// Return message being composed in in-game chat
const char *GameUserInterface::getChatMessage()
{
   return mHelperManager.getChatMessage();
}


// Some reusable containers
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
static void populateRenderZones()
{
   rawRenderObjects.clear();
   BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects);
   fillRenderZones();
}


static void populateRenderZones(const Rect extentRect)
{
   rawRenderObjects.clear();
   BotNavMeshZone::getBotZoneDatabase()->findObjects(BotNavMeshZoneTypeNumber, rawRenderObjects, extentRect);
   fillRenderZones();
}


static S32 QSORT_CALLBACK renderSortCompare(BfObject **a, BfObject **b)
{
   return (*a)->getRenderSortValue() - (*b)->getRenderSortValue();
}


void GameUserInterface::renderGameNormal()
{
   // Start of the level, we only show progress bar
   if(mShowProgressBar)
      return;

   // Here we determine if we have a control ship.
   // If not (like after we've been killed), we'll still render the current position and things
   Ship *ship = getGame()->getLocalPlayerShip();

   if(ship)
      visExt = getGame()->computePlayerVisArea(ship);

   // TODO: This should not be needed here -- mPos is set elsewhere, but appears to be lagged by a frame, which 
   //       creates a weird slightly off-center effect when moving.  This is harmless for the moment, but should be removed.
   if(ship)
      mShipPos.set(ship->getRenderPos());

   glPushMatrix();

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() / 2.f, gScreenInfo.getGameCanvasHeight() / 2.f, 0);       

   // These scaling factors are different when changing the visible area by equiping the sensor module
   F32 scaleFactX = (gScreenInfo.getGameCanvasWidth()  / 2) / visExt.x;
   F32 scaleFactY = (gScreenInfo.getGameCanvasHeight() / 2) / visExt.y;

   glScalef(scaleFactX, scaleFactY, 1);
   glTranslatef(-mShipPos.x, -mShipPos.y, 0);

   drawStars(mStars, mStarColors, NumStars, 1.0, mShipPos, visExt * 2);

   // Render all the objects the player can see
   screenSize.set(visExt);
   Rect extentRect(mShipPos - screenSize, mShipPos + screenSize);

   // Fill rawRenderObjects with anything within extentRect (our visibility extent)
   rawRenderObjects.clear();
   getGame()->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, rawRenderObjects, extentRect);    

   // Cast objects in rawRenderObjects and put them in renderObjects
   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   // Normally a big no-no, we'll access the server's bot zones directly if we are running locally so we can visualize them without bogging
   // the game down with the normal process of transmitting zones from server to client.  The result is that we can only see zones on our local
   // server.
   if(mDebugShowMeshZones)
      populateRenderZones(extentRect);

   if(mShowDebugBots)
      for(S32 i = 0; i < getGame()->getBotCount(); i++)
         renderObjects.push_back(getGame()->getBot(i));

   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 i = -1; i < 2; i++)
   {
      Barrier::renderEdges(i, *getGame()->getSettings()->getWallOutlineColor());    // Render wall edges

      if(mDebugShowMeshZones)
         for(S32 j = 0; j < renderZones.size(); j++)
            renderZones[j]->renderLayer(i);

      for(S32 j = 0; j < renderObjects.size(); j++)
         renderObjects[j]->renderLayer(i);

      mFxManager.render(i, getCommanderZoomFraction());
   }


   // Render a highlight/outline around any objects in our highlight type list, for help
   static Vector<const Vector<Point> *> polygons;
   polygons.clear();

   const Vector<HighlightItem> *itemsToHighlight = mHelpItemManager.getItemsToHighlight();      

   for(S32 i = 0; i < itemsToHighlight->size(); i++)
      for(S32 j = 0; j < renderObjects.size(); j++)
         if(itemsToHighlight->get(i).type == renderObjects[j]->getObjectTypeNumber())
         {
            HighlightItem::Whose whose = itemsToHighlight->get(i).whose;

            S32 team = renderObjects[j]->getTeam();
            S32 playerTeam = ship ? ship->getTeam() : NO_TEAM;

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
      getGame()->getGameObjDatabase()->findObjects(itemTypes, fillVector, *getGame()->getWorldExtents());
      polygons.clear();
      for(S32 i = 0; i < fillVector.size(); i++)
         polygons.push_back(fillVector[i]->getOutline());
   }
#endif

   if(polygons.size() > 0)
   {
      Vector<Vector<Point> > outlines;

      offsetPolygons(polygons, outlines, HIGHLIGHTED_OBJECT_BUFFER_WIDTH);

      for(S32 j = 0; j < outlines.size(); j++)
         renderPolygonOutline(&outlines[j], &Colors::green);
   }

   FxTrail::renderTrails();

   getUIManager()->getUI<GameUserInterface>()->renderEngineeredItemDeploymentMarker(ship);

   // Again, we'll be accessing the server's data directly so we can see server-side item ids directly on the client.  Again,
   // the result is that we can only see zones on our local server.
   if(mDebugShowObjectIds)
      renderObjectIds();

   glPopMatrix();

   // Render current ship's energy
   if(ship)
      UI::EnergyGaugeRenderer::render(ship->mEnergy);   

   //renderOverlayMap();     // Draw a floating overlay map
}


void GameUserInterface::renderGameCommander()
{
   // Start of the level, we only show progress bar
   if(mShowProgressBar)
      return;

   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

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

   //mShipPos = ship ? ship->getRenderPos()                 : Point(0,0);
   visSize = ship ? getGame()->computePlayerVisArea(ship) * 2 : worldExtents;


   glPushMatrix();

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() * 0.5f, gScreenInfo.getGameCanvasHeight() * 0.5f, 0);    

   F32 zoomFrac = getCommanderZoomFraction();

   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;
   glScalef(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y, 1);

   Point offset = (mDispWorldExtents.getCenter() - mShipPos) * zoomFrac + mShipPos;
   glTranslatef(-offset.x, -offset.y, 0);

   // zoomFrac == 1.0 when fully zoomed out to cmdr's map
   if(zoomFrac < 0.95)
      drawStars(mStars, mStarColors, NumStars, 1 - zoomFrac, offset, modVisSize);
 

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects.  Note that this no longer captures
   // walls -- those will be rendered separately.
   rawRenderObjects.clear();

   if(ship && ship->hasModule(ModuleSensor))
      getGame()->getGameObjDatabase()->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, rawRenderObjects);
   else
      getGame()->getGameObjDatabase()->findObjects((TestFunc)isVisibleOnCmdrsMapType, rawRenderObjects);

   renderObjects.clear();

   // Copy rawRenderObjects into renderObjects
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(static_cast<BfObject *>(rawRenderObjects[i]));

   // Add extra bots if we're showing them
   if(mShowDebugBots)
      for(S32 i = 0; i < getGame()->getBotCount(); i++)
         renderObjects.push_back(getGame()->getBot(i));

   // If we're drawing bot zones, get them now (put them in the renderZones vector)
   if(mDebugShowMeshZones)
      populateRenderZones();

   if(ship)
   {
      // Get info about the current player
      S32 playerTeam = -1;

      if(gameType)
      {
         playerTeam = ship->getTeam();
         Color teamColor = *ship->getColor();

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

                  glColor(teamColor * zoomFrac * 0.35f);
                  drawFilledRect(p.x - visExt.x, p.y - visExt.y, p.x + visExt.x, p.y + visExt.y);
               }
            }
         }

         const Vector<DatabaseObject *> *spyBugs = getGame()->getGameObjDatabase()->findObjects_fast(SpyBugTypeNumber);

         // Render spy bug visibility range second, so ranges appear above ship scanner range
         for(S32 i = 0; i < spyBugs->size(); i++)
         {
            SpyBug *sb = static_cast<SpyBug *>(spyBugs->get(i));

            if(sb->isVisibleToPlayer(playerTeam, gameType->isTeamGame()))
            {
               renderSpyBugVisibleRange(sb->getRenderPos(), teamColor);
               glColor(teamColor * 0.8f);     // Draw a marker in the middle
               drawCircle(sb->getRenderPos(), 2);
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
   Barrier::renderEdges(1, *getGame()->getSettings()->getWallOutlineColor());    // Render wall edges

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

   glPopMatrix();

   // Render current ship's energy
   if(ship)
      UI::EnergyGaugeRenderer::render(ship->mEnergy);   
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
//   const S32 canvasWidth  = gScreenInfo.getGameCanvasWidth();
//   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
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
//   renderVertexArray(vertices, 4, GL_LINE_LOOP);
//
//
//   glEnable(GL_SCISSOR_BOX);                    // Crop to overlay map display area
//   glScissor(mapX, mapY + mapHeight, mapWidth, mapHeight);  // Set cropping window
//
//   glPushMatrix();   // Set scaling and positioning of the overlay
//
//   glTranslatef(mapX + mapWidth / 2.f, mapY + mapHeight / 2.f, 0);          // Move map off to the corner
//   glScalef(mapScale, mapScale, 1);                                     // Scale map
//   glTranslatef(-position.x, -position.y, 0);                           // Put ship at the center of our overlay map area
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
//   glPopMatrix();
//   glDisable(GL_SCISSOR_BOX);     // Stop cropping
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


void GameUserInterface::renderSuspended()
{
   glColor(Colors::yellow);
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = gScreenInfo.getGameCanvasHeight() / 2 - 3 * (textHeight + textGap);

   drawCenteredString(ypos, textHeight, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   drawCenteredString(ypos, textHeight, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   drawCenteredString(ypos, textHeight, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   drawCenteredString(ypos, textHeight, "Press <SPACE> to resume playing now");
}


////////////////////////////////////////
////////////////////////////////////////

void ColorString::set(const string &s, const Color &c, U32 id)    // id defaults to 0
{
   str = s;
   color = c;
   groupId = id;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ChatMessageDisplayer::ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool expire, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontWidth)
{
   mDisplayChatMessageTimer.setPeriod(5000);    // How long messages stay visible (ms)
   mChatScrollTimer.setPeriod(100);             // Transition time when new msg arrives (ms) 

   mMessages.resize(msgCount + 1);              // Have an extra message for scrolling effect.  Will only display msgCount messages.

   reset();

   mGame      = game;
   mExpire    = expire;
   mTopDown   = topDown;
   mWrapWidth = wrapWidth;
   mFontSize  = fontSize;
   mFontGap   = fontWidth;
   
   mNextGroupId = 0;
}

// Destructor
ChatMessageDisplayer::~ChatMessageDisplayer()
{
   // Do nothing
}


// Effectivley clears all messages
void ChatMessageDisplayer::reset()
{
   mFirst = mLast = 0;
   mFull = false;
}


void ChatMessageDisplayer::idle(U32 timeDelta)
{
   mChatScrollTimer.update(timeDelta);

   // Clear out any expired messages
   if(mExpire && mDisplayChatMessageTimer.update(timeDelta))
   {
      mDisplayChatMessageTimer.reset();

      if(mFirst > mLast)
      {
         if(mTopDown)
            mChatScrollTimer.reset();

         advanceLast();         
      }
   }
}


// Make room for a new message at the head of our list
void ChatMessageDisplayer::advanceFirst()
{
   mFirst++;

   if(mLast % mMessages.size() == mFirst % mMessages.size())
   {
      mLast++;
      mFull = true;
   }
}


// Clear out messages from the back of our list; expire all messages with same id together.
void ChatMessageDisplayer::advanceLast()
{
   mLast++;

   U32 id = mMessages[mLast % mMessages.size()].groupId;

   while(mMessages[(mLast + 1) % mMessages.size()].groupId == id && mFirst > mLast)
      mLast++;

   mFull = false;

   TNLAssert(mLast <= mFirst, "index error! -- add check to correct this!");
}


// Replace %vars% in chat messages 
// Currently only evaluates names of keybindings (as used in the INI file), and %playerName%
// Vars are case insensitive
static string getSubstVarVal(ClientGame *game, const string &var)
{
   // %keybinding%
   InputCode inputCode = game->getSettings()->getInputCodeManager()->getKeyBoundToBindingCodeName(var);
   if(inputCode != KEY_UNKNOWN)
      return string("[") + InputCodeManager::inputCodeToString(inputCode) + "]";
   
   // %playerName%
   if(caseInsensitiveStringCompare(var, "playerName"))
      return game->getClientInfo()->getName().getString();

   // Not a variable... preserve formatting
   return "%" + var + "%";
}


// Add it to the list, will be displayed in render()
void ChatMessageDisplayer::onChatMessageReceived(const Color &msgColor, const string &msg)
{
   Vector<string> lines;
   wrapString(substitueVars(msg), mWrapWidth, mFontSize, ChatMessageContext, lines, "      ");

   // All lines from this message will share a groupId.  We'll use that to expire the group as a whole.
   for(S32 i = 0; i < lines.size(); i++)
   {
      advanceFirst();
      mMessages[mFirst % mMessages.size()].set(lines[i], msgColor, mNextGroupId); 
   }

   mNextGroupId++;

   // When displaying messages from the top of the screen, the animation happens when we expire messages
   mDisplayChatMessageTimer.reset();

   if(!mTopDown)
      mChatScrollTimer.reset();
}


// Check if we have any %variables% that need substituting
string ChatMessageDisplayer::substitueVars(const string &str)
{
   string s = str;      // Make working copy

   bool inside = false;

   std::size_t startPos, endPos;

   inside = false;

   for(std::size_t i = 0; i < s.length(); i++)
   {
      if(s[i] == '%')
      {
         if(!inside)    // Found beginning of variable
         {
            startPos = i + 1;
            inside = true;
         }
         else           // Found end of variable
         {
            endPos = i - startPos;
            inside = false;

            string var = s.substr(startPos, endPos);
            string val = getSubstVarVal(mGame, var);

            s.replace(startPos - 1, endPos + 2, val);

            i += val.length() - var.length() - 2;     // Make sure we don't evaluate the contents of val; i.e. no recursion
         }
      }
   }

   return s;
}


// Render any incoming player chat msgs
void ChatMessageDisplayer::render(S32 anchorPos, bool helperVisible, bool anouncementActive, F32 alpha) const
{
   // Are we in the act of transitioning between one message and another?
   bool isScrolling = (mChatScrollTimer.getCurrent() > 0);  

   // Check if there any messages to display... if not, bail
   if(mFirst == mLast && !(mTopDown && isScrolling))
      return;

   S32 lineHeight = mFontSize + mFontGap;


   // Reuse this to avoid startup and breakdown costs
   static ScissorsManager scissorsManager;

   // Only need to set scissors if we're scrolling.  When not scrolling, we control the display by only showing
   // the specified number of lines; there are normally no partial lines that need vertical clipping as 
   // there are when we're scrolling.  Note also that we only clip vertically, and can ignore the horizontal.
   if(isScrolling)    
   {
      // Remember that our message list contains an extra entry that exists only for scrolling purposes.
      // We want the height of the clip window to omit this line, so we subtract 1 below.  
      S32 displayAreaHeight = (mMessages.size() - 1) * lineHeight;     
      S32 displayAreaYPos = anchorPos + (mTopDown ? displayAreaHeight : lineHeight);

      scissorsManager.enable(true, mGame->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"), 
                             0.0f, F32(displayAreaYPos - displayAreaHeight), F32(gScreenInfo.getGameCanvasWidth()), F32(displayAreaHeight));
   }

   // Initialize the starting rendering position.  This represents the bottom of the message rendering area, and
   // we'll work our way up as we go.  In all cases, newest messages will appear on the bottom, older ones on top.
   // Note that anchorPos reflects something different (i.e. the top or the bottom of the area) in each case.
   S32 y = anchorPos + S32(mChatScrollTimer.getFraction() * lineHeight);

   // Advance anchor from top to the bottom of the render area.  When we are rendering at the bottom, anchorPos
   // already represents the bottom, so no additional adjustment is necessary.
   if(mTopDown)
      y += (mFirst - mLast - 1) * lineHeight;

   // Render an extra message while we're scrolling (in some cases).  Scissors will control the total vertical height.
   S32 renderExtra = 0;
   if(isScrolling)
   {
      if(mTopDown)
         renderExtra = 1;
      else if(mFull)    // Only render extra item on bottom-up if list is fully occupied
         renderExtra = 1;
   }

   // Adjust our last line if we have an announcement
   U32 last = mLast;
   if(anouncementActive)
   {
      // Render one less line if we're past the size threshold for this displayer
      if(mFirst >= (U32)mMessages.size() - 1)
         last++;

      y -= lineHeight;
   }

   FontManager::pushFontContext(ChatMessageContext);

   // Draw message lines
   for(U32 i = mFirst; i != last - renderExtra; i--)
   {
      U32 index = i % (U32)mMessages.size();    // Handle wrapping in our message list

      glColor(mMessages[index].color, alpha); 

      drawString(UserInterface::horizMargin, y, mFontSize, mMessages[index].str.c_str());

      y -= lineHeight;
   }

   FontManager::popFontContext();

   // Restore scissors settings -- only used during scrolling
   scissorsManager.disable();
}


////////////////////////////////////////
////////////////////////////////////////


LevelListDisplayer::LevelListDisplayer()
{
   mLevelLoadDisplayFadeTimer.setPeriod(1000);
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
      TNLAssert(glIsEnabled(GL_BLEND), "Blending should be enabled here!");

      for(S32 i = 0; i < mLevelLoadDisplayNames.size(); i++)
      {
         glColor(Colors::white, (1.4f - ((F32) (mLevelLoadDisplayNames.size() - i) / 10.f)) * 
                                        (mLevelLoadDisplay ? 1 : mLevelLoadDisplayFadeTimer.getFraction()) );
         drawStringf(100, gScreenInfo.getGameCanvasHeight() - /*vertMargin*/ 0 - (mLevelLoadDisplayNames.size() - i) * 20, 
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

