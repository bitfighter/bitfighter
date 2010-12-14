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

#ifndef _UIQUERYSERVERS_H_
#define _UIQUERYSERVERS_H_

#include "UI.h"
#include "UIChat.h"
#include "point.h"

#include "../tnl/tnlNonce.h"

#include <stdarg.h>


namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

class Button
{
private:
   S32 mX, mY, mTextSize, mPadding;
   const char *mLabel;
   void (*mOnClickCallback)();
   bool mouseOver(S32 mouseX, S32 mouseY);
   bool mTransparent;
   Color mBgColor, mFgColor, mHlColor;

public:
   Button(S32 x, S32 y, S32 textSize, S32 padding, const char *label, Color fgColor, Color hlColor, void(*callback)());   // Constructor

   void render(S32 mouseX, S32 mouseY);
   void onClick(S32 mouseX, S32 mouseY);
};


////////////////////////////////////////
////////////////////////////////////////

class QueryServersUserInterface : public UserInterface, public AbstractChat
{
   typedef AbstractChat ChatParent;

private:
   bool mScrollingUpMode;     // false = scrolling down, true = scrolling up
   bool mMouseAtBottomFixFactor;    // UGLY

   Vector<Button> buttons;

   S32 mPage;
   S32 mServersPerPage;
   S32 getFirstServerIndexOnCurrentPage() { return mPage * mServersPerPage; }
   S32 getLastPage() { return (servers.size() - 1) / mServersPerPage; }

   Timer mouseScrollTimer;
   void sortSelected();

   void contactEveryone();    // Try contacting master server, and local broadcast servers
   bool mWaitingForResponseFromMaster;
   bool mItemSelectedWithMouse;
   bool mSortAscending;
   bool mShouldSort;
   bool mAnnounced;           // Have we announced to the master that we've joined the chat room?
   bool mGivenUpOnMaster;     // Gets set to true once we start using our fallback server list

   S32 selectedId;
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

public:
   QueryServersUserInterface();      // Constructor
   bool mRecievedListOfServersFromMaster;
   Nonce mNonce;
   U32 pendingPings;
   U32 pendingQueries;
   U32 mBroadcastPingSendTime;
   U32 mLastUsedServerId;     // A unique ID we can assign to new servers
   Timer mMasterRequeryTimer;
   U32 time;

   static const S32 MaxServerNameLen = 40;
   static const S32 MaxServerDescrLen = 254;

   void advancePage();
   void backPage();

   enum {
      MaxPendingPings = 15,
      MaxPendingQueries = 10,
      PingQueryTimeout = 1500,
      PingQueryRetryCount = 3,
      RequeryTime = 10000,           // Time to refresh ping or query to game servers
      MasterRequeryTime = 10000,     // Time to refresh server query to master server
      CheckMasterServerReady = 1000, // If not connected to master, check again in this time
   };
   struct ServerRef
   {
      enum State
      {
         Start,
         SentPing,
         ReceivedPing,
         SentQuery,
         ReceivedQuery,
      };
      U32 state;
      U32 id;
      U32 pingTime;
      U32 identityToken;
      U32 lastSendTime;
      U32 sendCount;
      bool isFromMaster;
      bool dedicated;
      bool test;
      bool passwordRequired;
      bool pingTimedOut;
      bool everGotQueryResponse;
      Nonce sendNonce;
      string serverName;
      string serverDescr;
      Color msgColor;
      Address serverAddress;
      U32 playerCount, maxPlayers, botCount;     // U32 because that's what we use on the master

      ServerRef() // Quickie constructor
      {
         pingTimedOut = false;
         everGotQueryResponse = false;
         passwordRequired = false;
         test = false;
         dedicated = false;
         sendCount = 0;
         pingTime = 9999;
         playerCount = -1;
         maxPlayers = -1;
         botCount = -1;
      }
   };
   struct ColumnInfo
   {
      const char *name;
      S32 xStart;
      ColumnInfo(const char *nm = NULL, U32 xs = 0) { name = nm; xStart = xs; }     // Constructor
   };

   struct HiddenServer
   {
      U32 timeUntilShow;
      Address serverAddress;
      HiddenServer(Address addr, U32 time) { serverAddress = addr; timeUntilShow = time; }

   };

   Vector<ServerRef> servers;
   ServerRef mLastSelectedServer;
   string getLastSelectedServerName() { return mLastSelectedServer.serverName; }

   Vector<ColumnInfo> columns;
   Vector<HiddenServer> hidden;
   S32 getSelectedIndex();

   // Functions for handling user input
   void onKeyDown(KeyCode keyCode, char ascii);
   void onMouseMoved(S32 x, S32 y);
   void onKeyUp(KeyCode keyCode);
   void onMouseDragged(S32 x, S32 y);

   S32 getDividerPos();
   S32 getServersPerPage();
   bool isMouseOverDivider();

   void onActivate();         // Run when select server screeen is displayed
   void idle(U32 t);          // Idle loop

   void render();             // Draw the screen

   void addPingServers(const Vector<IPAddress> &ipList);    // Add many addresses
   void addHiddenServer(Address addr, U32 time);            // Add server to list of servers we don't show the user


   void sort();               // Sort servers for pretty viewing

   // Handle responses to packets we sent
   void gotPingResponse(const Address &theAddress, const Nonce &clientNonce, U32 clientIdentityToken);
   void gotQueryResponse(const Address &theAddress, const Nonce &clientNonce, const char *serverName, const char *serverDescr, 
                         U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired);
};

extern QueryServersUserInterface gQueryServersUserInterface;

};

#endif


