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
#include "sfx.h"
#include "voiceCodec.h"
#include "point.h"

namespace Zap
{

class GameUserInterface : public UserInterface
{
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
   enum {
      MessageStoreCount = 20,          // How many messages to store (only top MessageDisplayCount are normally displayed)
      MessageDisplayCount = 6,         // How many messages to display
      DisplayMessageTimeout = 3000,    // How long to display them (ms)
   };

   enum MessageDisplayMode {
      ShortTimeout,            // Traditional message display mode (6 MessageDisplayCount lines, messages timeout after DisplayMessageTimeout)
      ShortFixed,              // Same length as ShortTimeout, but without timeout
      LongFixed,               // Long form: Display MessageStoreCount messages, no timout
      MessageDisplayModes
   };

   MessageDisplayMode mMessageDisplayMode;    // Our current message display mode

   // These are our normal chat messages, that "time out"
   Color mDisplayMessageColor[MessageDisplayCount];
   char mDisplayMessage[MessageDisplayCount][MAX_CHAT_MSG_LENGTH];

   // These are only displayed in the extended chat panel, and don't time out
   Color mStoreMessageColor[MessageStoreCount];
   char mStoreMessage[MessageStoreCount][MAX_CHAT_MSG_LENGTH];

   Timer mDisplayMessageTimer;
   Timer mShutdownTimer;

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

   StringTableEntry mShutdownName;  // Name of user who iniated the shutdown
   StringPtr mShutdownReason;       // Reason user provided for the shutdown
   bool mShutdownInitiator;         // True if local client initiated shutdown (and can therefore cancel it)

   bool mGotControlUpdate;

   bool mFPSVisible;          // Are we displaying FPS info?
   U32 mRecalcFPSTimer;       // Controls recalcing FPS running average

   enum {
      WrongModeMsgDisplayTime = 2500,
      FPSAvgCount = 32,
   };
   Timer mWrongModeMsgDisplay;       // Help if user is trying to use keyboard in joystick mode
   F32 mFPSAvg;
   F32 mPingAvg;

   U32 mIdleTimeDelta[FPSAvgCount];
   U32 mPing[FPSAvgCount];
   U32 mFrameIndex;

   // Various helper objects
   HelperMenu *mHelper;       // Current helper
   QuickChatHelper mQuickChatHelper;
   LoadoutHelper mLoadoutHelper;
   EngineerHelper mEngineerHelper;


   struct VoiceRecorder
   {
      enum {
         FirstVoiceAudioSampleTime = 250,
         VoiceAudioSampleTime = 100,
         MaxDetectionThreshold = 2048,
      };

      Timer mVoiceAudioTimer;
      RefPtr<SFXObject> mVoiceSfx;
      RefPtr<VoiceEncoder> mVoiceEncoder;
      bool mRecordingAudio;
      S32 mMaxAudioSample;
      S32 mMaxForGain;
      ByteBufferPtr mUnusedAudio;

      VoiceRecorder();

      void idle(U32 timeDelta);
      void process();
      void start();
      void stop();
      void render();

   } mVoiceRecorder;

   void dropItem();                       // User presses drop item key

   bool mFiring;                          // Are we firing?
   bool mModActivated[ShipModuleCount];

   void setBusyChatting(bool busy);     // Tell the server we are (or are not) busy chatting

   enum VolumeType {
      SfxVolumeType,
      MusicVolumeType,
      VoiceVolumeType,
      ServerAlertVolumeType,
   };

   Vector<string> mChatCmds;        // List of all commands we can type at chat prompt, for <tab> completion

public:
   GameUserInterface();             // Constructor

   bool displayInputModeChangeAlert;
   bool mMissionOverlayActive;      // Are game instructions (F2) visible?
   bool mDebugShowShipCoords;       // Show coords on ship?
   bool mDebugShowMeshZones;        // Show bot nav mesh zones?

   void displayErrorMessage(const char *format, ...);
   void displayMessage(const Color &msgColor, const char *format, ...);


   void initializeLoadoutOptions(bool engineerAllowed) { mLoadoutHelper.initialize(engineerAllowed); }

   void render();                   // Render game screen
   void renderReticle();            // Render crosshairs
   void renderProgressBar();        // Render level-load progress bar
   void renderMessageDisplay();     // Render incoming chat msgs
   void renderCurrentChat();        // Render chat msg user is composing
   void renderLoadoutIndicators();  // Render indicators for the various loadout items
   void renderShutdownMessage();    // Render an alert if server is shutting down

   void idle(U32 timeDelta);

   void issueChat();                // Send chat message (either Team or Global)
   void cancelChat();

   void shutdownInitiated(U16 time, StringTableEntry who, StringPtr why, bool initiator);
   void shutdownCanceled();

   Vector<string> parseString(const char *str);    // Break a chat msg into parsable bits
   bool processCommand(Vector<string> &words);     // Process a cmd entered into the chat interface
   void populateChatCmdList();                     // Add all our chat cmds to a list for autocompletion purposes

   void setVolume(VolumeType volType, Vector<string> &words);

   // Mouse handling
   void onMouseDragged(S32 x, S32 y);
   void onMouseMoved(S32 x, S32 y);
   void onMouseMoved();

   void onActivate();               // Gets run when interface is first activated
   void onReactivate();             // Gets run when interface is subsequently reactivated

   string remoteLevelDownloadFilename;
   //ofstream mOutputFile;            // For saving downloaded levels
   FILE *mOutputFile;               // For saving downloaded levels

   void onKeyDown(KeyCode keyCode, char ascii);
   void onKeyUp(KeyCode keyCode);

   void advanceWeapon();            // Choose next weapon
   void selectWeapon(U32 index);    // Choose weapon by its index

   void suspendGame();
   void unsuspendGame();

   // Modes we could be in during the game
   enum Mode {
      PlayMode,               // Playing
      ChatMode,               // Composing chat message
      QuickChatMode,          // In quick-chat menu
      LoadoutMode,            // In loadout menu
      EngineerMode,           // In engineer overlay mode
   };

   void enterMode(GameUserInterface::Mode mode);      // Enter QuickChat, Loadout, or Engineer mode

   Mode mCurrentMode;              // Current game mode
   void setPlayMode();             // Set mode to PlayMode

   void renderEngineeredItemDeploymentMarker(Ship *ship);

   void receivedControlUpdate(bool recvd) { mGotControlUpdate = recvd; }

   bool isInScoreboardMode() { return mInScoreboardMode; }

   Move *getCurrentMove();
   Timer mProgressBarFadeTimer;     // For fading out progress bar after level is loaded
   bool mShowProgressBar;

   // Message colors... (rest to follow someday)
   static Color privateF5MessageDisplayedInGameColor;
};

extern GameUserInterface gGameUserInterface;

};

#endif

