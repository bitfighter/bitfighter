//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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
#include "quickChat.h"
#include "loadoutSelect.h"
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
      LongFixed,               // Longform: Display MessageStoreCount messages, no timout
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

   enum ChatType {            // Types of in-game chat messages
      GlobalChat,
      TeamChat,
   };
   ChatType mCurrentChatType; // Current in-game chat mode (global or local)
   char mChatBuffer[128];     // Message being composed
   U32 mChatCursorPos;        // Position of composition cursor

   bool mInScoreboardMode;
   bool mGotControlUpdate;

   bool mFPSVisible;          // Are we displaying FPS info?
   U32 mRecalcFPSTimer;       // Controls recalcing FPS running average

   enum {
      FPSAvgCount = 32,
   };
   F32 mFPSAvg;


   enum {
      WrongModeMsgDisplayTime = 2500,
   };
   Timer mWrongModeMsgDisplay;       // Help if user is trying to use keyboard in joystick mode

   U32 mIdleTimeDelta[FPSAvgCount];
   U32 mFrameIndex;

   QuickChatHelper mQuickChat;
   LoadoutHelper mLoadout;

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

   void enterQuickChat();
   void enterLoadout();

   bool mFiring;                          // Are we firing?
   bool mModActivated[ShipModuleCount];   

public:

   bool displayInputModeChangeAlert;
   bool mMissionOverlayActive;      // Are game instructions (F2) visible?

   GameUserInterface();

   void displayMessage(Color messageColor, const char *format, ...);

   void render();                   // Render game screen
   void renderReticle();            // Render crosshairs
   void renderMessageDisplay();     // Render incoming chat msgs
   void renderCurrentChat();        // Render chat msg user is composing
   void renderLoadoutIndicators();  // Render indicators for the various loadout items

   void idle(U32 timeDelta);

   void issueChat();                // Send chat message (either Team or Global)
   void cancelChat();

   void onMouseMoved(S32 x, S32 y);
   void onMouseDragged(S32 x, S32 y);
   void onActivate();               // Gets run when interface is first activated
   void onReactivate();             // Gets run when interface is subsequently reactivated


   void onKeyDown(KeyCode keyCode, char ascii);
   void onKeyUp(KeyCode keyCode);

   void advanceWeapon();         // Choose next weapon
   void selectWeapon(U32 index); // Choose weapon by its index

   // Modes we could be in during the game
   enum Mode {
      PlayMode,               // Playing
      ChatMode,               // Composing chat message
      QuickChatMode,          // In quick-chat menu
      LoadoutMode,            // In loadout menu
   };
   Mode mCurrentMode;         // Current game mode
   void setPlayMode();        // Set mode to PlayMode

   void receivedControlUpdate(bool recvd) { mGotControlUpdate = recvd; }

   Move *getCurrentMove();
};

extern GameUserInterface gGameUserInterface;

};

#endif
