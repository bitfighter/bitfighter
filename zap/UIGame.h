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
#include "Timer.h"
#include "ChatCommands.h"
#include "voiceCodec.h"
#include "Point.h"
#include "Color.h"
#include "Timer.h"
#include "game.h"
#include "ship.h"             // For ShipModuleCount
#include "HelperManager.h"    // For HelperManager

namespace Zap
{

class GameUserInterface;

////////////////////////////////////////
////////////////////////////////////////

struct ColorString
{
   Color color;
   string str;
   U32 groupId;

   void set(const string &s, const Color &c, U32 groupId = 0);
};


////////////////////////////////////////
////////////////////////////////////////

class ChatMessageDisplayer
{
private:
   U32 mFirst, mLast;
   bool mExpire;
   bool mTopDown;    // Render from top down or bottom up?
   S32 mWrapWidth;
   S32 mFontSize, mFontGap;
   bool mFull;       // Our message displayer is full up

   U32 mNextGroupId;


   void advanceFirst();
   void advanceLast();

   Timer mChatScrollTimer;
   Timer mDisplayChatMessageTimer;

   ClientGame *mGame;

   // These are the messages and their colors
   Vector<ColorString> mMessages;

public:
   // Constructor
   ChatMessageDisplayer(ClientGame *game, S32 msgCount, bool msgsExpire, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontGap);
   void reset();

   void idle(U32 timeDelta);
   void render(S32 ypos, bool helperVisible, bool anouncementActive, F32 alpha);   // Render incoming chat msgs

   void onChatMessageRecieved(const Color &msgColor, const string &msg);
   string substitueVars(const string &str);
};

////////////////////////////////////////
////////////////////////////////////////

class Move;
class SoundEffect;

class GameUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   enum MessageDisplayMode {
      ShortTimeout,            // Traditional message display mode (6 MessageDisplayCount lines, messages timeout after DisplayMessageTimeout)
      ShortFixed,              // Same length as ShortTimeout, but without timeout
      LongFixed,               // Long form: Display MessageStoreCount messages, no timout
      MessageDisplayModes
   };
	
   MessageDisplayMode mMessageDisplayMode;    // Our current message display mode
   void renderChatMsgs();
   void renderAnnouncement(S32 pos);

   Move mCurrentMove;
   Move mTransformedMove;
   Point mMousePoint;

   HelperManager mHelperManager;


   //Timer mDisplayMessageTimer;
   Timer mShutdownTimer;

   bool mMissionOverlayActive;      // Are game instructions (F2) visible?

   enum ShutdownMode {
      None,                   // Nothing happening
      ShuttingDown,           // Shutting down, obviously
      Canceled                // Was shutting down, but are no longer
   };

   U32 mChatCursorPos;        // Position of composition cursor

   bool mInScoreboardMode;
   ShutdownMode mShutdownMode;

   // Some rendering routines
   void renderScoreboard();

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

   void updateChatMessages(U32 timeDelta);
   bool checkEnterChatInputCode(InputCode inputCode);  // Helper for key handler

   void renderInputModeChangeAlert();
   void renderMissionOverlay(const GameType *gameType);
   void renderTeamFlagScores(const GameType *gameType, U32 rightAlignCoord);
   void renderCoreScores(const GameType *gameType, U32 rightAlignCoord);
   void renderLeadingPlayerScores(const GameType *gameType, U32 rightAlignCoord);
   void renderTimeLeft(U32 rightAlignCoord);
   void renderTalkingClients();              // Render things related to voice chat
   void renderDebugStatus();                 // Render things related to debugging

   F32 mFPSAvg;
   F32 mPingAvg;

   U32 mIdleTimeDelta[FPS_AVG_COUNT];
   U32 mPing[FPS_AVG_COUNT];
   U32 mFrameIndex;

   struct VoiceRecorder
   {
      private:
         ClientGame *mGame;

      public:
         enum {
            FirstVoiceAudioSampleTime = 250,
            VoiceAudioSampleTime      = 100,
            MaxDetectionThreshold     = 2048,
         };

      Timer mVoiceAudioTimer;
      RefPtr<SoundEffect> mVoiceSfx;
      RefPtr<VoiceEncoder> mVoiceEncoder;
      bool mRecordingAudio;
      U8 mWantToStopRecordingAudio;
      S32 mMaxAudioSample;
      S32 mMaxForGain;
      ByteBufferPtr mUnusedAudio;

      explicit VoiceRecorder(ClientGame *game);
      virtual ~VoiceRecorder();

      void idle(U32 timeDelta);
      void process();
      void start();
      void stop();
      void stopNow();
      void render();
   } mVoiceRecorder;

   ChatMessageDisplayer mServerMessageDisplayer;   // Messages from the server
   ChatMessageDisplayer mChatMessageDisplayer1;    // Short form, message expire
   ChatMessageDisplayer mChatMessageDisplayer2;    // Short form, messages do not expire
   ChatMessageDisplayer mChatMessageDisplayer3;    // Long form, messages do not expire

   Timer mAnnouncementTimer;
   string mAnnouncement;

   void dropItem();                       // User presses drop item key

   bool mFiring;                          // Are we firing?
   bool mModPrimaryActivated[ShipModuleCount];
   bool mModSecondaryActivated[ShipModuleCount];

   Timer mModuleDoubleTapTimer[ShipModuleCount];  // Timer for detecting if a module key is double-tapped
   static const S32 DoubleClickTimeout = 200;          // Timeout in milliseconds

   // Some key event handlers
   bool processPlayModeKey(InputCode inputCode);
   void checkForKeyboardMovementKeysInJoystickMode(InputCode inputCode);

public:
   explicit GameUserInterface(ClientGame *game);  // Constructor
   virtual ~GameUserInterface();                  // Destructor

   bool displayInputModeChangeAlert;

   void toggleChatDisplayMode();            // Set which chat message display mode we're in (Ctrl-M)

   bool isShowingMissionOverlay() const;    // Are game instructions (F2) visible?

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void setAnnouncement(string announcement);
   void displayMessage(const Color &msgColor, const char *message);
   void displayMessagef(const Color &msgColor, const char *format, ...);
   void onChatMessageRecieved(const Color &msgColor, const char *format, ...);
   const char *getChatMessage();    // Return message being composed in in-game chat

   void resetInputModeChangeAlertDisplayTimer(U32 timeInMs);

   void render();                   // Render game screen
  
   void renderReticle();            // Render crosshairs
   void renderProgressBar();        // Render level-load progress bar
   //void renderMessageDisplay();     // Render incoming server msgs
   void renderLoadoutIndicators();  // Render indicators for the various loadout items
   void renderShutdownMessage();    // Render an alert if server is shutting down
   void renderLostConnectionMessage(); 
   void renderSuspendedMessage();

   bool isChatting();               // Returns true if player is composing a chat message

   
   void renderBasicInterfaceOverlay(const GameType *gameType, bool scoreboardVisible);
   void renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio);

   void idle(U32 timeDelta);

   void resetLevelInfoDisplayTimer();     // 6 seconds
   void clearLevelInfoDisplayTimer();

   void shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator);
   void cancelShutdown();

   // Mouse handling
   void onMouseDragged();
   void onMouseMoved();

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

   void suspendGame();
   void unsuspendGame();

   void activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd = false);  
   void exitHelper();

   void renderEngineeredItemDeploymentMarker(Ship *ship);

   void receivedControlUpdate(bool recvd);

   bool isInScoreboardMode();

   Move *getCurrentMove();
   Timer mProgressBarFadeTimer;     // For fading out progress bar after level is loaded
   bool mShowProgressBar;

   // Message colors... (rest to follow someday)
   static Color privateF5MessageDisplayedInGameColor;
};


};

#endif

