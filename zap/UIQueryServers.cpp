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

#include "tnlRandom.h"

#include "masterConnection.h"
#include "gameNetInterface.h"
#include "game.h"
#include "gameType.h"

#include "config.h"     // TODO: remove requirement -- currently for gIni stuff in screen pos calc

#include "UIChat.h"
#include "UIDiagnostics.h"

#include "keyCode.h"
#include "../glut/glutInclude.h"

namespace Zap
{


static const U32 GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME = 8000;      // 8 seconds should be enough time to connect to the master

// Text sizes and the like
static const U32 COLUMN_HEADER_TEXTSIZE = 14;     // Size of text in column headers
static const U32 SERVER_DESCR_TEXTSIZE = 18;    // Size of lower description of selected server
static const U32 SERVER_ENTRY_TEXTSIZE = 14;
static const U32 SERVER_ENTRY_VERT_GAP = 4; 
static const U32 SERVER_ENTRY_HEIGHT = SERVER_ENTRY_TEXTSIZE + SERVER_ENTRY_VERT_GAP;
static const U32 SEL_SERVER_INSTR_SIZE = 18;    // Size of "UP, DOWN TO SELECT..." text
static const U32 SEL_SERVER_INSTR_GAP_ABOVE_DIVIDER_LINE = 10;

// Positions of things on the screen
static const U32 BANNER_HEIGHT = 76;  // Height of top green banner area
static const U32 COLUMN_HEADER_TOP = BANNER_HEIGHT + 1;
static const U32 COLUMN_HEADER_HEIGHT = COLUMN_HEADER_TEXTSIZE + 6;

static const U32 TOP_OF_SERVER_LIST = BANNER_HEIGHT + COLUMN_HEADER_TEXTSIZE + 9;     // 9 provides some visual padding  [99]
static const U32 AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER = (SEL_SERVER_INSTR_SIZE + SERVER_DESCR_TEXTSIZE + SERVER_ENTRY_VERT_GAP + 10);

// "Chat window" includes chat composition, but not list of names of people in chatroom
#define BOTTOM_OF_CHAT_WINDOW (gScreenInfo.getGameCanvasHeight() - vertMargin / 2 - CHAT_NAMELIST_SIZE)

// Some colors
static const Color red = Color(1,0,0);
static const Color green = Color(0,1,0);
static const Color yellow = Color(1,1,0);
static const Color blue = Color(0,0,1);
static const Color white = Color(1,1,1);

// Our one and only instantiation of this interface!
QueryServersUserInterface gQueryServersUserInterface;


// Button callbacks
static void nextButtonClickedCallback()
{
   gQueryServersUserInterface.advancePage();
}

static void prevButtonClickedCallback()
{
   gQueryServersUserInterface.backPage();
}


// Constructor
QueryServersUserInterface::QueryServersUserInterface()
{
   setMenuID(QueryServersScreenUI);
   mLastUsedServerId = 0;
   mSortColumn = 0;
   mLastSortColumn = 0;
   mHighlightColumn = 0;
   mSortAscending = true;
   mRecievedListOfServersFromMaster = false;
   mouseScrollTimer.setPeriod(10 * MenuUserInterface::MOUSE_SCROLL_INTERVAL);

   mServersPerPage = 8;    // To start with, anyway...

   // Column name, x-start pos
   columns.push_back(ColumnInfo("SERVER NAME", 3));
   columns.push_back(ColumnInfo("STAT", 400));
   columns.push_back(ColumnInfo("PING", 450));
   columns.push_back(ColumnInfo("PLAYERS/BOTS", 490));
   columns.push_back(ColumnInfo("ADDRESS", 610));

   selectedId = 0xFFFFFF;

   sort();
   mShouldSort = false;
   mShowChat = true;

   mMessageDisplayCount = 16;

   // Create our buttons
   S32 textsize = 12;
   S32 ypos = BANNER_HEIGHT - 30;

   Button prevButton = Button(horizMargin, ypos, textsize, 4, "PREV", white, yellow, prevButtonClickedCallback);
   Button nextButton = Button(gScreenInfo.getGameCanvasWidth() - horizMargin - 50, ypos, 
                              textsize, 4, "NEXT", white, yellow, nextButtonClickedCallback);
   
   buttons.push_back(prevButton);
   buttons.push_back(nextButton);
}

// Initialize: Runs when "connect to server" screen is shown
void QueryServersUserInterface::onActivate()
{
   servers.clear();     // Start fresh
   mRecievedListOfServersFromMaster = false;
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
      s.serverAddress.port = 28000;
      s.serverAddress.netNum[0] = Random::readI();
      s.maxPlayers = Random::readF() * 16 + 8;
      s.playerCount = Random::readF() * s.maxPlayers;
      s.pingTimedOut = false;
      s.everGotQueryResponse = false;
      s.serverDescr = "Here is  description.  There are many like it, but this one is mine.";
      s.msgColor = yellow; 
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


Vector<string> prevServerListFromMaster;
Vector<string> alwaysPingList;

// Checks for connection to master, and sets up timer to keep running this until it finds one.  Once a connection is located,
// it fires off a series of requests to the master asking for servers and chat names.
void QueryServersUserInterface::contactEveryone()
{
   mBroadcastPingSendTime = Platform::getRealMilliseconds();

   //Address broadcastAddress(IPProtocol, Address::Broadcast, 28000);
   //gClientGame->getNetInterface()->sendPing(broadcastAddress, mNonce);

   // Always ping these servers -- typically a local server
   for(S32 i = 0; i < alwaysPingList.size(); i++)
   {
      Address address(alwaysPingList[i].c_str());
      gClientGame->getNetInterface()->sendPing(address, mNonce);
   } 

   // Try to ping the servers from our fallback list if we're having trouble connecting to the master
   if(gClientGame->getTimeUnconnectedToMaster() > GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME) 
   {
      for(S32 i = 0; i < prevServerListFromMaster.size(); i++)
         gClientGame->getNetInterface()->sendPing(Address(prevServerListFromMaster[i].c_str()), mNonce);

      mGivenUpOnMaster = true;
   }

   // If we already have a connection to the Master, start the server query... otherwise, don't
   MasterServerConnection *conn = gClientGame->getConnectionToMaster();

   if(conn)
   {
      if(!mAnnounced)
      {
         conn->c2mJoinGlobalChat();    // Announce our presence in the chat room
         mAnnounced = true;
      }
      conn->startServerQuery();
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
   for(S32 i = 0; i < servers.size(); i++)
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
         servers.erase(i);    // ...bye-bye!
   }

   // Save servers from the master
   if(ipList.size() != 0) 
      prevServerListFromMaster.clear();    // Don't clear if we have nothing to add... 

   // Now add any new servers
   for(S32 i = 0; i < ipList.size(); i++)
   {
      bool isHidden = false;

      // Make sure this isn't a server we're banned from... if so, don't show the connection
      for(S32 j = 0; j < hidden.size(); j++)
      {
         if(hidden[j].serverAddress == Address(ipList[i]))
            isHidden = true;
         break;
      }

      if(isHidden)
         break;

      prevServerListFromMaster.push_back( Address(ipList[i]).toString() );

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
         s.msgColor = white;   // white messages
         servers.push_back(s);
         mShouldSort = true;
      }
   }

   mMasterRequeryTimer.reset(MasterRequeryTime);
   mWaitingForResponseFromMaster = false;
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
      s.msgColor = white;   // white messages
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
         s.state = ServerRef::ReceivedPing;
         s.identityToken = clientIdentityToken;
         pendingPings--;
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
         s.everGotQueryResponse = true;

         s.serverName = string(serverName).substr(0, MaxServerNameLen);
         s.serverDescr = string(serverDescr).substr(0, MaxServerDescrLen);
         s.msgColor = yellow;   // yellow server details
         s.state = ServerRef::ReceivedQuery;
         s.pingTime = Platform::getRealMilliseconds() - s.lastSendTime;
         s.lastSendTime = Platform::getRealMilliseconds();     // Record time our last query was recieved, so we'll know when to send again
         pendingQueries--;
      }
   }
   mShouldSort = true;
}


void QueryServersUserInterface::idle(U32 timeDelta)
{
   LineEditor::updateCursorBlink(timeDelta);
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
   if(pendingPings < MaxPendingPings)
   {
      for(S32 i = 0; i < servers.size() ; i++)
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
               s.msgColor = red;   // red for errors
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
               gClientGame->getNetInterface()->sendPing(s.serverAddress, s.sendNonce);
               pendingPings++;
               if(pendingPings >= MaxPendingPings)
                  break;
            }
         }
      }
   }

   // When all pings have been answered or have timed out, send out server status queries
   if(pendingPings == 0 && (pendingQueries < MaxPendingQueries))
   {
      for(S32 i = 0; i < servers.size(); i++)
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
                  servers.erase(i);
                  continue;
               }
               // Otherwise, we can deal with timeouts on remote servers
               s.serverName = "Query Timed Out";
               s.serverDescr = "No information: Server not responding to status query";
               s.msgColor = red;   // red for errors
               s.playerCount = s.maxPlayers = s.botCount = 0;
               s.state = ServerRef::ReceivedQuery;
               mShouldSort = true;

            }
            else
            {
               s.state = ServerRef::SentQuery;
               s.lastSendTime = time;
               gClientGame->getNetInterface()->sendQuery(s.serverAddress, s.sendNonce, s.identityToken);
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

   // Clear out our hidden server list
   for(S32 i = 0; i < hidden.size(); i++)
      if(time > hidden[i].timeUntilShow)
         hidden.erase(i);

   // Not sure about the logic in here... maybe this is right...
   if( (mMasterRequeryTimer.update(elapsedTime) && !mWaitingForResponseFromMaster) ||
            (!mGivenUpOnMaster && gClientGame->getTimeUnconnectedToMaster() > GIVE_UP_ON_MASTER_AND_GO_IT_ALONE_TIME) )
       contactEveryone();

   // Go to previous page if a server has gone away and the last server has disappeared from the current screen
   while(getFirstServerIndexOnCurrentPage() >= servers.size() && mPage > 0)
       mPage--;

}  // end idle


// Add a server to our list of servers not to show the client... usually happens when user has been kicked
// They'll see the server again at time
void QueryServersUserInterface::addHiddenServer(Address addr, U32 time)
{
   hidden.push_back(HiddenServer(addr, time));
}


bool QueryServersUserInterface::mouseInHeaderRow(const Point *pos)
{
   return pos->y >= COLUMN_HEADER_TOP && pos->y < COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT - 1;
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
      
      S32 indx = floor((gScreenInfo.getMousePos()->y - TOP_OF_SERVER_LIST + 2) / SERVER_ENTRY_HEIGHT) + 
                 getFirstServerIndexOnCurrentPage() - (mScrollingUpMode || mMouseAtBottomFixFactor ? 1 : 0);

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


extern void drawString(S32 x, S32 y, U32 size, const char *string);

static void renderDedicatedIcon()
{
   // Add a "D"
   UserInterface::drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "D");
}


static void renderTestIcon()
{
   // Add a "T"
   UserInterface::drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "T");
}


static void renderLockIcon()
{
   glBegin(GL_LINE_LOOP);
      glVertex2f(0,2);
      glVertex2f(0,4);
      glVertex2f(3,4);
      glVertex2f(3,2);
   glEnd();

   glBegin(GL_LINE_STRIP);
      glVertex2f(2.6, 2);
      glVertex2f(2.6, 1.3);
      glVertex2f(2.4, 0.9);
      glVertex2f(1.9, 0.6);
      glVertex2f(1.1, 0.6);
      glVertex2f(0.6, 0.9);
      glVertex2f(0.4, 1.3);
      glVertex2f(0.4, 2);
   glEnd();
}


extern Color gMasterServerBlue;

void QueryServersUserInterface::render()
{
   const S32 canvasWidth =  gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

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

   bool connectedToMaster = gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->isEstablished();
   
   if(connectedToMaster)
   {
      glColor(gMasterServerBlue);
      drawCenteredStringf(vertMargin - 8, 12, "Connected to %s", gClientGame->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      glColor(red);
      if(mGivenUpOnMaster)
         drawCenteredString(vertMargin - 8, 12, "Couldn't connect to Master Server - Using server list from last successful connect.");
      else
         drawCenteredString(vertMargin - 8, 12, "Couldn't connect to Master Server - Firewall issues? Do you have the latest version?");
   }

   // Show some chat messages
   if(mShowChat)
   {
       S32 dividerPos = getDividerPos();

      // Horizontal divider between game list and chat window
      glColor(white);
      glBegin(GL_LINES);
         glVertex2f(horizMargin, dividerPos);
         glVertex2f(canvasWidth - horizMargin, dividerPos);
      glEnd();


      S32 ypos = dividerPos + 3;      // 3 = gap after divider

      renderMessages(ypos, mMessageDisplayCount);

      renderMessageComposition(BOTTOM_OF_CHAT_WINDOW - CHAT_FONT_SIZE - 2 * CHAT_FONT_MARGIN);
      renderChatters(horizMargin, BOTTOM_OF_CHAT_WINDOW);
   }

   // Instructions at bottom of server selection section
   glColor(white);
   drawCenteredString(getDividerPos() - SEL_SERVER_INSTR_SIZE - SEL_SERVER_INSTR_GAP_ABOVE_DIVIDER_LINE, SEL_SERVER_INSTR_SIZE, 
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
      for(S32 i = 1; i >= 0; i--)
      {
         if(composingMessage() && !mJustMovedMouse)   // Disable selection highlight if we're typing a message
            glColor(i ? Color(0.4,0.4,0.4) : Color(0.8,0.8,0.8));
         else
            glColor(i ? Color(0,0,0.4) : blue);     

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
            glVertex2f(0, y);
            glVertex2f(canvasWidth, y);
            glVertex2f(canvasWidth, y + SERVER_ENTRY_TEXTSIZE + 4);
            glVertex2f(0, y + SERVER_ENTRY_TEXTSIZE + 4);
         glEnd();
      }

      S32 lastServer = min(servers.size() - 1, (mPage + 1) * getServersPerPage() - 1);

      for(S32 i = getFirstServerIndexOnCurrentPage(); i <= lastServer; i++)
      {
         y = TOP_OF_SERVER_LIST + (i - getFirstServerIndexOnCurrentPage()) * SERVER_ENTRY_HEIGHT + 2;
         ServerRef &s = servers[i];

         if(i == selectedIndex)
         {
            // Render server description at bottom
            glColor(s.msgColor);
            U32 serverDescrLoc = TOP_OF_SERVER_LIST + getServersPerPage() * SERVER_ENTRY_HEIGHT  ;
            drawString(horizMargin, serverDescrLoc, SERVER_DESCR_TEXTSIZE, s.serverDescr.c_str());    
         }

         // Truncate server name to fit in the first column...
         string sname = "";

         // ...but first, see if the name will fit without truncation... if so, don't bother
         if(getStringWidth(SERVER_ENTRY_TEXTSIZE, s.serverName.c_str()) < colwidth)
            sname = s.serverName;
         else
            for(size_t j = 0; j < s.serverName.length(); j++)
               if(getStringWidth(SERVER_ENTRY_TEXTSIZE, (sname + s.serverName.substr(j, 1)).c_str() ) < colwidth)
                  sname += s.serverName[j];
               else
                  break;

         glColor(white);
         drawString(columns[0].xStart, y, SERVER_ENTRY_TEXTSIZE, sname.c_str());

         // Render icons
         glColor(green);
         if(s.dedicated || s.test || s.pingTimedOut || !s.everGotQueryResponse)
         {
            glPushMatrix();
               glTranslatef(columns[1].xStart + 5, y + 2, 0);
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
               glTranslatef(columns[1].xStart + 25, y + 2, 0);
               if(s.pingTimedOut || !s.everGotQueryResponse)
                  drawString(0, 0, SERVER_ENTRY_TEXTSIZE, "?");
               else
               {
                  glScalef(3.65, 3.65, 1);
                  renderLockIcon();
               }
            glPopMatrix();
         }

         // Set color based on ping time
         if(s.pingTime < 100)
            glColor(green);
         else if(s.pingTime < 250)
            glColor(yellow);
         else
            glColor(red);

         drawStringf(columns[2].xStart, y, SERVER_ENTRY_TEXTSIZE, "%d", s.pingTime);

         // Color by number of players
         if(s.playerCount == s.maxPlayers)
            glColor(red);       // max players
         else if(s.playerCount == 0)
            glColor(yellow);    // no players
         else
            glColor(green);     // 1 or more players

         //if(s.playerCount < 0)      // U32 will never be < 0...
         //   drawString(columns[3].xStart, y, SERVER_ENTRY_TEXTSIZE, "?? / ??");
         //else
            drawStringf(columns[3].xStart, y, SERVER_ENTRY_TEXTSIZE, "%d / %d", s.playerCount, s.botCount);
         glColor(white);
         drawString(columns[4].xStart, y, SERVER_ENTRY_TEXTSIZE, s.serverAddress.toString());
      }
   }
   // Show some special messages if there are no servers, or we're not connected to the master
   else if(connectedToMaster && mRecievedListOfServersFromMaster)        // We have our response, and there were no servers
      drawmsg1 = true;
   else if(!connectedToMaster)     // Still waiting to connect to the master...
      drawmsg2 = true;


   renderColumnHeaders();

   if(drawmsg1 || drawmsg2)
      renderMessageBox(drawmsg1, drawmsg2);
}


void QueryServersUserInterface::renderTopBanner()
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Top banner
   glColor3f(0, 0.35, 0);
   glBegin(GL_POLYGON);
      glVertex2f(0, 0);
      glVertex2f(canvasWidth, 0);
      glVertex2f(canvasWidth, BANNER_HEIGHT);
      glVertex2f(0, BANNER_HEIGHT);
   glEnd();

   glColor(white);
   drawCenteredString(vertMargin + 7, 35, "BITFIGHTER GAME LOBBY");

   drawStringf(horizMargin, vertMargin, 12, "SERVERS: %d", servers.size());
   drawStringfr(canvasWidth, vertMargin, 12, "PAGE %d/%d", mPage + 1, getLastPage() + 1);
}


void QueryServersUserInterface::renderColumnHeaders()
{
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Draw vertical dividing lines
   glColor3f(0.7, 0.7, 0.7);

   for(S32 i = 1; i < columns.size(); i++)
   {
      glBegin(GL_LINES);
         glVertex2f(columns[i].xStart - 4, COLUMN_HEADER_TOP);
         glVertex2f(columns[i].xStart - 4, TOP_OF_SERVER_LIST + getServersPerPage() * SERVER_ENTRY_HEIGHT + 2);
      glEnd();
   }

   // Horizontal lines under column headers
   glBegin(GL_LINES);
      glVertex2f(0, COLUMN_HEADER_TOP);
      glVertex2f(canvasWidth, COLUMN_HEADER_TOP);

      glVertex2f(0, COLUMN_HEADER_TOP + COLUMN_HEADER_TEXTSIZE + 7);
      glVertex2f(canvasWidth, COLUMN_HEADER_TOP + COLUMN_HEADER_TEXTSIZE + 7);
   glEnd();


   // Column headers (will partially overwrite horizontal lines) 
   S32 x1 = max(columns[mSortColumn].xStart - 3, 1);    // Going to 0 makes line look too thin...
   S32 x2;
   if(mSortColumn == columns.size() - 1)
      x2 = canvasWidth - 1;
   else
      x2 = columns[mSortColumn+1].xStart - 5;

   for(S32 i = 1; i >= 0; i--)
   {
      // Render box around (behind, really) selected column
      glColor(i ? Color(.4, .4, 0) : white);
      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(x1, COLUMN_HEADER_TOP);
         glVertex2f(x2, COLUMN_HEADER_TOP);
         glVertex2f(x2, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1);
         glVertex2f(x1, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1);
      glEnd();
   }

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

      glColor(white);
      glBegin(GL_LINE_LOOP);
         glVertex2f(x1, COLUMN_HEADER_TOP);
         glVertex2f(x2, COLUMN_HEADER_TOP);
         glVertex2f(x2, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1);
         glVertex2f(x1, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1);
      glEnd();
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
   else
   {
      msg1 = "Contacting master server...";
      msg2 = "";
      lines = 1;
   }

   const S32 strwid = getStringWidth(fontsize, msg1);
   const S32 msgboxMargin = 20;

   S32 ypos = mShowChat ? TOP_OF_SERVER_LIST + 25 + (getDividerPos() - TOP_OF_SERVER_LIST) * 2 / 5 : canvasHeight / 2 ;
   ypos += (lines - 2) * (F32(fontsize + fontgap) * .5);

   const S32 ypos1 = ypos - lines * (fontsize + fontgap) - msgboxMargin;
   const S32 ypos2 = ypos + msgboxMargin;

   const S32 xpos1 = (canvasWidth - strwid) / 2 - msgboxMargin; 
   const S32 xpos2 = (canvasWidth + strwid) / 2 + msgboxMargin;

   for(S32 i = 1; i >= 0; i--)    // First fill, then outline
   {
      glColor(i ? Color(.4, 0, 0) : red);

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(xpos1, ypos1);
         glVertex2f(xpos1, ypos2);
         glVertex2f(xpos2, ypos2);
         glVertex2f(xpos2, ypos1);
      glEnd();
   }

   // Now text
   glColor(white);

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


// All key handling now under one roof!
void QueryServersUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   keyCode = convertJoystickToKeyboard(keyCode);
   mJustMovedMouse = (keyCode == MOUSE_LEFT || keyCode == MOUSE_MIDDLE || keyCode == MOUSE_RIGHT);
   mDraggingDivider = false;
   S32 currentIndex = -1;

   if(keyCode == KEY_ENTER || keyCode == BUTTON_START || keyCode == MOUSE_LEFT)     // Return - select highlighted server & join game
   {
      const Point *mousePos = gScreenInfo.getMousePos();

      if(keyCode == MOUSE_LEFT && mouseInHeaderRow(mousePos))
      {
         sortSelected();
      }
      else if(keyCode == MOUSE_LEFT && mousePos->y < COLUMN_HEADER_TOP)
      {
         // Check buttons -- they're all up here for the moment
         for(S32 i = 0; i < buttons.size(); i++)
            buttons[i].onClick(mousePos->x, mousePos->y);
      }
      else if(keyCode == MOUSE_LEFT && isMouseOverDivider())
      {
         mDraggingDivider = true;
      }
      else if(keyCode == MOUSE_LEFT && mousePos->y > COLUMN_HEADER_TOP + SERVER_ENTRY_HEIGHT * min(servers.size() + 1, getServersPerPage() + 2))
      {
         // Clicked too low... also do nothing
      }
      else
      {
         // If the user is composing a message and hits enter, submit the message
         if(keyCode == KEY_ENTER && composingMessage())
            issueChat();
         else
         {
            S32 currentIndex = getSelectedIndex();
            if(currentIndex == -1)
               return;

            if(servers.size() > currentIndex)      // Index is valid
            {
               leaveGlobalChat();

               // Join the selected game...   (what if we select a local server from the list...  wouldn't 2nd param be true?)
               joinGame(servers[currentIndex].serverAddress, servers[currentIndex].isFromMaster, false);
               mLastSelectedServer = servers[currentIndex];    // Save this because we'll need the server name when connecting.  Kind of a hack.

               // ...and clear out the server list so we don't do any more pinging
               servers.clear();
            }
         }
      }
   }
   else if(keyCode == KEY_ESCAPE)  // Return to main menu
   {
      UserInterface::playBoop();
      leaveGlobalChat();
      gMainMenuUserInterface.activate();
   }
   else if(keyCode == keyOUTGAMECHAT)           // Toggle half-height servers, full-height servers, and full chat overlay
   {
      mShowChat = !mShowChat;
      if(mShowChat) 
      {
         gChatInterface.activate();
         gChatInterface.setRenderUnderlyingUI(false);    // Don't want this screen to bleed through...
      }
   }

   else if(keyCode == KEY_LEFT)
   {
      mHighlightColumn--;
      if(mHighlightColumn < 0)
         mHighlightColumn = 0;

      if(keyCode == BUTTON_DPAD_LEFT)
         sortSelected();

   }
   else if(keyCode == KEY_RIGHT)
   {
      mHighlightColumn++;
      if(mHighlightColumn >= columns.size())
         mHighlightColumn = columns.size() - 1;

      if(keyCode == BUTTON_DPAD_RIGHT)
         sortSelected();
   }
   else if(keyCode == KEY_PAGEUP)
   {
      backPage();

      glutSetCursor(GLUT_CURSOR_NONE);        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
   }
   else if(keyCode == KEY_PAGEDOWN) 
   {
      advancePage();

      glutSetCursor(GLUT_CURSOR_NONE);        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
   }
   else if (keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)       // Do backspacey things
      mLineEditor.handleBackspace(keyCode);   
   else if(ascii)                               // Other keys - add key to message
     addCharToMessage(ascii);

   // The following keys only make sense if there are some servers to browse through
   else if(servers.size() == 0)
      return;

   else if(keyCode == KEY_UP)
   {
      currentIndex = getSelectedIndex() - 1;
      if(currentIndex < 0)
         currentIndex = servers.size() - 1;
      mPage = currentIndex / getServersPerPage(); 

      glutSetCursor(GLUT_CURSOR_NONE);        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
      selectedId = servers[currentIndex].id;
   }
   else if(keyCode == KEY_DOWN)
   {
      currentIndex = getSelectedIndex() + 1;
      if(currentIndex >= servers.size())
         currentIndex = 0;

      mPage = currentIndex / getServersPerPage();

      glutSetCursor(GLUT_CURSOR_NONE);        // Hide cursor when navigating with keyboard or joystick
      mItemSelectedWithMouse = false;
      selectedId = servers[currentIndex].id;
   }
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


void QueryServersUserInterface::onKeyUp(KeyCode keyCode)
{
   if(mDraggingDivider)
   {
      glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);      // Reset cursor
      mDraggingDivider = false;
   }
}


void QueryServersUserInterface::onMouseDragged(S32 x, S32 y) 
{
   const Point *mousePos = gScreenInfo.getMousePos();    // (used in some of the macro expansions)

   if(mDraggingDivider)
   {
      S32 MIN_SERVERS_PER_PAGE = 4;
      S32 MAX_SERVERS_PER_PAGE = 18;

      mServersPerPage = (mousePos->y - TOP_OF_SERVER_LIST - AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER) / SERVER_ENTRY_HEIGHT;

      // Bounds checking
      if(mServersPerPage < MIN_SERVERS_PER_PAGE)
         mServersPerPage = MIN_SERVERS_PER_PAGE;
      else if(mServersPerPage > MAX_SERVERS_PER_PAGE)
         mServersPerPage = MAX_SERVERS_PER_PAGE;

      mMessageDisplayCount = (BOTTOM_OF_CHAT_WINDOW - getDividerPos() - 4) / (CHAT_FONT_SIZE + CHAT_FONT_MARGIN) - 1;         
   }
}


// User is sorting by selected column
void QueryServersUserInterface::sortSelected()
{
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
void QueryServersUserInterface::onMouseMoved(S32 x, S32 y)
{
   const Point *mousePos = gScreenInfo.getMousePos();

   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse

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

   else if(isMouseOverDivider())
      glutSetCursor(GLUT_CURSOR_UP_DOWN);

   else
      mHighlightColumn = mSortColumn;

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
   if(mLineEditor.length() > 10)
   {
	   const char *str1 = mLineEditor.c_str();
	   S32 a = 0;

	   while(a < 9)      // compare character by character, now case insensitive
	   {       
		   if(str1[a] != "/connect "[a] && str1[a] != "/CONNECT "[a] ) 
            a = S32_MAX;
         else
		      a++;
	   }
	   if(a == 9)
      {
		   Address address(&str1[9]);
         //endGame(); // avoid error when in game, F5 and type "/Connect"  <== shouldn't be needed when this is in UIQueryServers.cpp
		   joinGame(address, false, false);
		   return;
	   }
   }
   ChatParent::issueChat();
}

////////////////////////////////////////
////////////////////////////////////////

// Contstructor -- x,y are UL corner of button
Button::Button(S32 x, S32 y, S32 textSize, S32 padding, const char *label, Color fgColor, Color hlColor, void (*onClickCallback)())
{
   mX = x;
   mY = y;
   mTextSize = textSize;
   mPadding = padding;
   mLabel = label;
   mFgColor = fgColor;
   mHlColor = hlColor;
   mTransparent = true;
   mOnClickCallback = onClickCallback;
}


bool Button::mouseOver(S32 mouseX, S32 mouseY)
{
   return(mouseX >= mX && mouseX <= mX + mPadding * 2 + UserInterface::getStringWidth(mTextSize, mLabel) &&
          mouseY >= mY && mouseY <= mY + mTextSize + mPadding * 2);
}


void Button::onClick(S32 mouseX, S32 mouseY)
{
   if(mOnClickCallback && mouseOver(mouseX, mouseY))
      mOnClickCallback();
}


void Button::render(S32 mouseX, S32 mouseY)
{
   S32 start = mTransparent ? 0 : 1;

   S32 labelLen = UserInterface::getStringWidth(mTextSize, mLabel);
   for(S32 i = start; i >= 0; i--)
   {
      if(mouseOver(mouseX, mouseY))
         glColor(i ? mBgColor : mHlColor * 2);
      else
         glColor(i ? mBgColor : mFgColor);         // Fill then border

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(mX,                           mY);
         glVertex2f(mX + mPadding * 2 + labelLen, mY);
         glVertex2f(mX + mPadding * 2 + labelLen, mY + mTextSize + mPadding * 2);
         glVertex2f(mX,                           mY + mTextSize + mPadding * 2);
      glEnd();
   }

   UserInterface::drawString(mX + mPadding, mY + mPadding, mTextSize, mLabel);
}
 


};


