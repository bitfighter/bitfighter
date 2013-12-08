//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_MANAGER_H_
#define _UI_MANAGER_H_

#include "SoundEffect.h"

#include "SoundSystemEnums.h"
#include "SparkTypesEnum.h"
#include "PlayerActionEnum.h"

#include "game.h"
#include "HelpItemManager.h"     // For HelpItem def
#include "LoadoutTracker.h"

#include "Color.h"
#include "Point.h"
#include "Timer.h"

#include "tnlNetStringTable.h"
#include "tnlNetConnection.h"
#include "tnlNonce.h"
#include "tnlString.h"
#include "tnlUDP.h"
#include "tnlVector.h"

#include <memory>
#include <typeinfo>

using namespace std;
using namespace TNL;

namespace Zap
{

class Move;
class Game;
class ClientGame;
class UserInterface;
class GameSettings;

class UIManager
{
public:
   static const S32 MessageBoxWrapWidth;

private:
   typedef map<const type_info *, UserInterface *> UiMapType;
   typedef UiMapType::iterator UiIterator;

   UiMapType mUis;

protected:
   ClientGame *mGame;
   Vector<UserInterface *> mPrevUIs;         // Previously active menus
   UserInterface *mCurrentInterface;

public:
   UIManager();            // Constructor
   virtual ~UIManager();   // Destructor


   template <typename T>
   const std::type_info *getTypeInfo()
   {
      static const std::type_info *typeinfo = &typeid(T);
      return typeinfo;
   }


   template <typename T>
   T *getUI()
   {
      const std::type_info *typeinfo = getTypeInfo<T>();

      T *ui = static_cast<T *>(mUis[typeinfo]);

      // Lazily initialize if UI has not yet been instantiated; store for later use
      if(!ui)  
      {
         ui = new T(mGame);
         mUis[typeinfo] = ui;
      }

      return ui;
   }


   template <typename T>
   bool isCurrentUI()
   {
      return mCurrentInterface == mUis[getTypeInfo<T>()];
   }


   // Did we arrive at our current interface via the specified interface?
   template <typename T>
   bool cameFrom()
   {
      for(S32 i = 0; i < mPrevUIs.size(); i++)
         if(mPrevUIs[i] == mUis[getTypeInfo<T>()])
            return true;

      return false;
   }


   template <typename T>
   void activate(bool save = true)
   {
      activate(getUI<T>(), save);
   }


private:
   GameSettings *mSettings;
   bool mUserHasSeenTimeoutMessage;

   UserInterface *mLastUI;             // Menu most immediately active before mCurrentUI
   bool mLastWasLower;                 // True if mLastUI was lower in the hierarchy than mCurrentUI

   Timer mMenuTransitionTimer;

   // Sounds
   MusicLocation selectMusic();
   void processAudio(U32 timeDelta);

public:
   void setClientGame(ClientGame *clientGame);
   void reactivatePrevUI();
   void reactivate(const UserInterface *ui);
   
   void activateGameUI();
   void reactivateGameUI(); 

   bool hasPrevUI();
   void clearPrevUIs();
   void renderPrevUI(const UserInterface *ui);


   void saveUI(UserInterface *ui);

   void renderCurrent();
   UserInterface *getCurrentUI();
   UserInterface *getPrevUI();

   void activate(UserInterface *ui, bool save = true);

   void idle(U32 timeDelta);

   void renderAndDimGameUserInterface();

   // Connecting and disconnecting
   void onConnectedToMaster();
   void onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr, bool wasFullyConnected);
   void onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr);
   void onConnectionToServerRejected(const char *reason);

   void onPlayerJoined(const char *playerName, bool isLocalClient, bool playAlert, bool showMessage);
   void onPlayerQuit(const char *name);
   void displayMessage(const Color &msgColor, const char *format, ...);

   void onGameStarting();
   void onGameOver();

   // Sounds and music
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position) const;
   SFXHandle playSoundEffect(U32 profileIndex, const Point &position, const Point &velocity, F32 gain) const;
   SFXHandle playSoundEffect(U32 profileIndex, F32 gain) const;
   void setMovementParams(SFXHandle& effect, const Point &position, const Point &velocity) const;
   void stopSoundEffect(SFXHandle &effect) const;
   void setListenerParams(const Point &position, const Point &velocity) const;
   void playNextTrack() const;
   void playPrevTrack() const;
   void queueVoiceChatBuffer(const SFXHandle &effect, const ByteBufferPtr &p) const;


   // Some passthroughs

   // HighScores UI:
   void setHighScores(const Vector<StringTableEntry> &groupNames, const Vector<string> &names, const Vector<string> &scores);

   // ChatUI:
   void gotGlobalChatMessage(const string &from, const string &message, bool isPrivate, bool isSystem, bool fromSelf);
   void setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks);
   void playerJoinedGlobalChat(const StringTableEntry &playerNick);
   void playerLeftGlobalChat  (const StringTableEntry &playerNick);

   // QueryServersUI:
   void gotServerListFromMaster(const Vector<IPAddress> &serverList);
   void gotPingResponse (const Address &address, const Nonce &nonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &address, const Nonce &nonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);
   string getLastSelectedServerName();

   // ServerPasswordUI
   void setConnectAddressAndActivatePasswordEntryUI(const Address &serverAddress);

   // GameUI
   void activateGameUserInterface();
   void renderLevelListDisplayer();
   void enableLevelLoadDisplay();
   void serverLoadedLevel(const string &levelName, Game::HostingModePhase phase);
   void disableLevelLoadDisplay(bool fade);
   void gotPasswordOrPermissionsReply(const ClientGame *game, const char *message);

   // MainMenuUi
   void setMOTD(const char *motd);
   void setNeedToUpgrade(bool needToUpgrade);

   // PlayerUI
   void showPlayerActionMenu(PlayerAction action);

   // TeamMenuUI
   void showMenuToChangeTeamForPlayer(const string &playerName);

   // ErrorMessageUI
   void displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, const Vector<StringTableEntry> &messages);
   void displayMessageBox(const string &title, const string &instr, const Vector<string> &messages);
   void displayMessageBox(const string &title, const string &instr, const string &message);

   // GameUI
   void startLoadingLevel(bool engineerEnabled);
   void doneLoadingLevel();
   void clearSparks();
   void emitBlast(const Point &pos, U32 size);
   void emitBurst(const Point &pos, const Point &scale, const Color &color1, const Color &color2);
   void emitDebrisChunk(const Vector<Point> &points, const Color &color, const Point &pos, const Point &vel, S32 ttl, F32 angle, F32 rotation);
   void emitTextEffect(const string &text, const Color &color, const Point &pos);
   void emitSpark(const Point &pos, const Point &vel, const Color &color, S32 ttl, UI::SparkType sparkType);
   void emitExplosion(const Point &pos, F32 size, const Color *colorArray, U32 numColors);
   void emitTeleportInEffect(const Point &pos, U32 type);
   
   void addInlineHelpItem(HelpItem item);
   void addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam);
   void removeInlineHelpItem(HelpItem item, bool markAsSeen);
   F32 getObjectiveArrowHighlightAlpha();
   bool isShowingInGameHelp();

   void onChatMessageReceived(const Color &msgColor, const char *format, ...);
   void gotAnnouncement(const string &announcement);
   bool isInScoreboardMode();
   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);
   bool isShowingDebugShipCoords();
   Point worldToScreenPoint(const Point *point, S32 canvasWidth, S32 canvasHeight);
   void resetCommandersMap();
   F32 getCommanderZoomFraction();
   void renderBasicInterfaceOverlay();
   void quitEngineerHelper();
   void exitHelper();
   void shutdownInitiated(U16 time, const StringTableEntry &name, const StringPtr &reason, bool originator);
   void cancelShutdown();
   void setSelectedEngineeredObject(U32 objectType);
   Move *getCurrentMove();
   void displayErrorMessage(const char *message);
   void displaySuccessMessage(const char *message);
   void setShowingInGameHelp(bool showing);
   void resetInGameHelpMessages();

   // EditorUI
   void readRobotLine(const string &robotLine);

};


};

#endif

