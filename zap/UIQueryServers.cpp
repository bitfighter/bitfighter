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

#include "UIQueryServers.h"

#include "UIMenus.h"
#include "UIManager.h"

#include "masterConnection.h"
#include "gameNetInterface.h"
#include "ClientGame.h"
#include "Colors.h"
#include "ScreenInfo.h"
#include "gameObjectRender.h"
#include "Cursor.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"

#include "tnlRandom.h"

#include <math.h>

namespace Zap
{


static const U32 GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME = 8000;      // 8 seconds should be enough time to connect to the master

// Text sizes and the like
static const S32 COLUMN_HEADER_TEXTSIZE = 14;     // Size of text in column headers
static const S32 SERVER_DESCR_TEXTSIZE = 18;    // Size of lower description of selected server
static const S32 SERVER_ENTRY_TEXTSIZE = 14;
static const S32 SERVER_ENTRY_VERT_GAP = 4; 
static const S32 SERVER_ENTRY_HEIGHT = SERVER_ENTRY_TEXTSIZE + SERVER_ENTRY_VERT_GAP;
static const S32 SEL_SERVER_INSTR_SIZE = 18;    // Size of "UP, DOWN TO SELECT..." text
static const S32 SEL_SERVER_INSTR_GAP_ABOVE_DIVIDER_LINE = 10;

// Positions of things on the screen
static const U32 BANNER_HEIGHT = 76;  // Height of top green banner area
static const U32 COLUMN_HEADER_TOP = BANNER_HEIGHT + 1;
static const U32 COLUMN_HEADER_HEIGHT = COLUMN_HEADER_TEXTSIZE + 6;

static const U32 TOP_OF_SERVER_LIST = BANNER_HEIGHT + COLUMN_HEADER_TEXTSIZE + 9;     // 9 provides some visual padding  [99]
static const U32 AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER = (SEL_SERVER_INSTR_SIZE + SERVER_DESCR_TEXTSIZE + SERVER_ENTRY_VERT_GAP + 10);

// "Chat window" includes chat composition, but not list of names of people in chatroom
#define BOTTOM_OF_CHAT_WINDOW (gScreenInfo.getGameCanvasHeight() - vertMargin / 2 - CHAT_NAMELIST_SIZE)


// Button callbacks
static void nextButtonClickedCallback(ClientGame *game)
{
   game->getUIManager()->getUI<QueryServersUserInterface>()->advancePage();
}


static void prevButtonClickedCallback(ClientGame *game)
{
   game->getUIManager()->getUI<QueryServersUserInterface>()->backPage();
}


// Constructor
QueryServersUserInterface::ServerRef::ServerRef()
{
   pingTimedOut = false;
   everGotQueryResponse = false;
   passwordRequired = false;
   test = false;
   dedicated = false;
   isFromMaster = false;
   sendCount = 0;
   pingTime = 9999;
   playerCount = -1;
   maxPlayers = -1;
   botCount = -1;

   id = 0;
   identityToken = 0;
   lastSendTime = 0;
   state = Start;
}

// Destructor
QueryServersUserInterface::ServerRef::~ServerRef()
{
   // Do nothing
}


// Constructor
QueryServersUserInterface::ColumnInfo::ColumnInfo(const char *nm, U32 xs)
{
   name = nm;
   xStart = xs;
}

// Destructor
QueryServersUserInterface::ColumnInfo::~ColumnInfo()
{
   // Do nothing
}


// Constructor
QueryServersUserInterface::QueryServersUserInterface(ClientGame *game) : UserInterface(game), ChatParent(game)
{
   mLastUsedServerId = 0;
   mSortColumn = 0;
   mLastSortColumn = 0;
   mHighlightColumn = 0;
   mSortAscending = true;
   mReceivedListOfServersFromMaster = false;
   mouseScrollTimer.setPeriod(10 * MOUSE_SCROLL_INTERVAL);

   mServersPerPage = 8;    // To start with, anyway...

   // Column name, x-start pos
   columns.push_back(ColumnInfo("SERVER NAME", 3));
   columns.push_back(ColumnInfo("STAT", 400));
   columns.push_back(ColumnInfo("PING", 449));
   columns.push_back(ColumnInfo("PLAYERS BOTS", 493));
   columns.push_back(ColumnInfo("ADDRESS", 616));

#ifdef TNL_ENABLE_ASSERTS
   // Make sure columns are wide enough for their labels
   static const S32 MIN_PAD = 3;  // appears to only be used for TNLAssert
   TNLAssert(columns[1].xStart - columns[0].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[0].name), "Col[0] too narrow!");
   TNLAssert(columns[2].xStart - columns[1].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[1].name), "Col[1] too narrow!");
   TNLAssert(columns[3].xStart - columns[2].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[2].name), "Col[2] too narrow!");
   TNLAssert(columns[4].xStart - columns[3].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[3].name), "Col[3] too narrow!");
   TNLAssert(gScreenInfo.getGameCanvasWidth() - columns[4].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[4].name), "Col[4] too narrow!");
#endif

   selectedId = 0xFFFFFF;

   sort();
   mShouldSort = false;
   mShowChat = true;

   mMessageDisplayCount = 16;

   // Create our buttons
   S32 textsize = 12;
   S32 ypos = BANNER_HEIGHT - 30;

   Button prevButton = Button(getGame(), horizMargin, ypos, textsize, 4, "PREV", Colors::white, Colors::yellow, prevButtonClickedCallback);
   Button nextButton = Button(getGame(), gScreenInfo.getGameCanvasWidth() - horizMargin - 50, ypos, 
                              textsize, 4, "NEXT", Colors::white, Colors::yellow, nextButtonClickedCallback);
   
   buttons.push_back(prevButton);
   buttons.push_back(nextButton);
}

// Destructor
QueryServersUserInterface::~QueryServersUserInterface()
{
   // Do nothing
}

// Initialize: Runs when "connect to server" screen is shown
void QueryServersUserInterface::onActivate()
{
   servers.clear();     // Start fresh
   mReceivedListOfServersFromMaster = false;
   mItemSelectedWithMouse = false;
   mScrollingUpMode = false;
   mJustMovedMouse = false;
   mDraggingDivider = false;
   mAnnounced = false;

   mGivenUpOnMaster = false;

   mPage = 0;     // Start off showing the first page, as expected

   
#if 0
   // Populate server list with dummy data to see how it looks
   for(U32 i = 0; i < 512; i++)
   {
      char name[128];
      dSprintf(name, MaxServerNameLen, "Dummy Svr%8x", Random::readI());

      ServerRef s;
      s.serverName = name;
      s.id = i;
      s.pingTime = Random::readF() * 512;
      s.serverAddress.port = GameSettings::DEFAULT_GAME_PORT;
      s.serverAddress.netNum[0] = Random::readI();
      s.maxPlayers = Random::readF() * 16 + 8;
      s.playerCount = Random::readF() * s.maxPlayers;
      s.pingTimedOut = false;
      s.everGotQueryResponse = false;
      s.serverDescr = "Here is  description.  There are many like it, but this one is mine.";
      s.msgColor = Colors::yellow;
      servers.push_back(s);
   }
#endif
   
   mSortColumn = 0;
   mHighlightColumn = 0;
   pendingPings = 0;
   pendingQueries = 0;
   mSortAscending = true;
   mNonce.getRandom();

   mPlayersInGlobalChat.clear();

   contactEveryone();
}


// Checks for connection to master, and sets up timer to keep running this until it finds one.  Once a connection is located,
// it fires off a series of requests to the master asking for servers and chat names.
void QueryServersUserInterface::contactEveryone()
{
   mBroadcastPingSendTime = Platform::getRealMilliseconds();

   //Address broadcastAddress(IPProtocol, Address::Broadcast, DEFAULT_GAME_PORT);
   //getGame()->getNetInterface()->sendPing(broadcastAddress, mNonce);

   // Always ping these servers -- typically a local server
   Vector<string> *pingList = &getGame()->getSettings()->getIniSettings()->alwaysPingList;

   for(S32 i = 0; i < pingList->size(); i++)
   {
      Address address(pingList->get(i).c_str());
      getGame()->getNetInterface()->sendPing(address, mNonce);
   } 

   // Try to ping the servers from our fallback list if we're having trouble connecting to the master
   if(getGame()->getTimeUnconnectedToMaster() > GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME) 
   {
      Vector<string> *serverList = &getGame()->getSettings()->getIniSettings()->prevServerListFromMaster;

      for(S32 i = 0; i < serverList->size(); i++)
         getGame()->getNetInterface()->sendPing(Address(serverList->get(i).c_str()), mNonce);

      mGivenUpOnMaster = true;
   }

   // If we already have a connection to the Master, start the server query... otherwise, don't
   MasterServerConnection *masterConn = getGame()->getConnectionToMaster();

   if(masterConn && masterConn->isEstablished())
   {
      if(!mAnnounced)
      {
         masterConn->c2mJoinGlobalChat();    // Announce our presence in the chat room
         mAnnounced = true;
      }
      masterConn->startServerQuery();
      mWaitingForResponseFromMaster = true;
   }
   else     // Don't have a valid connection object
   {
      mMasterRequeryTimer.reset(CheckMasterServerReady);    // Check back in a second to see if we've established a connection to the master
      mWaitingForResponseFromMaster = false;
   }
}


// Master server has returned a list of servers that match our original criteria (including being of the
// correct version).  Send a query packet to each.
void QueryServersUserInterface::addPingServers(const Vector<IPAddress> &ipList)
{
   // First check our list for dead servers -- if it's on our local list, but not on the master server's list, it's dead
   for(S32 i = servers.size()-1; i >= 0 ; i--)
   {
      if(!servers[i].isFromMaster)     // Skip servers that we didn't learn about from the master
         continue;

      bool found = false;
      for(S32 j = 0; j < ipList.size(); j++)
      {
         if(servers[i].serverAddress == Address(ipList[j]))
         {
            found = true;
            break;
         }
      }

      if(!found)              // It's a defunct server...
         servers.erase_fast(i);    // ...bye-bye!
   }

   Vector<string> *serverList = &getGame()->getSettings()->getIniSettings()->prevServerListFromMaster;

   // Save servers from the master
   if(ipList.size() != 0) 
      serverList->clear();    // Don't clear if we have nothing to add... 

   // Now add any new servers
   for(S32 i = 0; i < ipList.size(); i++)
   {
      serverList->push_back(Address(ipList[i]).toString());

      bool found = false;
      // Is this server already in our list?
      for(S32 j = 0; j < servers.size(); j++)
         if(servers[j].serverAddress == Address(ipList[i]))
         {
            found = true;
            break;
         }

      if(!found)  // It's a new server!
      {
         ServerRef s;
         s.state = ServerRef::Start;
         s.id = ++mLastUsedServerId;
         s.sendNonce.getRandom();
         s.isFromMaster = true;
         s.serverAddress.set(ipList[i]);

         s.serverName = "Internet Server";
         s.serverDescr = "Internet Server -- attempting to connect";
         s.msgColor = Colors::white;   // white messages
         servers.push_back(s);
         mShouldSort = true;
      }
   }

   mMasterRequeryTimer.reset(MasterRequeryTime);
   mWaitingForResponseFromMaster = false;
}


void QueryServersUserInterface::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   mReceivedListOfServersFromMaster = true;
   addPingServers(serverList);
}



void QueryServersUserInterface::gotPingResponse(const Address &theAddress, const Nonce &theNonce, U32 clientIdentityToken)
{
   // See if this ping is a server from the local broadcast ping:
   if(mNonce == theNonce)
   {
      for(S32 i = 0; i < servers.size(); i++)
         if(servers[i].serverAddress == theAddress)      // servers[i].sendNonce == theNonce &&
            return;

      // Yes, it was from a local ping
      ServerRef s;
      s.pingTime = Platform::getRealMilliseconds() - mBroadcastPingSendTime;
      s.state = ServerRef::ReceivedPing;
      s.id = ++mLastUsedServerId;
      s.sendNonce = theNonce;
      s.identityToken = clientIdentityToken;
      s.serverAddress = theAddress;
      s.isFromMaster = false;
      s.serverName = "LAN Server";
      s.serverDescr = "LAN Server -- attempting to connect";
      s.msgColor = Colors::white;   // white messages
      servers.push_back(s);
      return;
   }

   // Otherwise, not from local broadcast ping.  Check if this ping is in the list:
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];
      if(s.sendNonce == theNonce && s.serverAddress == theAddress && s.state == ServerRef::SentPing)
      {
         s.pingTime = Platform::getRealMilliseconds() - s.lastSendTime;
         s.identityToken = clientIdentityToken;
         if(s.state == ServerRef::SentPing)
         {
            s.state = ServerRef::ReceivedPing;
            pendingPings--;
         }
         break;
      }
   }
   mShouldSort = true;
}


void QueryServersUserInterface::gotQueryResponse(const Address &theAddress, const Nonce &clientNonce, const char *serverName, const char *serverDescr, U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];
      if(s.sendNonce == clientNonce && s.serverAddress == theAddress && s.state == ServerRef::SentQuery)
      {
         s.playerCount = playerCount;
         s.maxPlayers = maxPlayers;
         s.botCount = botCount;
         s.dedicated = dedicated;
         s.test = test;
         s.passwordRequired = passwordRequired;
         if(!s.isFromMaster)
            s.pingTimedOut = false;       // Cures problem with local servers incorrectly displaying ?s for first 15 seconds
         s.sendCount = 0;  // Fix random "Query/ping timed out"
         s.everGotQueryResponse = true;

         s.serverName = string(serverName).substr(0, MaxServerNameLen);
         s.serverDescr = string(serverDescr).substr(0, MaxServerDescrLen);
         s.msgColor = Colors::yellow;   // yellow server details
         s.pingTime = Platform::getRealMilliseconds() - s.lastSendTime;
         s.lastSendTime = Platform::getRealMilliseconds();     // Record time our last query was received, so we'll know when to send again
         if(s.state == ServerRef::SentQuery)
         {
            s.state = ServerRef::ReceivedQuery;
            pendingQueries--;
         }
      }
   }
   mShouldSort = true;
}


void QueryServersUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   U32 elapsedTime = Platform::getRealMilliseconds() - time;
   time = Platform::getRealMilliseconds();
   mouseScrollTimer.update(timeDelta);

   // Timeout old pings and queries
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];

      if(s.state == ServerRef::SentPing && (time - s.lastSendTime) > PingQueryTimeout)
      {
         s.state = ServerRef::Start;
         pendingPings--;
      }
      else if(s.state == ServerRef::SentQuery && (time - s.lastSendTime) > PingQueryTimeout)
      {
         s.state = ServerRef::ReceivedPing;
         pendingQueries--;
      }
   }

   // Send new pings
   for(S32 i = 0; i < servers.size() ; i++)
   {
      if(pendingPings < MaxPendingPings)   // IF goes inside FOR, so it won't send 100 pings at the same time.
      {
         ServerRef &s = servers[i];
         if(s.state == ServerRef::Start)     // This server is at the beginning of the process
         {
            s.pingTimedOut = false;
            s.sendCount++;
            if(s.sendCount > PingQueryRetryCount)     // Ping has timed out, sadly
            {
               s.pingTime = 999;
               s.serverName = "Ping Timed Out";
               s.serverDescr = "No information: Server not responding to pings";
               s.msgColor = Colors::red;   // red for errors
               s.playerCount = 0;
               s.maxPlayers = 0;
               s.botCount = 0;
               s.state = ServerRef::ReceivedQuery;    // In effect, this will tell app not to send any more pings or queries to this server
               mShouldSort = true;
               s.pingTimedOut = true;
            }
            else
            {
               s.state = ServerRef::SentPing;
               s.lastSendTime = time;
               s.sendNonce.getRandom();
               getGame()->getNetInterface()->sendPing(s.serverAddress, s.sendNonce);
               pendingPings++;
               if(pendingPings >= MaxPendingPings)
                  break;
            }
         }
      }
   }

   // When all pings have been answered or have timed out, send out server status queries ... too slow
   // Want to start query immediately, to display server name / current players faster
   for(S32 i = servers.size()-1; i >= 0 ; i--)
   {
      if(pendingQueries < MaxPendingQueries)
      {
         ServerRef &s = servers[i];
         if(s.state == ServerRef::ReceivedPing)
         {
            s.sendCount++;
            if(s.sendCount > PingQueryRetryCount)
            {
               // If this is a local server, remove it from the list if the query times out...
               // We don't have another mechanism for culling dead local servers
               if(!s.isFromMaster)
               {
                  servers.erase_fast(i);
                  continue;
               }
               // Otherwise, we can deal with timeouts on remote servers
               s.serverName = "Query Timed Out";
               s.serverDescr = "No information: Server not responding to status query";
               s.msgColor = Colors::red;   // red for errors
               s.playerCount = s.maxPlayers = s.botCount = 0;
               s.state = ServerRef::Start;//ReceivedQuery;
               mShouldSort = true;

            }
            else
            {
               s.state = ServerRef::SentQuery;
               s.lastSendTime = time;
               getGame()->getNetInterface()->sendQuery(s.serverAddress, s.sendNonce, s.identityToken);
               pendingQueries++;
               if(pendingQueries >= MaxPendingQueries)
                  break;
            }
         }
      }
   }

   // Every so often, send out a new batch of queries
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];
      if(s.state == ServerRef::ReceivedQuery && time - s.lastSendTime > RequeryTime)
      {
         if(s.pingTimedOut)
         {
            s.state = ServerRef::Start;            // Will trigger a new round of pinging
         }
         else
            s.state = ServerRef::ReceivedPing;     // Will trigger a new round of querying

         s.sendCount = 0;
      }
   }

   // Not sure about the logic in here... maybe this is right...
   if( (mMasterRequeryTimer.update(elapsedTime) && !mWaitingForResponseFromMaster) ||
            (!mGivenUpOnMaster && getGame()->getTimeUnconnectedToMaster() > GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME) )
       contactEveryone();

   // Go to previous page if a server has gone away and the last server has disappeared from the current screen
   while(getFirstServerIndexOnCurrentPage() >= servers.size() && mPage > 0)
       mPage--;

}  // end idle


bool QueryServersUserInterface::mouseInHeaderRow(const Point *pos)
{
   return pos->y >= COLUMN_HEADER_TOP && pos->y < COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT - 1;
}


string QueryServersUserInterface::getLastSelectedServerName()
{
   return mLastSelectedServer.serverName;
}


S32 QueryServersUserInterface::getSelectedIndex()
{
   if(servers.size() == 0)       // When no servers, return dummy value
      return -1;

   // This crazy thing can happen if the number of servers changes and suddenly there's none shown on the screen
   // Shouldn't happen anymore due to check in idle() function
   if(getFirstServerIndexOnCurrentPage() >= servers.size())
      return -1;

   if(mItemSelectedWithMouse)    // When using mouse, always follow mouse cursor
   {
      S32 indx = S32(floor((gScreenInfo.getMousePos()->y - TOP_OF_SERVER_LIST + 2) / SERVER_ENTRY_HEIGHT) + 
                     getFirstServerIndexOnCurrentPage() ); // fixes mouse offset problem by removing this part: - (mScrollingUpMode || mMouseAtBottomFixFactor ? 1 : 0) 

      // Bounds checking and such
      if(indx < 0)
         indx = 0;
      else if(indx >= servers.size())
         indx = servers.size() - 1;

      // Even after that check, we can still have an indx that extends below the bottom of our screen
      if(indx < getFirstServerIndexOnCurrentPage())
         indx = getFirstServerIndexOnCurrentPage();
      else if(indx > getServersPerPage() + getFirstServerIndexOnCurrentPage() - 1)
         indx = getServersPerPage() + getFirstServerIndexOnCurrentPage() - 1;
      
      return indx;
   }
   else
   {
      for(S32 i = 0; i < servers.size(); i++)
         if(servers[i].id == selectedId)
            return i;
      return -1;                 // Can't find selected server; return dummy
   }
}


static void renderDedicatedIcon()
{
   // Add a "D"
   drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "D");
}


static void renderTestIcon()
{
   // Add a "T"
   drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "T");
}


static void renderLockIcon()
{
   F32 vertices[] = {
         0,2,
         0,4,
         3,4,
         3,2
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);

   F32 vertices2[] = {
         2.6f, 2,
         2.6f, 1.3f,
         2.4f, 0.9f,
         1.9f, 0.6f,
         1.1f, 0.6f,
         0.6f, 0.9f,
         0.4f, 1.3f,
         0.4f, 2
   };
   renderVertexArray(vertices2, ARRAYSIZE(vertices2) / 2, GL_LINE_STRIP);
}


extern void glScale(F32 scaleFactor);

void QueryServersUserInterface::render()
{
   const S32 canvasWidth =  gScreenInfo.getGameCanvasWidth();
//   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   bool drawmsg1 = false;
   bool drawmsg2 = false;

   if(mShouldSort)
   {
      mShouldSort = false;
      sort();
   }

   renderTopBanner();

   // Render buttons
   const Point *mousePos = gScreenInfo.getMousePos();

   for(S32 i = 0; i < buttons.size(); i++)
      buttons[i].render(mJustMovedMouse ? mousePos->x : -1, mJustMovedMouse ? mousePos->y : -1);

   MasterServerConnection *masterConn = getGame()->getConnectionToMaster();
   bool connectedToMaster = masterConn && masterConn->isEstablished();

   if(connectedToMaster)
   {
      glColor(Colors::MasterServerBlue);
      drawCenteredStringf(vertMargin - 8, 12, "Connected to %s", masterConn->getMasterName().c_str() );
   }
   else
   {
      glColor(Colors::red);
      if(mGivenUpOnMaster && getGame()->getSettings()->getIniSettings()->prevServerListFromMaster.size() != 0)
         drawCenteredString(vertMargin - 8, 12, "Couldn't connect to Master Server - Using server list from last successful connect.");
      else
         drawCenteredString(vertMargin - 8, 12, "Couldn't connect to Master Server - Firewall issues? Do you have the latest version?");
   }

   // Show some chat messages
   if(mShowChat)
   {
       S32 dividerPos = getDividerPos();

      // Horizontal divider between game list and chat window
      glColor(Colors::white);
      F32 vertices[] = {
            (F32)horizMargin,               (F32)dividerPos,
            (F32)canvasWidth - horizMargin, (F32)dividerPos
      };
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);


      S32 ypos = dividerPos + 3;      // 3 = gap after divider

      renderMessages(ypos, mMessageDisplayCount);

      renderMessageComposition(BOTTOM_OF_CHAT_WINDOW - CHAT_FONT_SIZE - 2 * CHAT_FONT_MARGIN);
      renderChatters(horizMargin, BOTTOM_OF_CHAT_WINDOW);
   }

   // Instructions at bottom of server selection section
   glColor(Colors::white);
   drawCenteredString(getDividerPos() - SEL_SERVER_INSTR_SIZE - SEL_SERVER_INSTR_GAP_ABOVE_DIVIDER_LINE + 1, SEL_SERVER_INSTR_SIZE, 
                      "UP, DOWN to select, ENTER to join | Click on column to sort | ESC exits");

   if(servers.size())      // There are servers to display...
   {
      // Find the selected server (it may have moved due to sort or new/removed servers)
      S32 selectedIndex = getSelectedIndex();
      if(selectedIndex < 0 && servers.size() >= 0)
      {
         selectedId = servers[0].id;
         selectedIndex = 0;
      }

      S32 colwidth = columns[1].xStart - columns[0].xStart;    

      U32 y = TOP_OF_SERVER_LIST + (selectedIndex - getFirstServerIndexOnCurrentPage()) * SERVER_ENTRY_HEIGHT;

      // Render box behind selected item -- do this first so that it will not obscure descenders on letters like g in the column above
      bool disabled = composingMessage() && !mJustMovedMouse;
      drawMenuItemHighlight(0, y, canvasWidth, y + SERVER_ENTRY_TEXTSIZE + 4, disabled);


      S32 lastServer = min(servers.size() - 1, (mPage + 1) * getServersPerPage() - 1);

      for(S32 i = getFirstServerIndexOnCurrentPage(); i <= lastServer; i++)
      {
         y = TOP_OF_SERVER_LIST + (i - getFirstServerIndexOnCurrentPage()) * SERVER_ENTRY_HEIGHT + 2;
         ServerRef &s = servers[i];

         if(i == selectedIndex)
         {
            // Render server description at bottom
            glColor(s.msgColor);
            U32 serverDescrLoc = TOP_OF_SERVER_LIST + getServersPerPage() * SERVER_ENTRY_HEIGHT + 2;
            drawString(horizMargin, serverDescrLoc, SERVER_DESCR_TEXTSIZE, s.serverDescr.c_str());    
         }

         // Truncate server name to fit in the first column...
         string sname = "";

         // ...but first, see if the name will fit without truncation... if so, don't bother
         if(getStringWidth(SERVER_ENTRY_TEXTSIZE, s.serverName.c_str()) < colwidth)
            sname = s.serverName;
         else
            for(std::size_t j = 0; j < s.serverName.length(); j++)
               if(getStringWidth(SERVER_ENTRY_TEXTSIZE, (sname + s.serverName.substr(j, 1)).c_str() ) < colwidth)
                  sname += s.serverName[j];
               else
                  break;

         glColor(Colors::white);
         drawString(columns[0].xStart, y, SERVER_ENTRY_TEXTSIZE, sname.c_str());

         // Render icons
         glColor(Colors::green);
         if(s.dedicated || s.test || s.pingTimedOut || !s.everGotQueryResponse)
         {
            glPushMatrix();
               glTranslate(columns[1].xStart + 5, y + 2, 0);
               if( s.pingTimedOut || !s.everGotQueryResponse )
                  drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "?");
               else if(s.test)
                  renderTestIcon();
               else
                  renderDedicatedIcon();
            glPopMatrix();
         }
         if(s.passwordRequired || s.pingTimedOut || !s.everGotQueryResponse)
         {
            glPushMatrix();
               glTranslatef(F32(columns[1].xStart + 25), F32(y + 2), 0);
               if(s.pingTimedOut || !s.everGotQueryResponse)
                  drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "?");
               else
               {
                  glScale(3.65f);
                  renderLockIcon();
               }
            glPopMatrix();
         }

         // Set color based on ping time
         if(s.pingTime < 100)
            glColor(Colors::green);
         else if(s.pingTime < 250)
            glColor(Colors::yellow);
         else
            glColor(Colors::red);

         drawStringf(columns[2].xStart, y, SERVER_ENTRY_TEXTSIZE, "%d", s.pingTime);

         // Color by number of players
         Color color;
         if(s.playerCount == s.maxPlayers)
            color = Colors::red;       // max players
         else if(s.playerCount == 0)
            color = Colors::yellow;    // no players
         else
            color = Colors::green;     // 1 or more players

         glColor(color * 0.5);         // dim color
         drawStringf(columns[3].xStart + 30, y, SERVER_ENTRY_TEXTSIZE, "/%d", s.maxPlayers);

         glColor(color);
         drawStringf(columns[3].xStart, y, SERVER_ENTRY_TEXTSIZE, "%d", s.playerCount);
         drawStringf(columns[3].xStart + 78, y, SERVER_ENTRY_TEXTSIZE, "%d", s.botCount);

         glColor(Colors::white);
         drawString(columns[4].xStart, y, SERVER_ENTRY_TEXTSIZE, s.serverAddress.toString());
      }
   }
   // Show some special messages if there are no servers, or we're not connected to the master
   else if(connectedToMaster && mReceivedListOfServersFromMaster)        // We have our response, and there were no servers
      drawmsg1 = true;
   else if(!connectedToMaster)         // Still waiting to connect to the master...
      drawmsg2 = true;

   renderColumnHeaders();

   if(drawmsg1 || drawmsg2)
      renderMessageBox(drawmsg1, drawmsg2);
}


void QueryServersUserInterface::renderTopBanner()
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Top banner
   glColor(Colors::richGreen);
   F32 vertices[] = {
         0,                0,
         (F32)canvasWidth, 0,
         (F32)canvasWidth, (F32)BANNER_HEIGHT,
         0,                (F32)BANNER_HEIGHT
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

   glColor(Colors::white);
   drawCenteredString(vertMargin + 7, 35, "BITFIGHTER GAME LOBBY");

   const S32 FONT_SIZE = 12;
   drawStringf(horizMargin, vertMargin, FONT_SIZE, "SERVERS: %d", servers.size());
   drawStringfr(canvasWidth - horizMargin, vertMargin, FONT_SIZE, "PAGE %d/%d", mPage + 1, getLastPage() + 1);
}


void QueryServersUserInterface::renderColumnHeaders()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Draw vertical dividing lines
   glColor(0.7f);

   for(S32 i = 1; i < columns.size(); i++)
      drawVertLine(columns[i].xStart - 4, COLUMN_HEADER_TOP, TOP_OF_SERVER_LIST + getServersPerPage() * SERVER_ENTRY_HEIGHT + 2);

   // Horizontal lines under column headers
   drawHorizLine(0, canvasWidth, COLUMN_HEADER_TOP);
   drawHorizLine(0, canvasWidth, COLUMN_HEADER_TOP + COLUMN_HEADER_TEXTSIZE + 7);

   // Column headers (will partially overwrite horizontal lines) 
   S32 x1 = max(columns[mSortColumn].xStart - 3, 1);    // Going to 0 makes line look too thin...
   S32 x2;
   if(mSortColumn == columns.size() - 1)
      x2 = canvasWidth - 1;
   else
      x2 = columns[mSortColumn+1].xStart - 5;

   drawFilledRect(x1, COLUMN_HEADER_TOP, x2, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1, Color(.4, .4, 0), Colors::white);

   // And now the column header text itself
   for(S32 i = 0; i < columns.size(); i++) 
      drawString(columns[i].xStart, COLUMN_HEADER_TOP + 3, COLUMN_HEADER_TEXTSIZE, columns[i].name);

   // Highlight selected column
   if(mHighlightColumn != mSortColumn) 
   {
      // And render a box around the column under the mouse, if different
      x1 = max(columns[mHighlightColumn].xStart - 3, 1);
      if(mHighlightColumn == columns.size() - 1)
         x2 = canvasWidth - 1;
      else
         x2 = columns[mHighlightColumn+1].xStart - 5;

      glColor(Colors::white);
      drawRect(x1, COLUMN_HEADER_TOP, x2, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1, GL_LINE_LOOP);
   }
}


void QueryServersUserInterface::renderMessageBox(bool drawmsg1, bool drawmsg2)
{
   // Warning... the following section is pretty darned ugly!  We're just drawing a message box...

   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   const S32 fontsize = 20;
   const S32 fontgap = 5;

   const char *msg1, *msg2;
   S32 lines;
   if(drawmsg1)
   {
      msg1 = "There are currently no games online.";
      msg2 = "Why don't you host one?";
      lines = 2;
   }
   else if(mGivenUpOnMaster)
   {
      msg1 = "Unable to connect to master";
      msg2 = "";
      lines = 1;
   }
   else
   {
      msg1 = "Contacting master server...";
      msg2 = "";
      lines = 1;
   }

   const S32 strwid = getStringWidth(fontsize, msg1);
   const S32 msgboxMargin = 20;

   S32 ypos = mShowChat ? TOP_OF_SERVER_LIST + 25 + (getDividerPos() - TOP_OF_SERVER_LIST) * 2 / 5 : canvasHeight / 2 ;
   ypos += (lines - 2) * (fontsize + fontgap) / 2;

   const S32 ypos1 = ypos - lines * (fontsize + fontgap) - msgboxMargin;
   const S32 ypos2 = ypos + msgboxMargin;

   const S32 xpos1 = (canvasWidth - strwid) / 2 - msgboxMargin; 
   const S32 xpos2 = (canvasWidth + strwid) / 2 + msgboxMargin;

   drawFilledRect(xpos1, ypos1, xpos2, ypos2, Color(.4, 0, 0), Colors::red);

   // Draw text
   glColor(Colors::white);

   drawCenteredString(ypos - lines * (fontsize + fontgap), fontsize, msg1);
   drawCenteredString(ypos - (fontsize + fontgap), fontsize, msg2);
}


void QueryServersUserInterface::recalcCurrentIndex()
{
   S32 indx = mPage * getServersPerPage() + selectedId % getServersPerPage() - 1;
   if(indx >= servers.size())
      indx = servers.size() - 1;

   selectedId = servers[indx].id;
}


void QueryServersUserInterface::onTextInput(char ascii)
{
   if(ascii)
      mLineEditor.addChar(ascii);
}


// All key handling now under one roof!
bool QueryServersUserInterface::onKeyDown(InputCode inputCode)
{
   inputCode = InputCodeManager::convertJoystickToKeyboard(inputCode);

   mJustMovedMouse = (inputCode == MOUSE_LEFT || inputCode == MOUSE_MIDDLE || inputCode == MOUSE_RIGHT);
   mDraggingDivider = false;

   if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_OUTGAMECHAT, inputCode))  
   {
      // Toggle half-height servers, full-height servers, full chat overlay

      mShowChat = !mShowChat;
      if(mShowChat) 
      {
         ChatUserInterface *ui = getUIManager()->getUI<ChatUserInterface>();
         ui->setRenderUnderlyingUI(false);    // Don't want this screen to bleed through...

         getUIManager()->activate(ui);
      }

      return true;
   }

   if(Parent::onKeyDown(inputCode))
      return true;

   S32 currentIndex = -1;

   if(inputCode == KEY_ENTER || inputCode == BUTTON_START || inputCode == MOUSE_LEFT)     // Return - select highlighted server & join game
   {
      const Point *mousePos = gScreenInfo.getMousePos();

      if(inputCode == MOUSE_LEFT && mouseInHeaderRow(mousePos))
      {
         sortSelected();
      }
      else if(inputCode == MOUSE_LEFT && mousePos->y < COLUMN_HEADER_TOP)
      {
         // Check buttons -- they're all up here for the moment
         for(S32 i = 0; i < buttons.size(); i++)
            buttons[i].onClick(mousePos->x, mousePos->y);
      }
      else if(inputCode == MOUSE_LEFT && isMouseOverDivider())
      {
         mDraggingDivider = true;
      }
      else if(inputCode == MOUSE_LEFT && mousePos->y > COLUMN_HEADER_TOP + SERVER_ENTRY_HEIGHT * min(servers.size() + 1, getServersPerPage() + 2))
      {
         // Clicked too low... also do nothing
      }
      else
      {
         // If the user is composing a message and hits enter, submit the message
         if(inputCode == KEY_ENTER && composingMessage())
            issueChat();
         else
         {
            S32 currentIndex = getSelectedIndex();
            if(currentIndex == -1)
               return true;

            if(servers.size() > currentIndex)      // Index is valid
            {
               leaveGlobalChat();

               // Join the selected game...   (what if we select a local server from the list...  wouldn't 2nd param be true?)
               // Second param, false when we can ping that server, allows faster connect. If we can ping, we can connect without master help.
               getGame()->joinRemoteGame(servers[currentIndex].serverAddress, servers[currentIndex].isFromMaster && 
                    (getGame()->getSettings()->getIniSettings()->neverConnectDirect || !servers[currentIndex].everGotQueryResponse));
               mLastSelectedServer = servers[currentIndex];    // Save this because we'll need the server name when connecting.  Kind of a hack.

               // ...and clear out the server list so we don't do any more pinging
               servers.clear();
            }
         }
      }
   }
   else if(inputCode == KEY_ESCAPE)  // Return to main menu
   {
      playBoop();
      leaveGlobalChat();
      getUIManager()->activate<MainMenuUserInterface>();
   }
   else if(inputCode == KEY_LEFT)
   {
      mHighlightColumn--;
      if(mHighlightColumn < 0)
         mHighlightColumn = 0;

      if(inputCode == BUTTON_DPAD_LEFT)
         sortSelected();
   }
   else if(inputCode == KEY_RIGHT)
   {
      mHighlightColumn++;
      if(mHighlightColumn >= columns.size())
         mHighlightColumn = columns.size() - 1;

      if(inputCode == BUTTON_DPAD_RIGHT)
         sortSelected();
   }
   else if(inputCode == KEY_PAGEUP)
   {
      backPage();

      Cursor::disableCursor();        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
   }
   else if(inputCode == KEY_PAGEDOWN) 
   {
      advancePage();

      Cursor::disableCursor();        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
   }

   // The following keys only make sense if there are some servers to browse through
   else if(servers.size() != 0)
   {
      if(inputCode == KEY_UP)
      {
         currentIndex = getSelectedIndex() - 1;
         if(currentIndex < 0)
            currentIndex = servers.size() - 1;
         mPage = currentIndex / getServersPerPage();

         Cursor::disableCursor();        // Hide cursor when navigating with keyboard or joystick
         mItemSelectedWithMouse = false;
         selectedId = servers[currentIndex].id;
      }
      else if(inputCode == KEY_DOWN)
      {
         currentIndex = getSelectedIndex() + 1;
         if(currentIndex >= servers.size())
            currentIndex = 0;

         mPage = currentIndex / getServersPerPage();

         Cursor::disableCursor();        // Hide cursor when navigating with keyboard or joystick
         mItemSelectedWithMouse = false;
         selectedId = servers[currentIndex].id;
      }
      else
         return mLineEditor.handleKey(inputCode);
   }
   // If no key is handled
   else
      return mLineEditor.handleKey(inputCode);

   // A key was handled
   return true;
}


void QueryServersUserInterface::backPage()
{
   mPage--;

   if(mPage < 0)
      mPage = getLastPage();      // Last page

   recalcCurrentIndex();
}


void QueryServersUserInterface::advancePage()
{
   mPage++;
   if(mPage > getLastPage())  
      mPage = 0;                 // First page

   recalcCurrentIndex();
}


void QueryServersUserInterface::onKeyUp(InputCode inputCode)
{
   if(mDraggingDivider)
   {
      SDL_SetCursor(Cursor::getDefault());
      mDraggingDivider = false;
   }
}


void QueryServersUserInterface::onMouseDragged() 
{
   const Point *mousePos = gScreenInfo.getMousePos();    // (used in some of the macro expansions)

   if(mDraggingDivider)
   {
      S32 MIN_SERVERS_PER_PAGE = 4;
      S32 MAX_SERVERS_PER_PAGE = 18;

      mServersPerPage = ((S32)mousePos->y - TOP_OF_SERVER_LIST - AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER) / SERVER_ENTRY_HEIGHT;

      // Bounds checking
      if(mServersPerPage < MIN_SERVERS_PER_PAGE)
         mServersPerPage = MIN_SERVERS_PER_PAGE;
      else if(mServersPerPage > MAX_SERVERS_PER_PAGE)
         mServersPerPage = MAX_SERVERS_PER_PAGE;

      mMessageDisplayCount = (BOTTOM_OF_CHAT_WINDOW - getDividerPos() - 4) / (CHAT_FONT_SIZE + CHAT_FONT_MARGIN) - 1;         
   }
}


S32 QueryServersUserInterface::getFirstServerIndexOnCurrentPage()
{
   return mPage * mServersPerPage;
}


// User is sorting by selected column
void QueryServersUserInterface::sortSelected()
{
   // No servers, no sorting!  So no crashing!
   if(servers.size() == 0)
      return;

   S32 currentItem = getSelectedIndex();     // Preserve the position of selected item (so highlight doesn't move during sort)

   mSortColumn = mHighlightColumn;

   if(mLastSortColumn == mSortColumn)
      mSortAscending = !mSortAscending;     // Toggle ascending/descending
   else
   {
      mLastSortColumn = mSortColumn;
      mSortAscending = true;
   }
   sort();   

   selectedId = servers[currentItem].id;
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void QueryServersUserInterface::onMouseMoved()
{
   Parent::onMouseMoved();

   if(InputCodeManager::getState(MOUSE_LEFT))
   {
      onMouseDragged();
      return;
   }

   const Point *mousePos = gScreenInfo.getMousePos();

   Cursor::enableCursor();

   if(mouseInHeaderRow(mousePos))
   {
      mHighlightColumn = 0;
      for(S32 i = columns.size()-1; i >= 0; i--)
         if(mousePos->x > columns[i].xStart)
         {
            mHighlightColumn = i;
            break;
         }
   }
   else
      mHighlightColumn = mSortColumn;

   if(isMouseOverDivider())
      SDL_SetCursor(Cursor::getVerticalResize());
   else
      SDL_SetCursor(Cursor::getDefault());


   mItemSelectedWithMouse = true;
   mJustMovedMouse = true;
}


S32 QueryServersUserInterface::getDividerPos()
{
   if(mShowChat)
      return TOP_OF_SERVER_LIST + getServersPerPage() * SERVER_ENTRY_HEIGHT + AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER;
   else
      return gScreenInfo.getGameCanvasHeight();
}


S32 QueryServersUserInterface::getServersPerPage()
{
   if(mShowChat)
      return mServersPerPage;
   else
      return (gScreenInfo.getGameCanvasHeight() - TOP_OF_SERVER_LIST - AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER) / SERVER_ENTRY_HEIGHT;
}


S32 QueryServersUserInterface::getLastPage()
{
   return (servers.size() - 1) / mServersPerPage;
}


bool QueryServersUserInterface::isMouseOverDivider()
{
   if(!mShowChat)       // Divider is only in operation when window is split
      return false;

   F32 mouseY = gScreenInfo.getMousePos()->y;

   S32 hitMargin = 4;
   S32 dividerPos = getDividerPos();

   return (mouseY >= dividerPos - hitMargin) && (mouseY <= dividerPos + hitMargin);
}


// Sort server list by various columns
static S32 QSORT_CALLBACK compareFuncName(const void *a, const void *b)
{
   return stricmp(((QueryServersUserInterface::ServerRef *) a)->serverName.c_str(),
                  ((QueryServersUserInterface::ServerRef *) b)->serverName.c_str());
}


static S32 QSORT_CALLBACK compareFuncPing(const void *a, const void *b)
{
   return S32(((QueryServersUserInterface::ServerRef *) a)->pingTime -
              ((QueryServersUserInterface::ServerRef *) b)->pingTime);
}


static S32 QSORT_CALLBACK compareFuncPlayers(const void *a, const void *b)
{
   S32 pc = S32(((QueryServersUserInterface::ServerRef *) a)->playerCount -
                ((QueryServersUserInterface::ServerRef *) b)->playerCount);
   if(pc)
      return pc;

   return S32(((QueryServersUserInterface::ServerRef *) a)->maxPlayers -
              ((QueryServersUserInterface::ServerRef *) b)->maxPlayers);
}


// First compare IPs, then, if equal, port numbers
static S32 QSORT_CALLBACK compareFuncAddress(const void *a, const void *b)
{
   U32 netNumA = ((QueryServersUserInterface::ServerRef *) a)->serverAddress.netNum[0];
   U32 netNumB = ((QueryServersUserInterface::ServerRef *) b)->serverAddress.netNum[0];

   if(netNumA == netNumB)
      return (S32)(((QueryServersUserInterface::ServerRef *) a)->serverAddress.port - 
                   ((QueryServersUserInterface::ServerRef *) b)->serverAddress.port);
   // else
   return S32(netNumA - netNumB);
}


void QueryServersUserInterface::sort()
{
   switch(mSortColumn)
   {
      case 0:
         qsort(servers.address(), servers.size(), sizeof(ServerRef), compareFuncName);
         break;
      case 2:
         qsort(servers.address(), servers.size(), sizeof(ServerRef), compareFuncPing);
         break;
      case 3:
         qsort(servers.address(), servers.size(), sizeof(ServerRef), compareFuncPlayers);
         break;
      case 4:
         qsort(servers.address(), servers.size(), sizeof(ServerRef), compareFuncAddress);
         break;
   }

   if(!mSortAscending)
   {
      S32 size = servers.size() / 2;
      S32 totalSize = servers.size();

      for(S32 i = 0; i < size; i++)
      {
         ServerRef temp = servers[i];
         servers[i] = servers[totalSize - i - 1];
         servers[totalSize - i - 1] = temp;
      }
   }
}


// Look for /commands in chat message before handing off to parent
void QueryServersUserInterface::issueChat()
{
   Vector<string> words;
   parseString(mLineEditor.getString(), words, ' ');

   if(words.size() == 0)  // might be caused by mLineEditor == " "
   {
      ;
   }
   else if(words[0] == "/connect")
   {
      Address address(&mLineEditor.c_str()[9]);
      if(address.isValid())
      {
         if(address.port == 0)
            address.port = GameSettings::DEFAULT_GAME_PORT;   // Use default port number if the user did not supply one

         getGame()->joinRemoteGame(address, false);
      }
      else
         newMessage("", "INVALID ADDRESS", false, true, true);
      return;
   }
   else if (words[0] == "/mute")
   {
      // No player name provided
      if(words.size() < 2)
         newMessage("", "USAGE: /mute <player name>", false, true, true);

      // Player is not found in the global chat list
      else if(!isPlayerInGlobalChat(words[1].c_str()))
         newMessage("", "PLAYER NOT FOUND", false, true, true);

      // If already muted, un-mute!
      else if(getGame()->isOnMuteList(words[1]))
      {
         getGame()->removeFromMuteList(words[1]);
         newMessage("", "PLAYER " + words[1] + " UN-MUTED" , false, true, true);
      }

      // Mute!!
      else
      {
         getGame()->addToMuteList(words[1]);
         newMessage("", "PLAYER " + words[1] + " MUTED" , false, true, true);
      }

      clearChat();
      return;
   }

   ChatParent::issueChat();
}


////////////////////////////////////////
////////////////////////////////////////

// Contstructor -- x,y are UL corner of button
Button::Button(ClientGame *game, S32 x, S32 y, S32 textSize, S32 padding, const char *label, Color fgColor, Color hlColor, void (*onClickCallback)(ClientGame *))
{
   mGame = game;
   mX = x;
   mY = y;
   mTextSize = textSize;
   mPadding = padding;
   mLabel = label;
   mFgColor = fgColor;
   mHlColor = hlColor;
   mBgColor = Colors::richGreen;
   mOnClickCallback = onClickCallback;
}

// Destructor
Button::~Button()
{
   // Do nothing
}


bool Button::isMouseOver(F32 mouseX, F32 mouseY)
{
   return(mouseX >= mX && mouseX <= mX + mPadding * 2 + getStringWidth(mTextSize, mLabel) &&
          mouseY >= mY && mouseY <= mY + mTextSize + mPadding * 2);
}


void Button::onClick(F32 mouseX, F32 mouseY)
{
   if(mOnClickCallback && isMouseOver(mouseX, mouseY) && isActive())
      mOnClickCallback(mGame);
}


bool Button::isActive() const
{
   return mGame->getUIManager()->getUI<QueryServersUserInterface>()->getLastPage() > 0;
}


void Button::render(F32 mouseX, F32 mouseY)
{
   if(!isActive())
      return;

   S32 labelLen = getStringWidth(mTextSize, mLabel);

   Color color;

   // Highlight a little when mouse is over buttons
   if(isMouseOver(mouseX, mouseY))
      color = mHlColor * 2;

   // Sit there and look unobtrusive
   else
      color = mFgColor;    

   drawFilledRect(mX, mY, mX + mPadding * 2 + labelLen, mY + mTextSize + mPadding * 2, mBgColor, color);
   drawString(mX + mPadding, mY + mPadding, mTextSize, mLabel);
}
 


};


