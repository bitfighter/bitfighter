//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIQUERYSERVERS_H_
#define _UIQUERYSERVERS_H_

#include "UI.h"
#include "UIChat.h"
#include "Point.h"
#include "Color.h"
#include "Intervals.h"

#include "MasterTypes.h"

#include "tnlNonce.h"

#include <stdarg.h>


namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

class Button
{
private:
   ClientGame *mGame;
   S32 mX, mY, mTextSize, mPadding;
   const char *mLabel;
   Color mBgColor, mFgColor, mHlColor;
   void (*mOnClickCallback)(ClientGame *);

   bool isActive() const;
   bool isMouseOver(F32 mouseX, F32 mouseY) const;

public:
   Button(ClientGame *game, S32 x, S32 y, S32 textSize, S32 padding, const char *label, Color fgColor, Color hlColor, void(*callback)(ClientGame *));   // Constructor
   virtual ~Button();

   void render(F32 mouseX, F32 mouseY) const;
   void onClick(F32 mouseX, F32 mouseY);
};


////////////////////////////////////////
////////////////////////////////////////

class QueryServersUserInterface : public UserInterface, public AbstractChat
{
   typedef UserInterface Parent;
   typedef AbstractChat ChatParent;

private:
   bool mScrollingUpMode;     // false = scrolling down, true = scrolling up
   bool mMouseAtBottomFixFactor;    // UGLY

   Vector<Button> buttons;

   S32 mPage;
   S32 mServersPerPage;
   S32 getFirstServerIndexOnCurrentPage();

   Nonce mNonce;
   bool mReceivedListOfServersFromMaster;
   U32 pendingPings;
   U32 pendingQueries;
   U32 mBroadcastPingSendTime;
   U32 mLastUsedServerId;     // A unique ID we can assign to new servers
   Timer mMasterRequeryTimer;
   U32 time;

   Timer mouseScrollTimer;
   void sortSelected();

   void contactEveryone();    // Try contacting master server, and local broadcast servers
   bool mWaitingForResponseFromMaster;
   bool mItemSelectedWithMouse;
   bool mSortAscending;
   bool mShouldSort;
   bool mAnnounced;           // Have we announced to the master that we've joined the chat room?
   bool mGivenUpOnMaster;     // Gets set to true once we start using our fallback server list

   U32 selectedId;
   S32 mSortColumn;
   S32 mHighlightColumn;
   S32 mLastSortColumn;
   bool mShowChat;
   S32 mMessageDisplayCount;  // Number of chat messages to show
   bool mJustMovedMouse;      // Track whether user is in mouse or keyboard mode
   bool mDraggingDivider;     // Track whether we are dragging the divider between chat and the servers


   void issueChat();          // Look for /commands in chat message before handing off to parent

   void recalcCurrentIndex();

   // Break up the render function a little
   void renderTopBanner();
   void renderColumnHeaders();
   void renderMessageBox(bool msg1, bool msg2);

   bool mouseInHeaderRow(const Point *pos);

   void addServersToPingList(const Vector<ServerAddr> &serverList); 
   void forgetServersNoLongerOnList(const Vector<ServerAddr> &serverListFromMaster);
   void sort();                                                

public:
   explicit QueryServersUserInterface(ClientGame *game);       // Constructor
   virtual ~QueryServersUserInterface();

   void advancePage();
   void backPage();

   enum {
      MaxPendingPings = 15,
      MaxPendingQueries = 10,
      PingQueryTimeout = 1500,
      PingQueryRetryCount = 3,
      RequeryTime = TEN_SECONDS,           // Time to refresh ping or query to game servers
      MasterRequeryTime = TEN_SECONDS,     // Time to refresh server query to master server
      CheckMasterServerReady = ONE_SECOND, // If not connected to master, check again in this time
   };

   struct ServerRef
   {
   private:
      U32 getNextId();

   public:
      enum State {
         Start,
         SentPing,
         ReceivedPing,
         SentQuery,
         ReceivedQuery,
      };

      ServerRef(S32 serverId, const Address &address, State state, bool isFromMaster); 
      virtual ~ServerRef();

      State state;
      U32 id;
      U32 pingTime;
      U32 identityToken;
      U32 lastSendTime;
      U32 sendCount;
      bool isFromMaster;      // True if remote server, false if local server
      bool dedicated;
      bool test;
      bool passwordRequired;
      bool pingTimedOut;
      bool everGotQueryResponse;
      S32 serverId;
      Nonce sendNonce;
      string serverName, serverDescr;
      Color msgColor;
      Address serverAddress;
      U32 playerCount, maxPlayers, botCount;     // U32 because that's what we use on the master

      void setNameDescr(const string &serverName, const string &serverDescr, const Color &msgColor);
      void setPlayerBotMax(U32 playerCount, U32 botCount, U32 maxPlayers);
   };

   struct ColumnInfo
   {
      const char *name;
      S32 xStart;
      ColumnInfo(const char *nm = NULL, U32 xs = 0);     // Constructor
      virtual ~ColumnInfo();
   };

   Vector<ServerRef> servers;
   string mLastSelectedServerName;
   string getLastSelectedServerName();

   Vector<ColumnInfo> columns;
   S32 getSelectedIndex();

   // Functions for handling user input
   bool onKeyDown(InputCode inputCode);
   void onTextInput(char ascii);
   void onMouseMoved();
   void onKeyUp(InputCode inputCode);
   void onMouseDragged();

   S32 getDividerPos();
   S32 getServersPerPage();
   S32 getLastPage();
   bool isMouseOverDivider();

   void onActivate();            // Run when select server screeen is displayed
   void idle(U32 t);             // Idle loop

   void render();                // Draw the screen

   // Handle responses to packets we sent
   void gotPingResponse(const Address &theAddress, const Nonce &clientNonce, U32 clientIdentityToken, S32 serverId);
   void gotQueryResponse(const Address &theAddress, S32 serverId, const Nonce &clientNonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);

   void gotServerListFromMaster(const Vector<ServerAddr> &serverList);
};


};

#endif


