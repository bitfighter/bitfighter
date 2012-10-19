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
#include "voiceCodec.h"
#include "Point.h"
#include "Color.h"
#include "Timer.h"
#include "game.h"
#include "ship.h"          // For ShipModuleCount

namespace Zap
{

enum ArgTypes {
   NAME,    // Player name (can be tab-completed)
   TEAM,    // Team name (can be tab-completed)
   xINT,     // Integer argument
   STR,     // String argument
   PT,      // Point argument (only used by Lua scripts)
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

const S32 MAX_CMDS = 9;

struct CommandInfo 
{
   string cmdName;
   void (GameUserInterface::*cmdCallback)(const Vector<string> &args);
   ArgTypes cmdArgInfo[MAX_CMDS];
   S32 cmdArgCount;
   HelpCategories helpCategory;
   S32 helpGroup;
   S32 lines;                    // # lines required to display help (usually 1, occasionally 2)
   string helpArgString[MAX_CMDS];
   string helpTextString;
};


////////////////////////////////////////
////////////////////////////////////////

struct ColorString
{
   Color color;
   string str;

   void clear();
   void set(const string &s, const Color &c);

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
      void render(S32 ypos, bool helperVisible);   // Render incoming chat msgs

      void onChatMessageRecieved(const Color &msgColor, const string &msg);
      string substitueVars(const string &str);
};

////////////////////////////////////////
////////////////////////////////////////

class HelperMenu;
class QuickChatHelper;
class LoadoutHelper;
class EngineerHelper;
class TeamShuffleHelper;
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

   Move mCurrentMove;
   Move mTransformedMove;
   Point mMousePoint;

   //Timer mDisplayMessageTimer;
   Timer mShutdownTimer;

   bool mMissionOverlayActive;      // Are game instructions (F2) visible?

   enum ChatType {            // Types of in-game chat messages:
      GlobalChat,             // Goes to everyone in game
      TeamChat,               // Goes to teammates only
      CmdChat,                // Entering a command
      NoChat                  // None of the above
   };

   enum ShutdownMode {
      None,                   // Nothing happening
      ShuttingDown,           // Shutting down, obviously
      Canceled                // Was shutting down, but are no longer
   };

   bool isCmdChat();          // Returns true if we're composing a command in the chat bar, false otherwise

   ChatType mCurrentChatType; // Current in-game chat mode (global or local)
   UIMode mCurrentUIMode;          

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

   // Various helper objects
   HelperMenu *mHelper;       // Current helper

   QuickChatHelper *mQuickChatHelper;
   LoadoutHelper *mLoadoutHelper;
   EngineerHelper *mEngineerHelper;
   TeamShuffleHelper *mTeamShuffleHelper;


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

   LineEditor mLineEditor;    // Message being composed

   ChatMessageDisplayer mServerMessageDisplayer;   // Messages from the server
   ChatMessageDisplayer mChatMessageDisplayer1;    // Short form, message expire
   ChatMessageDisplayer mChatMessageDisplayer2;    // Short form, messages do not expire
   ChatMessageDisplayer mChatMessageDisplayer3;    // Long form, messages do not expire

   void dropItem();                       // User presses drop item key

   bool mFiring;                          // Are we firing?
   bool mModPrimaryActivated[ShipModuleCount];
   bool mModSecondaryActivated[ShipModuleCount];

   void setBusyChatting(bool busy);       // Tell the server we are (or are not) busy chatting

   Timer mModuleDoubleTapTimer[ShipModuleCount];  // Timer for detecting if a module key is double-tapped
   static const S32 DoubleClickTimeout = 200;          // Timeout in milliseconds

public:
   GameUserInterface(ClientGame *game);           // Constructor
   virtual ~GameUserInterface();                  // Destructor

   bool displayInputModeChangeAlert;

   void toggleChatDisplayMode();            // Set which chat message display mode we're in (Ctrl-M)

   bool isShowingMissionOverlay() const;    // Are game instructions (F2) visible?

   void displayErrorMessage(const char *format, ...);
   void displaySuccessMessage(const char *format, ...);

   void displayMessage(const Color &msgColor, const char *message);
   void displayMessagef(const Color &msgColor, const char *format, ...);
   void onChatMessageRecieved(const Color &msgColor, const char *format, ...);

   void resetInputModeChangeAlertDisplayTimer(U32 timeInMs);

   void render();                   // Render game screen
   void renderReticle();            // Render crosshairs
   void renderProgressBar();        // Render level-load progress bar
   //void renderMessageDisplay();     // Render incoming server msgs
   void renderCurrentChat();        // Render chat msg user is composing
   void renderLoadoutIndicators();  // Render indicators for the various loadout items
   void renderShutdownMessage();    // Render an alert if server is shutting down
   void renderLostConnectionMessage(); 
   void renderSuspendedMessage();

   void renderBasicInterfaceOverlay(const GameType *gameType, bool scoreboardVisible);
   void renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio);

   void idle(U32 timeDelta);

   void resetLevelInfoDisplayTimer();     // 6 seconds
   void clearLevelInfoDisplayTimer();

   void runCommand(const char *input);
   void issueChat();                // Send chat message (either Team or Global)
   void cancelChat();

   void shutdownInitiated(U16 time, const StringTableEntry &who, const StringPtr &why, bool initiator);
   void cancelShutdown();

   Vector<string> parseStringx(const char *str);    // Break a chat msg into parsable bits

   void setVolume(VolumeType volType, const Vector<string> &words);

   // Mouse handling
   void onMouseDragged();
   void onMouseMoved();

   void onActivate();                 // Gets run when interface is first activated
   void onReactivate();               // Gets run when interface is subsequently reactivated


   QuickChatHelper   *getQuickChatHelper(ClientGame *game);
   LoadoutHelper     *getLoadoutHelper(ClientGame *game);
   EngineerHelper    *getEngineerHelper(ClientGame *game);
   TeamShuffleHelper *getTeamShuffleHelper(ClientGame *game);

   void quitEngineerHelper();

   //ofstream mOutputFile;            // For saving downloaded levels
   //FILE *mOutputFile;               // For saving downloaded levels

   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onTextInput(char ascii);

   bool processPlayModeKey(InputCode inputCode);
   bool processChatModeKey(InputCode inputCode);

   void chooseNextWeapon();           
   void choosePrevWeapon();   
   void selectWeapon(U32 index);    // Choose weapon by its index
   void activateModule(S32 index);  // Activate a specific module by its index

   void suspendGame();
   void unsuspendGame();

   void enterMode(UIMode mode);     // Enter QuickChat, Loadout, or Engineer mode
   UIMode getUIMode();

   void renderEngineeredItemDeploymentMarker(Ship *ship);

   void receivedControlUpdate(bool recvd);

   bool isInScoreboardMode();

   Move *getCurrentMove();
   Timer mProgressBarFadeTimer;     // For fading out progress bar after level is loaded
   bool mShowProgressBar;

   // Message colors... (rest to follow someday)
   static Color privateF5MessageDisplayedInGameColor;


   // TODO: Move these to ClientGame???  They could really go anywhere!
   void mVolHandler(const Vector<string> &args);    
   void sVolHandler(const Vector<string> &args);    
   void vVolHandler(const Vector<string> &args);
   void servVolHandler(const Vector<string> &args);
   void mNextHandler(const Vector<string> &words);
   void mPrevHandler(const Vector<string> &words);
   void getMapHandler(const Vector<string> &words);
   void nextLevelHandler(const Vector<string> &words);
   void prevLevelHandler(const Vector<string> &words);
   void restartLevelHandler(const Vector<string> &words);
   void randomLevelHandler(const Vector<string> &words);
   void shutdownServerHandler(const Vector<string> &words);
   void kickPlayerHandler(const Vector<string> &words);
   void submitPassHandler(const Vector<string> &words);
   void showCoordsHandler(const Vector<string> &words);
   void showZonesHandler(const Vector<string> &words);
   void showPathsHandler(const Vector<string> &words);
   void pauseBotsHandler(const Vector<string> &words);
   void stepBotsHandler(const Vector<string> &words);
   void setAdminPassHandler(const Vector<string> &words);
   void setServerPassHandler(const Vector<string> &words);
   void setLevPassHandler(const Vector<string> &words);
   void setServerNameHandler(const Vector<string> &words);
   void setServerDescrHandler(const Vector<string> &words);
   void setLevelDirHandler(const Vector<string> &words);
   void serverCommandHandler(const Vector<string> &words);
   void pmHandler(const Vector<string> &words);
   void muteHandler(const Vector<string> &words);
   void voiceMuteHandler(const Vector<string> &words);
   void maxFpsHandler(const Vector<string> &words);
   void lagHandler(const Vector<string> &words);
   void clearCacheHandler(const Vector<string> &words);
   void lineWidthHandler(const Vector<string> &words);
   void idleHandler(const Vector<string> &words);
   void suspendHandler(const Vector<string> &words);
   void showPresetsHandler(const Vector<string> &words);
   void deleteCurrentLevelHandler(const Vector<string> &words);
   void addTimeHandler(const Vector<string> &words);
   void setTimeHandler(const Vector<string> &words);
   void setWinningScoreHandler(const Vector<string> &words);
   void resetScoreHandler(const Vector<string> &words);
   void addBotHandler(const Vector<string> &words);
   void addBotsHandler(const Vector<string> &words);
   void kickBotHandler(const Vector<string> &words);
   void kickBotsHandler(const Vector<string> &words);
   void showBotsHandler(const Vector<string> &words);
   void setMaxBotsHandler(const Vector<string> &words);
   void banPlayerHandler(const Vector<string> &words);
   void banIpHandler(const Vector<string> &words);
   void renamePlayerHandler(const Vector<string> &words);
   void globalMuteHandler(const Vector<string> &words);
   void shuffleTeams(const Vector<string> &words);

   bool isHelperActive();
   bool isChatting();
};


};

#endif

