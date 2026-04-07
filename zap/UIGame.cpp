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
#include "projectile.h"          // For SpyBug

#include "GaugeRenderer.h"

#include "Console.h"             // Our console object
#include "DisplayManager.h"
#include "Renderer.h"
#include "ClientGame.h"
#include "ServerGame.h"
#include "Colors.h"
#include "Cursor.h"
#include "ScissorsManager.h"
#include "voiceCodec.h"
#include "SoundSystem.h"
#include "FontManager.h"
#include "Intervals.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "GeomUtils.h"

#include "GameRecorderPlayback.h"
#include "XtankShape.h"     // For xtank vehicle body cycling (Ctrl+Alt+Shift+X)

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
                  mLevelInfoDisplayer(game),
                  mHelpItemManager(game->getSettings()),
                  mScoreboardRenderer(this)
{
   mInScoreboardMode = false;
   displayInputModeChangeAlert = false;
   mMissionOverlayActive = false;
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;

   mHelperManager.initialize(game);

   mMessageDisplayMode = ShortTimeout;

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
   mHasShipPos = false;
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
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;

   clearDisplayers();

   // Clear out any walls we were using in a previous run
   Barrier::clearRenderItems();           // TODO: Should really go in an onDeactivate method, which we don't really have
   mLevelInfoDisplayer.clearDisplayTimer();

   mLoadoutIndicator.reset();
   mShowProgressBar = true;               // Causes screen to be black before level is loaded
   mHasShipPos = false;

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
   mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap = true;
}


// Called when level just beginning (called from GameConnection::onStartGhosting)
// We probably don't have a GameType yet, so we don't know what our level name will be
void GameUserInterface::onGameStarting()
{
   mDispWorldExtents.set(Point(0,0), 0);
   Barrier::clearRenderItems();
   mHasShipPos = false;

   mHelpItemManager.addStartingHelpItemsToQueue(getGame());      // Do this here so if the helpItem manager gets turned on, items will start displaying next game
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
   mConnectionStatsRenderer.idle(timeDelta, getGame()->getConnectionToServer());

   mHelperManager.idle(timeDelta);
   mVoiceRecorder.idle(timeDelta);
   mLevelListDisplayer.idle(timeDelta);

   mLoadoutIndicator.idle(timeDelta);

   // Processes sparks and teleporter effects --
   //    do this even while suspended to make objects look normal while in /idling
   //    But not while playing back game recordings, idled in idleFxManager with custom timeDelta
   if(dynamic_cast<GameRecorderPlayback *>(getGame()->getConnectionToServer()) == NULL)
      mFxManager.idle(timeDelta);

   if(shouldCountdownHelpItemTimer())
      mHelpItemManager.idle(timeDelta, getGame());

   // Update mShipPos... track this so that we can keep a fix on the ship location even if it subsequently dies
   Ship *ship = getGame()->getLocalPlayerShip();

   if(ship)
   {
      mShipPos.set(ship->getRenderPos());     // Get the player's ship position
      mHasShipPos = true;
   }

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


void GameUserInterface::toggleShowingShipCoords() { mDebugOverlayRenderer.toggleShowingShipCoords(); }
void GameUserInterface::toggleShowingObjectIds()  { mDebugOverlayRenderer.toggleShowingObjectIds();  }
void GameUserInterface::toggleShowingMeshZones()  { mDebugOverlayRenderer.toggleShowingMeshZones();  }
void GameUserInterface::toggleShowDebugBots()     { mDebugOverlayRenderer.toggleShowDebugBots();     }


bool GameUserInterface::isShowingDebugShipCoords() const { return mDebugOverlayRenderer.isShowingDebugShipCoords(); }


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
   mChatMessageDisplayer1.reset();
   mChatMessageDisplayer2.reset();
   mChatMessageDisplayer3.reset();

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
   Renderer& r = Renderer::get();

   if(!getGame()->isConnectedToServer())
   {
      r.setColor(Colors::white);
      drawCenteredString(260, 30, "Connecting to server...");

      r.setColor(Colors::green);
      if(getGame()->getConnectionToServer())
         drawCenteredString(310, 16, GameConnection::getConnectionStateString(getGame()->getConnectionToServer()->getConnectionState()));

      r.setColor(Colors::white);
      drawCenteredString(346, 20, "Press <ESC> to abort");

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

   mFpsRenderer.render(DisplayManager::getScreenInfo()->getGameCanvasWidth());     // Display running average FPS
   mConnectionStatsRenderer.render(getGame()->getConnectionToServer());     // Display running average FPS

   mHelperManager.render();

   GameType *gameType = getGame()->getGameType();

   if(gameType)
      gameType->renderInterfaceOverlay(DisplayManager::getScreenInfo()->getGameCanvasWidth(), DisplayManager::getScreenInfo()->getGameCanvasHeight());

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
   if(getGame()->inReturnToGameCountdown())
   {
      static string waitMsg[] = { "", "WILL RESPAWN", "IN BLAH BLAH SECONDS", "" };
      waitMsg[2] = "IN " + ftos(ceil(F32(getGame()->getReturnToGameDelay()) * MS_TO_SECONDS)) + " SECONDS";
      renderMsgBox(waitMsg,  ARRAYSIZE(waitMsg));
   }
   else
   {
      static string readyMsg[] = { "", "PRESS ANY", "KEY TO", "RESPAWN", "" };
      renderMsgBox(readyMsg, ARRAYSIZE(readyMsg));
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
   renderMsgBox(msg,  ARRAYSIZE(msg));
}


// This is a helper for renderSuspendedMessage and renderLevelUpMessage.  It assumes that none of the messages
// will have [[key_bindings]] in them.  If this assumption changes, will need to replace the NULL below in the
// SymbolString() construction.
void GameUserInterface::renderMsgBox(const string *message, S32 msgLines) const
{
   Vector<SymbolShapePtr> messages(msgLines);

   for(S32 i = 0; i < msgLines; i++)
      messages.push_back(SymbolShapePtr(new SymbolString(message[i], NULL, ErrorMsgContext, 30, true)));

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
      static string msg = "We have lost contact with the server; You can't play "
                          "until the connection has been re-established.\n\n"
                          "Trying to reconnect... [[SPINNER]]";

      renderMessageBox("SERVER CONNECTION PROBLEMS", "", msg, -30);
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
         string msg = string(timemsg) + "\n\nShutdown sequence intitated by you.\n\n" + mShutdownReason.getString();
         renderMessageBox("SERVER SHUTDOWN INITIATED", "Press [[Esc]] to cancel shutdown", msg, 7);
      }
      else                       // Remote user intiated the shutdown
      {
         char whomsg[255];
         dSprintf(whomsg, sizeof(whomsg), "Shutdown sequence initiated by %s.", mShutdownName.getString());

         string msg = string(timemsg) + "\n\n" +
                      whomsg + "\n\n" +
                      mShutdownReason.getString();
         renderMessageBox("SHUTDOWN INITIATED", "Press [[Esc]] to dismiss", msg, 7);
      }
   }
   else if(mShutdownMode == Canceled)
   {
      // Keep same number of messages as above, so if message changes, it will be a smooth transition
      string msg = "Server shutdown sequence canceled.\n\n"
                   "Play on!";

      renderMessageBox("SHUTDOWN CANCELED", "Press [[Esc]] to dismiss", msg, 7);
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
   Renderer& r = Renderer::get();

   if((mShowProgressBar || mProgressBarFadeTimer.getCurrent() > 0) && gt && gt->mObjectsExpected > 0)
   {
      r.setColor(Colors::green, mShowProgressBar ? 1 : mProgressBarFadeTimer.getFraction());

      // Outline
      const F32 left = 200;
      const F32 width = DisplayManager::getScreenInfo()->getGameCanvasWidth() - 2 * left;
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
               left,     F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin),
               left + w, F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin),
               left + w, F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - height),
               left,     F32(DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin - height)
         };
         r.renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? RenderType::LineLoop : RenderType::TriangleFan);
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

   Point offsetMouse = mMousePoint + Point(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2, DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2);

   F32 vertices[] = {
      // Center cross-hairs
      offsetMouse.x - 15, offsetMouse.y,
      offsetMouse.x + 15, offsetMouse.y,
      offsetMouse.x,      offsetMouse.y - 15,
      offsetMouse.x,      offsetMouse.y + 15,

      // Large axes lines
      0, offsetMouse.y,
      offsetMouse.x - 30, offsetMouse.y,

      offsetMouse.x + 30, offsetMouse.y,
      (F32)DisplayManager::getScreenInfo()->getGameCanvasWidth(), offsetMouse.y,

      offsetMouse.x, 0,
      offsetMouse.x, offsetMouse.y - 30,

      offsetMouse.x, offsetMouse.y + 30,
      offsetMouse.x, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight(),
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

   Renderer::get().renderColored(vertices, colors, ARRAYSIZE(vertices) / 2, RenderType::Lines);
}


void GameUserInterface::renderWrongModeIndicator() const
{
   if(mWrongModeMsgDisplay.getCurrent())
   {
      // Fade for last half second
      F32 alpha = mWrongModeMsgDisplay.getCurrent() < 500 ? mWrongModeMsgDisplay.getCurrent() / 500.0f : 1.0f;

      Renderer::get().setColor(Colors::red, alpha);
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

   mMousePoint.set(DisplayManager::getScreenInfo()->getMousePos()->x - DisplayManager::getScreenInfo()->getGameCanvasWidth()  / 2,
                   DisplayManager::getScreenInfo()->getMousePos()->y - DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2);

   if(mInCommanderMap)     // Ship not in center of the screen in cmdrs map.  Where is it?
   {
      Ship *ship = getGame()->getLocalPlayerShip();

      if(!ship)
         return;

      Point o = ship->getRenderPos();  // To avoid taking address of temporary
      Point p = worldToScreenPoint(&o, DisplayManager::getScreenInfo()->getGameCanvasWidth(), DisplayManager::getScreenInfo()->getGameCanvasHeight());

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


// Notify the HUD that the server has sent an updated xtank design.
void GameUserInterface::xtankDesignUpdated(const XtankDesign &design)
{
   mLoadoutIndicator.setXtankDesign(design);
}


// Apply an xtank design chosen by the player (via the UIXtankHelper).
// Updates the local ship immediately for responsive rendering, and sets the
// Move fields so the server will receive the new design on the next tick.
void GameUserInterface::applyXtankDesign(const XtankDesign &design)
{
   // Update the move so the server learns about the new design.
   mCurrentMove.bodyIndex    = design.bodyIndex;
   mCurrentMove.engineType   = (S8)design.engineType;
   mCurrentMove.treadType    = (S8)design.treadType;
   mCurrentMove.heatSinkCount = design.heatSinkCount;
   for(S32 i = 0; i < 4; i++)
      mCurrentMove.weaponSlot[i] = (S8)(S32)design.weapons[i];

   // Update the local ship immediately (client-side prediction).
   Ship *ship = getGame()->getLocalPlayerShip();
   if(ship)
   {
      ship->mXtankBodyIndex = design.bodyIndex;
      ship->mXtankDesign    = design;
   }

   // Notify the HUD indicator.
   mLoadoutIndicator.setXtankDesign(design);
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
         if(!checkInputCode(BINDING_OUTGAMECHAT, inputCode)  &&
            !checkInputCode(BINDING_GLOBCHAT,    inputCode)  &&
            !checkInputCode(BINDING_TEAMCHAT,    inputCode)  &&
            !checkInputCode(BINDING_CMDCHAT,     inputCode)  &&
            !checkInputCode(BINDING_SCRBRD,      inputCode))
         {
            getGame()->undelaySpawn();
            if(inputCode != KEY_ESCAPE)  // Don't swollow escape: Lagged out and can't un-idle to bring up the menu?
               return true;
         }
      }
   }

   if(checkInputCode(BINDING_OUTGAMECHAT, inputCode))
      getGame()->setBusyChatting(true);

   if(Parent::onKeyDown(inputCode))    // Let parent try handling the key
      return true;

   if(gConsole.onKeyDown(inputCode))   // Pass the key on to the console for processing
      return true;

   if(checkInputCode(BINDING_HELP, inputCode))   // Turn on help screen
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
      if(gConsole.isOk())                 // Console is only not Ok if something bad has happened somewhere
         gConsole.toggleVisibility();

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

   if(!gConsole.isVisible())
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
   if(gConsole.isVisible())
      gConsole.onKeyDown(ascii);

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

   // Ctrl+Alt+Shift+X cycles through xtank vehicle bodies
   else if(inputCode == KEY_X &&
           InputCodeManager::checkModifier(KEY_CTRL, KEY_ALT, KEY_SHIFT))
   {
      Ship *ship = getGame()->getLocalPlayerShip();
      if(ship)
      {
         ship->cycleXtankBody();
         S32 bodyIdx = ship->getXtankBodyIndex();
         const char *bodyName = (bodyIdx >= 0) ? xtankBodyNames[bodyIdx] : "Bitfighter Ship";
         getGame()->displayMessage(Colors::cyan, "Vehicle body: %s", bodyName);

         // Propagate the new body choice to the server via the Move struct.
         // mCurrentMove.bodyIndex is delta-compressed, so it will be sent
         // in the next packet and every packet thereafter until it changes.
         mCurrentMove.bodyIndex = (S8)bodyIdx;
      }
   }

   else if(checkInputCode(BINDING_LOAD_PRESET_1, inputCode))  // Loading loadout presets
      loadLoadoutPreset(getGame(), 0);
   else if(checkInputCode(BINDING_LOAD_PRESET_2, inputCode))
      loadLoadoutPreset(getGame(), 1);
   else if(checkInputCode(BINDING_LOAD_PRESET_3, inputCode))
      loadLoadoutPreset(getGame(), 2);
   else if(checkInputCode(BINDING_LOAD_PRESET_4, inputCode))
      loadLoadoutPreset(getGame(), 3);
   else if(checkInputCode(BINDING_LOAD_PRESET_5, inputCode))
      loadLoadoutPreset(getGame(), 4);
   else if(checkInputCode(BINDING_LOAD_PRESET_6, inputCode))
      loadLoadoutPreset(getGame(), 5);

   else if(checkInputCode(BINDING_SAVE_PRESET_1, inputCode))  // Saving loadout presets
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 0);
   else if(checkInputCode(BINDING_SAVE_PRESET_2, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 1);
   else if(checkInputCode(BINDING_SAVE_PRESET_3, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 2);
   else if(checkInputCode(BINDING_SAVE_PRESET_4, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 3);
   else if(checkInputCode(BINDING_SAVE_PRESET_5, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 4);
   else if(checkInputCode(BINDING_SAVE_PRESET_6, inputCode))
      saveLoadoutPreset(getGame(), mLoadoutIndicator.getLoadout(), 5);

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
      if(!mVoiceRecorder.isRecording())  // Turning recorder on
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
         {
            // In xtank mode, open the vehicle design menu; otherwise the standard loadout menu.
            Ship *localShip = getGame()->getLocalPlayerShip();
            if(localShip && localShip->getXtankBodyIndex() >= 0)
               activateHelper(HelperMenu::XtankHelperType);
            else
               activateHelper(HelperMenu::LoadoutHelperType);
         }
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
   Renderer& r = Renderer::get();

   r.setColor(Colors::red);
   r.setLineWidth(gLineWidth4);

   S32 x = drawStringAndGetWidth(UserInterface::horizMargin, pos, 16, "*** ");
   x += drawStringAndGetWidth(UserInterface::horizMargin + x, pos, 16, mAnnouncement.c_str());
   drawString(UserInterface::horizMargin + x, pos, 16, " ***");

   r.setLineWidth(gDefaultLineWidth);
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
      if(mVoiceRecorder.isRecording())  // Turning recorder off
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

   if(!mDisableShipKeyboardInput && getUIManager()->isCurrentUI<GameUserInterface>() && !gConsole.isVisible())
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

         mCurrentMove.x = F32((InputCodeManager::getState(getInputCode(settings, BINDING_RIGHT)) ? 1 : 0) -
                              (InputCodeManager::getState(getInputCode(settings, BINDING_LEFT))  ? 1 : 0));

         mCurrentMove.y = F32((InputCodeManager::getState(getInputCode(settings, BINDING_DOWN))  ? 1 : 0) -
                              (InputCodeManager::getState(getInputCode(settings, BINDING_UP))    ? 1 : 0));
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
   if(!mLevelInfoDisplayer.isActive())
      mLevelInfoDisplayer.onActivated();

   mLevelInfoDisplayer.resetDisplayTimer();
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
         Renderer::get().setColor(Colors::yellow);
         drawRect(25.f, 200.f, progress * (DisplayManager::getScreenInfo()->getGameCanvasWidth()-50) + 25.f, 210.f, RenderType::TriangleFan);
         drawRect(25, 200, DisplayManager::getScreenInfo()->getGameCanvasWidth()-25, 210, RenderType::LineLoop);
      }
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
      renderInputModeChangeAlert();

   bool showScore = scoreboardIsVisible();

   if(showScore && getGame()->getTeamCount() > 0)      // How could teamCount be 0?
      mScoreboardRenderer.render();

   // Render timer and associated doodads in the lower-right corner
   mTimeLeftRenderer.render(gameType, showScore, true);

   renderTalkingClients();
   mDebugOverlayRenderer.renderDebugStatus();
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

   if(shouldRenderLevelInfo())
   {
      mLevelInfoDisplayer.render();
      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }
}


// Display alert about input mode changing
void GameUserInterface::renderInputModeChangeAlert() const
{
   F32 alpha = 1;

   if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
      alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

   Renderer::get().setColor(Colors::paleRed, alpha);
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

         Renderer::get().setColor( *getGame()->getTeamColor(client->getTeamIndex()) );
         drawString(10, y, TEXT_HEIGHT, client->getName().getString());
         y += TEXT_HEIGHT + 5;
      }
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


HelpItemManager *GameUserInterface::getHelpItemManager()
{
   return &mHelpItemManager;
}


// Some reusable containers --> will probably need to become non-static if we have more than one clientGame active
static Point screenSize, visSize, visExt;
static Vector<DatabaseObject *> rawRenderObjects;
static Vector<BfObject *> renderObjects;


static bool renderSortCompare(BfObject * const &a, BfObject* const &b)
{
   return (a->getRenderSortValue() < b->getRenderSortValue());
}


void GameUserInterface::renderGameNormal()
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

   // TODO: This should not be needed here -- mPos is set elsewhere, but appears to be lagged by a frame, which
   //       creates a weird slightly off-center effect when moving.  This is harmless for the moment, but should be removed.
   mShipPos.set(ship->getRenderPos());
   mHasShipPos = true;

   Renderer& r = Renderer::get();
   r.pushMatrix();

   // Put (0,0) at the center of the screen
   r.translate(DisplayManager::getScreenInfo()->getGameCanvasWidth() / 2.f,
                DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2.f, 0);

   // These scaling factors are different when changing the visible area by equiping the sensor module
   F32 scaleFactX = (DisplayManager::getScreenInfo()->getGameCanvasWidth()  / 2) / visExt.x;
   F32 scaleFactY = (DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2) / visExt.y;

   r.scale(scaleFactX, scaleFactY, 1);
   r.translate(-mShipPos.x, -mShipPos.y, 0);

   renderStars(mStars, mStarColors, NumStars, 1.0, mShipPos, visExt * 2);

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

   // Normally a big no-no, we'll access the server's bot zones directly if we are running locally
   // so we can visualize them without bogging the game down with the normal process of transmitting
   // zones from server to client.  The result is that we can only see zones on our local server.
   if(mDebugOverlayRenderer.renderingMeshZones())
      mDebugOverlayRenderer.populateRenderZones(getGame(), &extentRect);

   if(mDebugOverlayRenderer.renderingBotPaths())
      mDebugOverlayRenderer.appendBotPaths(getGame(), renderObjects);

   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 i = -1; i < 2; i++)
   {
      Barrier::renderEdges(i, *getGame()->getSettings()->getWallOutlineColor());    // Render wall edges

      if(mDebugOverlayRenderer.renderingMeshZones())
         mDebugOverlayRenderer.renderMeshZones(i);

      for(S32 j = 0; j < renderObjects.size(); j++)
         renderObjects[j]->renderLayer(i);

      mFxManager.render(i, getCommanderZoomFraction());
   }

   S32 team = NONE;
   if(getGame()->getLocalRemoteClientInfo())
      team = getGame()->getLocalRemoteClientInfo()->getTeamIndex();
   renderInlineHelpItemOutlines(team, getBackgroundTextDimFactor(false));

   FxTrail::renderTrails();

   getUIManager()->getUI<GameUserInterface>()->renderEngineeredItemDeploymentMarker(ship);

   // Again, we'll be accessing the server's data directly so we can see server-side item ids directly on the client.  Again,
   // the result is that we can only see zones on our local server.
   if(mDebugOverlayRenderer.renderingObjectIds())
      mDebugOverlayRenderer.renderObjectIds(getGame());

   r.popMatrix();

   // Render current ship's energy
   if(ship)
      UI::GaugeRenderer::render(ship->mEnergy, ship->mHealth);


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
               ((whose == HighlightItem::Enemy || whose == HighlightItem::EorHostile) && ((team >= 0 && team != playerTeam) || team == TEAM_HOSTILE)) ||
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
         if(static_cast<BfObject *>(fillVector[i])->shouldRender())
            polygons.push_back(fillVector[i]->getOutline());
   }
#endif

   if(polygons.size() > 0)
   {
      Vector<Vector<Point> > outlines;

      offsetPolygons(polygons, outlines, HIGHLIGHTED_OBJECT_BUFFER_WIDTH);

      for(S32 j = 0; j < outlines.size(); j++)
         renderPolygonOutline(&outlines[j], &Colors::green, alpha);
   }
}


void GameUserInterface::renderGameCommander()
{
   // Start of the level, we only show progress bar
   if(mShowProgressBar)
      return;

   const S32 canvasWidth  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   const S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();
   Renderer& r = Renderer::get();

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


   r.pushMatrix();

   // Put (0,0) at the center of the screen
   r.translate(DisplayManager::getScreenInfo()->getGameCanvasWidth() * 0.5f, DisplayManager::getScreenInfo()->getGameCanvasHeight() * 0.5f, 0);

   F32 zoomFrac = getCommanderZoomFraction();

   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;
   r.scale(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y, 1);

   // We should probably check that mHasShipPos == true, but it will hardly ever matter
   Point offset = (mDispWorldExtents.getCenter() - mShipPos) * zoomFrac + mShipPos;
   r.translate(-offset.x, -offset.y, 0);

   // zoomFrac == 1.0 when fully zoomed out to cmdr's map
   renderStars(mStars, mStarColors, NumStars, 1 - zoomFrac, offset, modVisSize);

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
   if(mDebugOverlayRenderer.renderingBotPaths())
      mDebugOverlayRenderer.appendBotPaths(getGame(), renderObjects);

   // If we're drawing bot zones, get them now (put them in the renderZones vector)
   if(mDebugOverlayRenderer.renderingMeshZones())
      mDebugOverlayRenderer.populateRenderZones(getGame());

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

                  Renderer::get().setColor(teamColor * zoomFrac * 0.35f);
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
               Renderer::get().setColor(teamColor * 0.8f);     // Draw a marker in the middle
               drawCircle(sb->getRenderPos(), 2);
            }
         }
      }
   }

   // Now render the objects themselves
   renderObjects.sort(renderSortCompare);

   if(mDebugOverlayRenderer.renderingMeshZones())
      mDebugOverlayRenderer.renderMeshZones(0);

   // First pass
   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->renderLayer(0);

   // Second pass
   Barrier::renderEdges(1, *getGame()->getSettings()->getWallOutlineColor());    // Render wall edges

   if(mDebugOverlayRenderer.renderingMeshZones())
      mDebugOverlayRenderer.renderMeshZones(1);

   for(S32 i = 0; i < renderObjects.size(); i++)
   {
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->renderLayer(1);
   }

   getUIManager()->getUI<GameUserInterface>()->renderEngineeredItemDeploymentMarker(ship);

   r.popMatrix();

   // Render current ship's energy
   if(ship)
      UI::GaugeRenderer::render(ship->mEnergy, ship->mHealth);

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
   Renderer::get().setColor(Colors::yellow);
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = DisplayManager::getScreenInfo()->getGameCanvasHeight() / 2 - 3 * (textHeight + textGap);

   drawCenteredString(ypos, textHeight, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   drawCenteredString(ypos, textHeight, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   drawCenteredString(ypos, textHeight, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   drawCenteredString(ypos, textHeight, "Press <SPACE> to resume playing now");
}


};
