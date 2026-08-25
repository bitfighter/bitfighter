//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIGAME_H_
#define _UIGAME_H_

#include "UI.h"
#include "UILevelInfoDisplayer.h"
#include "HelperManager.h"
#include "LoadoutIndicator.h"
#include "TimeLeftRenderer.h"
#include "FpsRenderer.h"
#include "ConnectionStatsRenderer.h"
#include "HelpItemManager.h"
#include "move.h"
#include "config.h"     // For UserSettings def
#include "ClientGame.h"

#include "SoundEffect.h"
#include "sparkManager.h"

#include "Color.h"
#include "Rect.h"
#include "ChatDisplay.h"
#include "LevelListDisplayer.h"
#include "VoiceRecorder.h"
#include "ScoreboardRenderer.h"
#include "DebugOverlayRenderer.h"

#include "tnlString.h"


namespace Zap
{

class GameUserInterface;

// For displaying levels as they're loaded in host mode
class Move;

using namespace Zap::UI;

class GameUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   enum MessageDisplayMode {
      ShortTimeout,            // Traditional message display mode (6 MessageDisplayCount lines, messages timeout after DisplayMessageTimeout)
      ShortFixed,              // Same length as ShortTimeout, but without timeout
      LongFixed,               // Long form: Display MessageStoreCount messages, no timeout
      MessageDisplayModes
   };


   MessageDisplayMode mMessageDisplayMode;    // Our current message display mode

   Move mCurrentMove;
   Move mTransformedMove;
   Point mMousePoint;
   Point mShipPos;      // Track ship pos, so we have something if the ship disappears for whatever reason

   HelperManager mHelperManager;
   LoadoutIndicator mLoadoutIndicator;
   TimeLeftRenderer mTimeLeftRenderer;

   bool mAlreadySeenLevelupMsg[UserSettings::LevelCount];     // Tracks which levelup messages we've already seen

   bool mCmdrsMapKeyRepeatSuppressionSystemApprovesToggleCmdrsMap;


   LevelListDisplayer mLevelListDisplayer;

   Rect mDispWorldExtents;          // Extents we display when we are in cmdrs map (usually the same as Game::mWorldExtents)
   Timer mShrinkDelayTimer;

   bool mInCommanderMap;
   Timer mCommanderZoomDelta;

   Timer mShutdownTimer;


   bool mMissionOverlayActive;      // Are game instructions (F2) visible?

   enum ShutdownMode {
      None,                         // Nothing happening
      ShuttingDown,                 // Shutting down, obviously
      Canceled                      // Was shutting down, but are no longer
   };


   UI::FxManager mFxManager;

   static const S32 NumStars = 256;    // 256 stars should be enough for anybody!   -- Bill Gates
   Point mStars[NumStars];
   Color mStarColors[NumStars];
   void prepareStars();

   U32 mChatCursorPos;        // Position of composition cursor

   bool mInScoreboardMode;
   ShutdownMode mShutdownMode;

   Timer mProgressBarFadeTimer;     // For fading out progress bar after level is loaded
   bool mShowProgressBar;
   bool mHasShipPos;                // True if mShipPos has been explicitly set


   // Some key press/release handler helpers
   void onMissionKeyPressed();
   void onMissionKeyReleased();

   StringTableEntry mShutdownName;  // Name of user who initiated the shutdown
   StringPtr mShutdownReason;       // Reason user provided for the shutdown
   bool mShutdownInitiator;         // True if local client initiated shutdown (and can therefore cancel it)

   bool mGotControlUpdate;

   Timer mWrongModeMsgDisplay;               // Help if user is trying to use keyboard in joystick mode
   Timer mInputModeChangeAlertDisplayTimer;  // Remind user that they just changed input modes

   void updateChatMessages(U32 timeDelta);
   bool checkEnterChatInputCode(InputCode inputCode);  // Helper for key handler

   // Retnder helpers
   void renderInputModeChangeAlert() const;
   void renderTalkingClients() const;        // Render things related to voice chat
   void renderChatMsgs() const;
   void renderAnnouncement(S32 pos) const;
   void renderInlineHelpItemOutlines(S32 playerTeam, F32 alpha) const;


   F32 getBackgroundTextDimFactor(bool chatRelated) const;

   VoiceRecorder mVoiceRecorder;

   ChatDisplay mServerMessageDisplayer;   // Messages from the server
   ChatDisplay mChatMessageDisplayer1;    // Short form, message expire
   ChatDisplay mChatMessageDisplayer2;    // Short form, messages do not expire
   ChatDisplay mChatMessageDisplayer3;    // Long form, messages do not expire

   UI::FpsRenderer mFpsRenderer;
   UI::LevelInfoDisplayer mLevelInfoDisplayer;
   UI::ConnectionStatsRenderer mConnectionStatsRenderer;

   HelpItemManager mHelpItemManager;
   ScoreboardRenderer mScoreboardRenderer;
   DebugOverlayRenderer mDebugOverlayRenderer;

   Timer mAnnouncementTimer;
   string mAnnouncement;

   void dropItem();                                // User presses drop item key


   void loadAlreadySeenLevelupMessageList();
   void saveAlreadySeenLevelupMessageList();

   const string getAlreadySeenLevelupMessageString() const;
   void setAlreadySeenLevelupMessageString(const string &vals);

   bool mFiring;                                   // Are we firing?
   bool mModPrimaryActivated[ShipModuleCount];
   bool mModSecondaryActivated[ShipModuleCount];

   void rectifyExtents(U32 timeDelta);

   Timer mModuleDoubleTapTimer[ShipModuleCount];   // Timer for detecting if a module key is double-tapped
   static const S32 DoubleClickTimeout = 200;      // Timeout in milliseconds

   // Some key event handlers
   bool processPlayModeKey(InputCode inputCode);
   void checkForKeyboardMovementKeysInJoystickMode(InputCode inputCode);
   bool shouldCountdownHelpItemTimer() const;

   bool renderWithCommanderMap() const;

   SFXHandle playSoundEffect(U32 profileIndex, F32 gain = 1.0) const;

public:
   explicit GameUserInterface(ClientGame *game);  // Constructor
   virtual ~GameUserInterface();                  // Destructor

   bool displayInputModeChangeAlert;

   void toggleChatDisplayMode();                  // Set which chat message display mode we're in (Ctrl-M)

   bool isShowingMissionOverlay() const;          // Are game instructions (F2) visible?

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void onGameStarting();
   void onGameTypeChanged();

   void startLoadingLevel(bool engineerEnabled);
   void doneLoadingLevel();
   void expandDispWorldExtents(const Rect &bounds);  // Expand display extents for tile delivery

   void setAnnouncement(const string &announcement);
   void displayMessage(const Color &msgColor, const char *message);
   void onChatMessageReceived(const Color &msgColor, const char *format, ...);
   const char *getChatMessage();    // Return message being composed in in-game chat

   HelpItemManager *getHelpItemManager();
   void resetInputModeChangeAlertDisplayTimer(U32 timeInMs);

   void showLevelLoadDisplay(bool show, bool fade);
   void serverLoadedLevel(const string &levelName);

   void render();                         // Render game screen

   void renderReticle() const;            // Render crosshairs
   void renderWrongModeIndicator() const;
   void renderProgressBar() const;        // Render level-load progress bar
   void renderShutdownMessage() const;    // Render an alert if server is shutting down
   void renderLostConnectionMessage() const;
   void renderSuspendedMessage() const;
   void renderLevelUpMessage(S32 newLevel) const;
   void renderLevelListDisplayer() const;
   void renderMsgBox(const string *message, S32 msgLines) const;


   bool isChatting() const;               // Returns true if player is composing a chat message

   void resetCommandersMap();             // Turn off commander's map when connecting to server
   F32 getCommanderZoomFraction() const;

   void toggleShowingShipCoords();
   void toggleShowingObjectIds();
   void toggleShowingMeshZones();
   void toggleShowDebugBots();
   void toggleShowingMapTiles();

   void addInlineHelpItem(HelpItem item);
   void addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam);
   void removeInlineHelpItem(HelpItem item, bool markAsSeen);
   F32 getObjectiveArrowHighlightAlpha() const;

   bool isShowingInGameHelp() const;
   void setShowingInGameHelp(bool showing);
   void resetInGameHelpMessages();

   bool isShowingDebugShipCoords() const;

   // FxManager passthroughs
   void clearSparks();
   void idleFxManager(U32 timeDelta);
   void emitBlast(const Point &pos, U32 size);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos);
   void emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType);
   void emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors);
   void emitTeleportInEffect(const Point &pos, U32 type);

   void clearDisplayers();

   void renderBasicInterfaceOverlay();
   void renderLevelInfo();
   bool shouldRenderLevelInfo() const;

   void idle(U32 timeDelta);

   void resetLevelInfoDisplayTimer();

   void shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator);
   void cancelShutdown();

   // Mouse handling
   void onMouseDragged();
   void onMouseMoved();

   Point worldToScreenPoint(const Point *point,  S32 canvasWidth, S32 canvasHeight) const;
   void toggleCommanderMap();

   void onActivate();                 // Gets run when interface is first activated
   void onReactivate();               // Gets run when interface is subsequently reactivated

   void onPlayerJoined();
   void onPlayerQuit();
   void onGameOver();

   void pregameSetup(bool engineerEnabled);
   void setSelectedEngineeredObject(U32 objectType);

   void quitEngineerHelper();

   //ofstream mOutputFile;            // For saving downloaded levels
   //FILE *mOutputFile;               // For saving downloaded levels

   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);

   void onTextInput(char ascii);

   void chooseNextWeapon();
   void choosePrevWeapon();
   void selectWeapon(U32 index);    // Choose weapon by its index
   void activateModule(S32 index);  // Activate a specific module by its index
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);
   void setModulePrimary(ShipModule module, bool isActive);
   void setModuleSecondary(ShipModule module, bool isActive);

   void toggleLevelRating();

   static string getPersonalRatingString(PersonalRating rating);
   static string getTotalRatingString(S16 rating);

   // Get UI element dimensions
   S32 getLoadoutIndicatorWidth() const;
   Point getTimeLeftIndicatorWidthAndHeight() const;
   bool scoreboardIsVisible() const;

   void activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd = false);
   void exitHelper();

   // Testing methods
   bool isHelperActive(HelperMenu::HelperMenuType helperType) const;
   const HelperMenu *getActiveHelper() const;


   void renderGameNormal();         // Render game in normal play mode
   void renderGameCommander();      // Render game in commander's map mode
   void renderSuspended();          // Render suspended game

   void renderOverlayMap();         // Render the overlay map in normal play mode

   void renderEngineeredItemDeploymentMarker(Ship *ship);

   void receivedControlUpdate(bool recvd);

   bool isInScoreboardMode();

   Move *getCurrentMove();
};


}

#endif

