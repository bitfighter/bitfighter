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

#ifndef _UIGAME_H_
#define _UIGAME_H_

#include "UI.h"
#include "gameConnection.h"
#include "quickChatHelper.h"
#include "loadoutHelper.h"
#include "engineerHelper.h"
#include "timer.h"
#include "voiceCodec.h"
#include "Point.h"
#include "Color.h"
#include "game.h"

namespace Zap
{

enum ArgTypes {
   NAME,    // Player name (can be tab-completed)
   TEAM,    // Team name (can be tab-completed)
   INT,     // Integer argument
   STR,      // String argument
   ARG_TYPES
};

enum HelpCategories {
   ADV_COMMANDS,
   LEVEL_COMMANDS,
   ADMIN_COMMANDS,
   DEBUG_COMMANDS,
   COMMAND_CATEGORIES
};

class GameUserInterface;

struct CommandInfo {
   string cmdName;
   void (*cmdCallback)(ClientGame *game, const Vector<string> &args);
   ArgTypes cmdArgInfo[9];
   S32 cmdArgCount;
   HelpCategories helpCategory;
   S32 helpGroup;
   string helpArgString[9];
   string helpTextString;
};

////////////////////////////////////////
////////////////////////////////////////

class GameUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   Move mCurrentMove;
   Move mTransformedMove;
   Point mMousePoint;

   bool mLeftDisabled;
   bool mRightDisabled;
   bool mUpDisabled;
   bool mDownDisabled;

   void disableMovementKey(KeyCode keyCode);

   // Related to display of in-game chat and status messages
   static const S32 MessageDisplayCount = 6;           // How many server messages to display
   static const S32 DisplayMessageTimeout = 3000;      // How long to display them (ms)

   static const S32 ChatMessageStoreCount = 24;        // How many chat messages to store (only top MessageDisplayCount are normally displayed)
   static const S32 ChatMessageDisplayCount = 5;       // How many chat messages to display
   static const S32 DisplayChatMessageTimeout = 4000;  // How long to display them (ms)

   enum MessageDisplayMode {
      ShortTimeout,            // Traditional message display mode (6 MessageDisplayCount lines, messages timeout after DisplayMessageTimeout)
      ShortFixed,              // Same length as ShortTimeout, but without timeout
      LongFixed,               // Long form: Display MessageStoreCount messages, no timout
      MessageDisplayModes
   };

   MessageDisplayMode mMessageDisplayMode;    // Our current message display mode

   // These are our normal server messages, that "time out"
   Color mDisplayMessageColor[MessageDisplayCount];
   char mDisplayMessage[MessageDisplayCount][MAX_CHAT_MSG_LENGTH];

   // These are our chat messages, that "time out"
   Color mDisplayChatMessageColor[ChatMessageDisplayCount];
   char mDisplayChatMessage[ChatMessageDisplayCount][MAX_CHAT_MSG_LENGTH];

   // These are only displayed in the extended chat panel, and don't time out
   Color mStoreChatMessageColor[ChatMessageStoreCount];
   char mStoreChatMessage[ChatMessageStoreCount][MAX_CHAT_MSG_LENGTH];

   Timer mDisplayMessageTimer;
   Timer mDisplayChatMessageTimer;
   Timer mShutdownTimer;

   bool mMissionOverlayActive;      // Are game instructions (F2) visible?

   enum ChatType {            // Types of in-game chat messages:
      GlobalChat,             // Goes to everyone in game
      TeamChat,               // Goes to teammates only
      CmdChat                 // Entering a command
   };

   enum ShutdownMode {
      None,                   // Nothing happening
      ShuttingDown,           // Shutting down, obviously
      Canceled                // Was shutting down, but are no longer
   };

   bool isCmdChat();          // Returns true if we're composing a command in the chat bar, false otherwise

   ChatType mCurrentChatType; // Current in-game chat mode (global or local)
   LineEditor mLineEditor;    // Message being composed

   U32 mChatCursorPos;        // Position of composition cursor

   bool mInScoreboardMode;
   ShutdownMode mShutdownMode;

   // Some rendering routines
   void renderScoreboard(const GameType *gameType);

   StringTableEntry mShutdownName;  // Name of user who iniated the shutdown
   StringPtr mShutdownReason;       // Reason user provided for the shutdown
   bool mShutdownInitiator;         // True if local client initiated shutdown (and can therefore cancel it)

   bool mGotControlUpdate;

   bool mFPSVisible;          // Are we displaying FPS info?
   U32 mRecalcFPSTimer;       // Controls recalcing FPS running average

   static const S32 WRONG_MODE_MSG_DISPLAY_TIME = 2500;
   static const S32 FPS_AVG_COUNT = 32;

   Timer mWrongModeMsgDisplay;               // Help if user is trying to use keyboard in joystick mode
   Timer mInputModeChangeAlertDisplayTimer;  // Remind user that they just changed input modes
   Timer mLevelInfoDisplayTimer;

   void renderTimeLeft(const GameType *gameType);
   void renderTalkingClients(const GameType *gameType);     // Render things related to voicechat
   void renderDebugStatus(const GameType *gameType);        // Render things related to debugging


   F32 mFPSAvg;
   F32 mPingAvg;

   U32 mIdleTimeDelta[FPS_AVG_COUNT];
   U32 mPing[FPS_AVG_COUNT];
   U32 mFrameIndex;

   // Various helper objects
   HelperMenu *mHelper;       // Current helper

   QuickChatHelper mQuickChatHelper;
   LoadoutHelper mLoadoutHelper;
   EngineerHelper mEngineerHelper;

   struct VoiceRecorder
   {
      private:
         ClientGame *mGame;

      public:
         enum {
            FirstVoiceAudioSampleTime = 250,
            VoiceAudioSampleTime = 100,
            MaxDetectionThreshold = 2048,
         };

      Timer mVoiceAudioTimer;
      RefPtr<SoundEffect> mVoiceSfx;
      RefPtr<VoiceEncoder> mVoiceEncoder;
      bool mRecordingAudio;
      U8 mWantToStopRecordingAudio;
      S32 mMaxAudioSample;
      S32 mMaxForGain;
      ByteBufferPtr mUnusedAudio;

      VoiceRecorder(ClientGame *game);
      ~VoiceRecorder();

      void idle(U32 timeDelta);
      void process();
      void start();
      void stop();
      void stopNow();
      void render();
   } mVoiceRecorder;

   void dropItem();                       // User presses drop item key

   bool mFiring;                          // Are we firing?
   bool mModActivated[ShipModuleCount];

   void setBusyChatting(bool busy);       // Tell the server we are (or are not) busy chatting

   UIMode mCurrentMode;                   // Current game mode

   static const S32 SERVER_MSG_FONT_SIZE = 14;
   static const S32 SERVER_MSG_FONT_GAP = 4;
   static const S32 CHAT_FONT_SIZE = 12;
   static const S32 CHAT_FONT_GAP = 3;
   static const S32 CHAT_MULTILINE_INDENT = 12;

public:
   GameUserInterface(ClientGame *game);    // Constructor
   virtual ~GameUserInterface();                   // Destructor

   bool displayInputModeChangeAlert;


   bool isShowingMissionOverlay()  const { return mMissionOverlayActive; }    // Are game instructions (F2) visible?

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void displayMessage(const Color &msgColor, const char *message);
   void displayMessagef(const Color &msgColor, const char *format, ...);
   void displayChatMessage(const Color &msgColor, const char *format, ...);

   void initializeLoadoutOptions(bool engineerAllowed) { mLoadoutHelper.initialize(engineerAllowed); }
   void resetInputModeChangeAlertDisplayTimer(U32 timeInMs) { mInputModeChangeAlertDisplayTimer.reset(timeInMs); }

   void render();                   // Render game screen
   void renderReticle();            // Render crosshairs
   void renderProgressBar();        // Render level-load progress bar
   void renderMessageDisplay();     // Render incoming server msgs
   void renderChatMessageDisplay(); // Render incoming chat msgs
   void renderCurrentChat();        // Render chat msg user is composing
   void renderLoadoutIndicators();  // Render indicators for the various loadout items
   void renderShutdownMessage();    // Render an alert if server is shutting down
   void renderLostConnectionMessage(); 

   void renderBasicInterfaceOverlay(const GameType *gameType, bool scoreboardVisible);

   void idle(U32 timeDelta);

   void resetLevelInfoDisplayTimer() { mLevelInfoDisplayTimer.reset(6000); }     // 6 seconds 
   void clearLevelInfoDisplayTimer() { mLevelInfoDisplayTimer.clear(); }

   void runCommand(const char *input);
   void issueChat();                // Send chat message (either Team or Global)
   void cancelChat();

   void shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator);
   void cancelShutdown();

   Vector<string> parseStringx(const char *str);    // Break a chat msg into parsable bits

   void setVolume(VolumeType volType, const Vector<string> &words);

   // Mouse handling
   void onMouseDragged(S32 x, S32 y);
   void onMouseMoved(S32 x, S32 y);
   void onMouseMoved();

   void onActivate();                 // Gets run when interface is first activated
   void onReactivate();               // Gets run when interface is subsequently reactivated

   
   //ofstream mOutputFile;            // For saving downloaded levels
   //FILE *mOutputFile;               // For saving downloaded levels

   void onKeyDown(KeyCode keyCode, char ascii);
   void onKeyUp(KeyCode keyCode);

   void processPlayModeKey(KeyCode keyCode, char ascii);
   void processChatModeKey(KeyCode keyCode, char ascii);


   void advanceWeapon();            // Choose next weapon
   void selectWeapon(U32 index);    // Choose weapon by its index

   void suspendGame();
   void unsuspendGame();

   void enterMode(UIMode mode);     // Enter QuickChat, Loadout, or Engineer mode

   void renderEngineeredItemDeploymentMarker(Ship *ship);

   void receivedControlUpdate(bool recvd) { mGotControlUpdate = recvd; }

   bool isInScoreboardMode() { return mInScoreboardMode; }

   Move *getCurrentMove();
   Timer mProgressBarFadeTimer;     // For fading out progress bar after level is loaded
   bool mShowProgressBar;

   // Message colors... (rest to follow someday)
   static Color privateF5MessageDisplayedInGameColor;


   // TODO: Move these to ClientGame???  They could really go anywhere!
   static void mVolHandler(ClientGame *game, const Vector<string> &args);    
   static void sVolHandler(ClientGame *game, const Vector<string> &args);    
   static void vVolHandler(ClientGame *game, const Vector<string> &args);    
   static void servVolHandler(ClientGame *game, const Vector<string> &args);  
   static void getMapHandler(ClientGame *game, const Vector<string> &words);
   static void nextLevelHandler(ClientGame *game, const Vector<string> &words);
   static void prevLevelHandler(ClientGame *game, const Vector<string> &words);
   static void restartLevelHandler(ClientGame *game, const Vector<string> &words);
   static void shutdownServerHandler(ClientGame *game, const Vector<string> &words);
   static void kickPlayerHandler(ClientGame *game, const Vector<string> &words);
   static void adminPassHandler(ClientGame *game, const Vector<string> &words);
   static void levelPassHandler(ClientGame *game, const Vector<string> &words);
   static void showCoordsHandler(ClientGame *game, const Vector<string> &words);
   static void showZonesHandler(ClientGame *game, const Vector<string> &words);
   static void showPathsHandler(ClientGame *game, const Vector<string> &words);
   static void pauseBotsHandler(ClientGame *game, const Vector<string> &words);
   static void stepBotsHandler(ClientGame *game, const Vector<string> &words);
   static void setAdminPassHandler(ClientGame *game, const Vector<string> &words);
   static void setServerPassHandler(ClientGame *game, const Vector<string> &words);
   static void setLevPassHandler(ClientGame *game, const Vector<string> &words);
   static void setServerNameHandler(ClientGame *game, const Vector<string> &words);
   static void setServerDescrHandler(ClientGame *game, const Vector<string> &words);
   static void serverCommandHandler(ClientGame *game, const Vector<string> &words);
   static void pmHandler(ClientGame *game, const Vector<string> &words);
   static void muteHandler(ClientGame *game, const Vector<string> &words);
   static void maxFpsHandler(ClientGame *game, const Vector<string> &words);
   static void lineSmoothHandler(ClientGame *game, const Vector<string> &words);
   static void lineWidthHandler(ClientGame *game, const Vector<string> &words);
   static void suspendHandler(ClientGame *game, const Vector<string> &words);
   static void deleteCurrentLevelHandler(ClientGame *game, const Vector<string> &words);
   static void addTimeHandler(ClientGame *game, const Vector<string> &words);
};


};

#endif

