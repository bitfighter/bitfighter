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

#include "UIQueryServers.h"
#include "UIMenus.h"

#include "../tnl/tnlRandom.h"

#include "masterConnection.h"
#include "gameNetInterface.h"
#include "game.h"
#include "gameType.h"

#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UIEditor.h"      // For convertWindowToCanvasCoord()

#include "keyCode.h"
#include "../glut/glutInclude.h"

namespace Zap
{
// Our one and only instantiation of this interface!
QueryServersUserInterface gQueryServersUserInterface;

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
   mouseScrollTimer.setPeriod(10*MenuUserInterface::MouseScrollInterval);

   // Column name, x-start pos
   columns.push_back(ColumnInfo("SERVER NAME", 3));
   columns.push_back(ColumnInfo("STAT", 250));
   columns.push_back(ColumnInfo("PING", 330));
   columns.push_back(ColumnInfo("PLAYERS", 395));
   columns.push_back(ColumnInfo("ADDRESS", 533));

   selectedId = 0xFFFFFF;

   sort();
   mShouldSort = false;
}

// Initialize: Runs when "connect to server" screen is shown
void QueryServersUserInterface::onActivate()
{
   servers.clear();     // Start fresh
   mRecievedListOfServersFromMaster = false;
   mItemSelectedWithMouse = false;
   mScrollingUpMode = false;
   mFirstServer = 0;

   /*
   // Populate server list with dummy data to see how it looks
   for(U32 i = 0;i < 512; i++)
   {
      ServerRef s;
      dSprintf(s.serverName, MaxServerNameLen, "Dummy Svr%8x", Random::readI());
      s.id = i;
      s.pingTime = Random::readF() * 512;
      s.serverAddress.port = 28000;
      s.serverAddress.netNum[0] = Random::readI();
      s.maxPlayers = Random::readF() * 16 + 8;
      s.playerCount = Random::readF() * s.maxPlayers;
      servers.push_back(s);
   }
   */
   mSortColumn = 0;
   mHighlightColumn = 0;
   pendingPings = 0;
   pendingQueries = 0;
   mSortAscending = true;
   mNonce.getRandom();

   contactEveryone();
}


void QueryServersUserInterface::contactEveryone()
{
   Address broadcastAddress(IPProtocol, Address::Broadcast, 28000);
   broadcastPingSendTime = Platform::getRealMilliseconds();

   gClientGame->getNetInterface()->sendPing(broadcastAddress, mNonce);

   // If we already have a connection to the Master, start the server query... otherwise, don't
   if(gClientGame->getConnectionToMaster())
   {
      gClientGame->getConnectionToMaster()->startServerQuery();
      mWaitingForResponseFromMaster = true;
   }
   else
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
         s.serverAddress.set(ipList[i]);
         s.sendCount = 0;
         s.pingTime = 9999;
         s.playerCount = s.maxPlayers = -1;
         s.isFromMaster = true;
         strcpy(s.serverName, "Internet Server");
         strcpy(s.serverDescr, "Internet Server -- attempting to connect");
         s.msgColor = Color(1,1,1);   // white messages
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
      s.pingTime = Platform::getRealMilliseconds() - broadcastPingSendTime;
      s.state = ServerRef::ReceivedPing;
      s.id = ++mLastUsedServerId;
      s.sendNonce = theNonce;
      s.identityToken = clientIdentityToken;
      s.serverAddress = theAddress;
      s.sendCount = 0;
      s.playerCount = -1;
      s.maxPlayers = -1;
      s.isFromMaster = false;
      strcpy(s.serverName, "LAN Server");
      strcpy(s.serverDescr, "LAN Server -- attempting to connect");
      s.msgColor = Color(1,1,1);   // white messages
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


void QueryServersUserInterface::gotQueryResponse(const Address &theAddress, const Nonce &clientNonce, const char *serverName, const char *serverDescr, U32 playerCount, U32 maxPlayers, bool dedicated, bool passwordRequired)
{
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];
      if(s.sendNonce == clientNonce && s.serverAddress == theAddress && s.state == ServerRef::SentQuery)
      {
         s.playerCount = playerCount;
         s.maxPlayers = maxPlayers;
         s.dedicated = dedicated;
         s.passwordRequired = passwordRequired;

         dSprintf(s.serverName, sizeof(s.serverName), "%s", serverName);
         dSprintf(s.serverDescr, sizeof(s.serverDescr), "%s", serverDescr);
         s.msgColor = Color(1,1,0);   // yellow server details
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
               strcpy(s.serverName, "PingTimedOut");
               strcpy(s.serverDescr, "Server not responding to pings");
               s.msgColor = Color(1,0,0);   // red for errors
               s.playerCount = 0;
               s.maxPlayers = 0;
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
               strcpy(s.serverName, "QueryTimedOut");
               strcpy(s.serverDescr, "Server not responding to status query");
               s.msgColor = Color(1,0,0);   // red for errors
               s.playerCount = s.maxPlayers = 0;
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

   if(mMasterRequeryTimer.update(elapsedTime) && !mWaitingForResponseFromMaster)
       contactEveryone();

}  // end idle


// Add a server to our list of servers not to show the client... usually happens when user has been kicked
// They'll see the server again at time
void QueryServersUserInterface::addHiddenServer(Address addr, U32 time)
{
   hidden.push_back(HiddenServer(addr, time));
}


S32 QueryServersUserInterface::getSelectedIndex()
{
   for(S32 i = 0; i < servers.size(); i++)
      if(servers[i].id == selectedId)
         return i;
   return -1;
}

extern void drawString(S32 x, S32 y, U32 size, const char *string);

static void renderDedicatedIcon()      // Rectangular box with horizontal lines
{
   // Draw a little rectangle
   //glBegin(GL_LINE_LOOP);
   //   glVertex2f(0,0);
   //   glVertex2f(0,4);
   //   glVertex2f(3,4);
   //   glVertex2f(3,0);
   //glEnd();

   // And some horizontal lines
   //glBegin(GL_LINES);
   //   glVertex2f(0.6, 1);
   //   glVertex2f(2.4, 1);
   //   glVertex2f(0.6, 2);
   //   glVertex2f(2.4, 2);
   //   glVertex2f(0.6, 3);
   //   glVertex2f(2.4, 3);
   //glEnd();

   // Add a "D"
   UserInterface::drawString(0, 0, 4, "D");
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
extern void glColor(Color c, float alpha = 1);

#define MENU_ITEM_HEIGHT 24
#define ITEMS_TOP (vertMargin + 55 + 30 + MENU_ITEM_HEIGHT)     // Need parens here because this is used in a subtraction below
#define COLUMNS_TOP ITEMS_TOP - MENU_ITEM_HEIGHT - 10 - MENU_ITEM_HEIGHT
#define COLUMN_HEADER_HEIGHT MENU_ITEM_HEIGHT + 5
#define MOUSE_IN_HEADER_ROW mousePos.y >= COLUMNS_TOP && mousePos.y < COLUMNS_TOP + COLUMN_HEADER_HEIGHT - 1


void QueryServersUserInterface::render()
{
   const S32 fontsize = 20;
   const S32 msgboxMargin = 20;
   const S32 fontgap = 5;
   bool drawmsg1 = false;
   bool drawmsg2 = false;

   if(mShouldSort)
   {
      mShouldSort = false;
      sort();
   }

   bool connectedToMaster = gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->isEstablished();

   if(connectedToMaster)
   {
      glColor(gMasterServerBlue);
      drawCenteredStringf(vertMargin - 8, 12, "Connected to Master Server %s", gClientGame->getConnectionToMaster()->getMasterName().c_str() );
   }
   else
   {
      glColor3f(1, 0, 0);
      drawCenteredString(vertMargin - 8, 12, "Couldn't connect to Master Server - Firewall issues? Do you have the latest version?");
   }

   // Instructions at bottom of screen
   glColor3f(1,1,1);
   drawCenteredString(vertMargin + 7, 35, "CHOOSE A SERVER TO JOIN:");
   drawCenteredString(canvasHeight - vertMargin - 40, 18, "UP, DOWN to select, ENTER to join.");
   drawCenteredString(canvasHeight - vertMargin - 20, 18, "LEFT, RIGHT select sort column, SPACE to sort.  ESC exits.");

   U32 bottom = canvasHeight - vertMargin - 60;

   S32 x1 = columns[mSortColumn].xStart - 3;
   S32 x2;
   if(mSortColumn == columns.size() - 1)
      x2 = 799;
   else
      x2 = columns[mSortColumn+1].xStart - 5;

   // Render box around selected column
   glColor3f(.4, .4, 0);
   glBegin(GL_POLYGON);
      glVertex2f(x1, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
      glVertex2f(x1, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
   glEnd();

   glColor3f(1,1,1);
   glBegin(GL_LINE_LOOP);
      glVertex2f(x1, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
      glVertex2f(x1, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
   glEnd();

   // And render a box around the column under the mouse, if different
   x1 = columns[mHighlightColumn].xStart - 3;
   if(mHighlightColumn == columns.size() - 1)
      x2 = 799;
   else
      x2 = columns[mHighlightColumn+1].xStart - 5;

   glColor3f(1,1,1);
   glBegin(GL_LINE_LOOP);
      glVertex2f(x1, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + 5);
      glVertex2f(x2, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
      glVertex2f(x1, COLUMNS_TOP + COLUMN_HEADER_HEIGHT + 1);
   glEnd();


   // And now the column header text itself
   for(S32 i = 0; i < columns.size(); i++)
   {
      drawString(columns[i].xStart, COLUMNS_TOP - 20 + MENU_ITEM_HEIGHT, MENU_ITEM_HEIGHT, columns[i].name);
   }

   bool drawScrollUpArrow = false;
   bool drawScrollDnArrow = false;

   if(servers.size())      // There are servers to display...
   {
      // Find the selected server (it may have moved due to sort or new/removed servers)
      S32 selectedIndex = getSelectedIndex();
      if(selectedIndex == -1)
         selectedIndex = 0;

      S32 bonusTopOffset = 0; 
      // We can show a couple more if we don't need to scroll...
      if(servers.size() <= totalRows + 2)    
      {
         mFirstServer = 0;
         mLastServer = servers.size() - 1;
         bonusTopOffset = 1;
      }
      else  // There will be scrolling... lots of oddball cases to be handled
      {
         if(getSelectedIndex() == 0)
             mScrollingUpMode = false;

         if(mScrollingUpMode)
         {
            mFirstServer = servers.size() - totalRows - 1 + (mItemSelectedWithMouse ? 1 : 0);
            if(getSelectedIndex() < mFirstServer + 1)
               mFirstServer = getSelectedIndex() - 1;
         }
         else  // scrollingDownMode
            mFirstServer = getSelectedIndex() - totalRows + 1 - (mItemSelectedWithMouse ? 1 : 0);

         if(mFirstServer < 0)
            mFirstServer = 0;

         mLastServer = mFirstServer + totalRows;
         if(mLastServer >= servers.size())
            mLastServer = servers.size() - 1;
         
         if(mFirstServer == 0)    // First sever should replace arrow
         {
            mLastServer++;        // (will need one more to fill in the list if we're shifting it up)
            bonusTopOffset = 1;
         }
         else if(mFirstServer == 1)  // Server just prior to first server should replace arrow
         {
            mFirstServer = 0;
            bonusTopOffset = 1;
         }
         else     // Draw arrow
            drawScrollUpArrow = true;

         // None of the fancy stuff on the bottom... draw the arrow when needed, not when not
         if(mLastServer < servers.size() - 1)
            drawScrollDnArrow = true;
         else if(selectedIndex == mLastServer)     // Want to select last server without any scrolling action
            mScrollingUpMode = true; 
      }


      U32 y = ITEMS_TOP + (selectedIndex - mFirstServer) * MENU_ITEM_HEIGHT - bonusTopOffset * MENU_ITEM_HEIGHT;

      // Render box behind selected item
      glColor3f(0,0,0.4);     // pale blue
      glBegin(GL_POLYGON);
         glVertex2f(0, y);
         glVertex2f(799, y);
         glVertex2f(799, y + MENU_ITEM_HEIGHT - 1);
         glVertex2f(0, y + MENU_ITEM_HEIGHT - 1);
      glEnd();

      glColor3f(0,0,1);       // blue
      glBegin(GL_LINE_LOOP);
         glVertex2f(0, y);
         glVertex2f(799, y);
         glVertex2f(799, y + MENU_ITEM_HEIGHT - 1);
         glVertex2f(0, y + MENU_ITEM_HEIGHT - 1);
      glEnd();


      for(S32 i = mFirstServer; i <= mLastServer; i++)
      {
         y = ITEMS_TOP + (i - mFirstServer) * MENU_ITEM_HEIGHT - bonusTopOffset * MENU_ITEM_HEIGHT;
         const U32 fontSize = 21;
         ServerRef &s = servers[i];

         if(i == selectedIndex)
         {
            // Render description of selected server at bottom
            glColor(s.msgColor);   
            drawString(horizMargin, canvasHeight - vertMargin - 62, 18, s.serverDescr);
         }

         glColor3f(1,1,1);
         drawString(columns[0].xStart, y, fontSize, s.serverName);

         // Render icons
         glColor3f(0,1,0);
         if(s.dedicated || s.pingTimedOut)
         {
            glPushMatrix();
               glTranslatef(columns[1].xStart+5, y+2, 0);
               if(s.pingTimedOut)
                  drawString(0, 0, 20, "?");
               else
               {
                  glScalef(5, 5, 1);
                  renderDedicatedIcon();
               }
            glPopMatrix();
         }
         if(s.passwordRequired || s.pingTimedOut)
         {
            glPushMatrix();
               glTranslatef(columns[1].xStart + 25, y+2, 0);
               if(s.pingTimedOut)
                  drawString(0, 0, 20, "?");
               else
               {
                  glScalef(5, 5, 1);
                  renderLockIcon();
               }
            glPopMatrix();
         }

         // Set color based on ping time
         if(s.pingTime < 100)
            glColor3f(0,1,0);    // green
         else if(s.pingTime < 250)
            glColor3f(1,1,0);    // yellow
         else
            glColor3f(1,0,0);    // red
         drawStringf(columns[2].xStart, y, fontSize, "%d", s.pingTime);

         // Color by number of players
         if(s.playerCount == s.maxPlayers)
            glColor3f(1,0,0);    // max players -> red
         else if(s.playerCount == 0)
            glColor3f(1,1,0);    // no players -> yellow
         else
            glColor3f(0,1,0);    // 1 or more players -> green

         if(s.playerCount < 0)
            drawString(columns[3].xStart, y, fontSize, "?? / ??");
         else
            drawStringf(columns[3].xStart, y, fontSize, "%d / %d", s.playerCount, s.maxPlayers);
         glColor3f(1,1,1);
         drawString(columns[4].xStart, y, fontSize, s.serverAddress.toString());
      }
   }
   // Show some special messages if there are no servers, or we're not connected to the master
   else if(connectedToMaster && mRecievedListOfServersFromMaster)        // We have our response, and there were no servers
      drawmsg1 = true;
   else if(!connectedToMaster)     // Still waiting for a response...
      drawmsg2 = true;

   // Draw lines
   glColor3f(0.7, 0.7, 0.7);
   for(S32 i = 1; i < columns.size(); i++)
   {
      glBegin(GL_LINES);
      glVertex2f(columns[i].xStart - 4, COLUMNS_TOP);
      glVertex2f(columns[i].xStart - 4, ITEMS_TOP + (totalRows + 1) * MENU_ITEM_HEIGHT);
      glEnd();
   }
   glBegin(GL_LINES);
   glVertex2f(0, COLUMNS_TOP + MENU_ITEM_HEIGHT + 7);
   glVertex2f(800, COLUMNS_TOP + MENU_ITEM_HEIGHT + 7);
   glEnd();


   if(drawScrollUpArrow)
      MenuUserInterface::renderArrowAbove(ITEMS_TOP + 9);
   if(drawScrollDnArrow)
      MenuUserInterface::renderArrowBelow(ITEMS_TOP + (totalRows + 1) * MENU_ITEM_HEIGHT + 5);

#define msg "There are currently no games online."

   if(drawmsg1)
   {
      S32 strwid = getStringWidth(fontsize, msg);
      glColor3f(1,0,0);
      for(S32 i = 0; i < 2; i++)    // First fill, then outline
      {
         if(!i)
            glColor3f(1, 0, 0);
         else
            glColor3f(.4, 0, 0);

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
            glVertex2f(((canvasWidth - strwid) / 2) - msgboxMargin, canvasHeight / 2 - 2 * (fontsize + fontgap) - msgboxMargin);
            glVertex2f(((canvasWidth - strwid) / 2) - msgboxMargin, canvasHeight / 2 + msgboxMargin);
            glVertex2f(((canvasWidth + strwid) / 2) + msgboxMargin, canvasHeight / 2 + msgboxMargin);
            glVertex2f(((canvasWidth + strwid) / 2) + msgboxMargin, canvasHeight / 2  - 2 * (fontsize + fontgap) - msgboxMargin);
         glEnd();
      }

      glColor3f(1,1,1);
      drawCenteredString(canvasHeight / 2 - 2 * (fontsize + fontgap), fontsize, msg);
      drawCenteredString(canvasHeight / 2 - (fontsize + fontgap), fontsize, "Why don't you host one?");
#undef msg
   }
   else if(drawmsg2)
   {
      S32 strwid = getStringWidth(fontsize, "Contacting master server...");

      for(S32 i = 0; i < 2; i++)    // First fill, then outline
      {
         if(!i)
            glColor3f(1, 0, 0);
         else
            glColor3f(.4, 0, 0);

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(((canvasWidth - strwid) / 2) - msgboxMargin, canvasHeight / 2 - 2 * (fontsize + fontgap) - msgboxMargin);
         glVertex2f(((canvasWidth - strwid) / 2) - msgboxMargin, canvasHeight / 2 - 1 * (fontsize + fontgap) + msgboxMargin);
         glVertex2f(((canvasWidth + strwid) / 2) + msgboxMargin, canvasHeight / 2 - 1 * (fontsize + fontgap) + msgboxMargin);
         glVertex2f(((canvasWidth + strwid) / 2) + msgboxMargin, canvasHeight / 2  - 2 * (fontsize + fontgap) - msgboxMargin);
      glEnd();
      }

      glColor3f(1,1,1);
      drawCenteredString(canvasHeight / 2 - 2 * (fontsize + fontgap), fontsize, "Contacting master server...");
   }
}


extern Point gMousePos;

// All key handling now under one roof!
void QueryServersUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_SPACE)                  // Space - Sort by mSortColumn
   {
      sortSelected();
   }
   else if(keyCode == KEY_ENTER || keyCode == BUTTON_START || keyCode == MOUSE_LEFT)     // Return - select highlighted server & join game
   {
      Point mousePos = gEditorUserInterface.convertWindowToCanvasCoord(gMousePos);

      if(keyCode == MOUSE_LEFT && MOUSE_IN_HEADER_ROW)
      {
         sortSelected();
      }
      else if(keyCode == MOUSE_LEFT && mousePos.y < COLUMNS_TOP)
      {
         // Clicked too high... do nothing
      }
      else
      {
         S32 currentIndex = getSelectedIndex();
         if(currentIndex == -1)
            currentIndex = 0;

         if(servers.size() > currentIndex)      // Index is valid
         {
            // Join the selected game...   (what if we select a local server from the list...  wouldn't 2nd param be true?)
            joinGame(servers[currentIndex].serverAddress, servers[currentIndex].isFromMaster, false);

            // ...and clear out the server list so we don't do any more pinging
            servers.clear();
         }
      }
   }
   else if(keyCode == KEY_ESCAPE || keyCode == BUTTON_BACK)  // Return to main menu
   {
      UserInterface::playBoop();
      gMainMenuUserInterface.activate();
   }
   else if(keyCode == keyOUTGAMECHAT)           // Turn on Global Chat overlay
      gChatInterface.activate();


  else if(keyCode == KEY_LEFT)
  {
      mHighlightColumn--;
      if(mHighlightColumn < 0)
         mHighlightColumn = 0;
  }
  else if(keyCode == KEY_RIGHT)
  {
      mHighlightColumn++;
      if(mHighlightColumn >= columns.size())
         mHighlightColumn = columns.size() - 1;
  }

   // The following keys only make sense if there are some servers to browse through
   if(!servers.size())
      return;

   S32 currentIndex = getSelectedIndex();
   if(currentIndex == -1)
      currentIndex = 0;

   switch(keyCode)
   {
      case KEY_UP:
         currentIndex--;
         glutSetCursor(GLUT_CURSOR_NONE);            // Hide cursor when navigating with keyboard
         mItemSelectedWithMouse = false;
         break;
      case KEY_DOWN:
         currentIndex++;
         glutSetCursor(GLUT_CURSOR_NONE);            // Hide cursor when navigating with keyboard
         mItemSelectedWithMouse = false;
         break;
   }

   if(currentIndex < 0)
      currentIndex = 0;
   if(currentIndex >= servers.size())
      currentIndex = servers.size() - 1;

   selectedId = servers[currentIndex].id;
}

// User is sorting by selected column
void QueryServersUserInterface::sortSelected()
{
   mSortColumn = mHighlightColumn;

   if(mLastSortColumn == mSortColumn)
      mSortAscending = !mSortAscending;     // Toggle ascending/descending
   else
   {
      mLastSortColumn = mSortColumn;
      mSortAscending = true;
   }
   sort();
}


// Handle mouse input, figure out which menu item we're over, and highlight it
void QueryServersUserInterface::onMouseMoved(S32 x, S32 y)
{
   Point mousePos = gEditorUserInterface.convertWindowToCanvasCoord(gMousePos);

   if(MOUSE_IN_HEADER_ROW)
   {
      mHighlightColumn = 0;
      for(S32 i = columns.size()-1; i >= 0; i--)
         if(mousePos.x > columns[i].xStart)
         {
            mHighlightColumn = i;
            break;
         }
   }

   // It only makes sense to select a server if there are any servers to select... get it?
   if(servers.size() > 0)
   {
      S32 indx = (S32) floor(( mousePos.y - ITEMS_TOP ) / MENU_ITEM_HEIGHT) + mFirstServer + 1 - (mScrollingUpMode ? 1 : 0); 

      // See if this requires scrolling.  If so, limit speed.
      if(indx <= mFirstServer - 1)
      {
         if(!mouseScrollTimer.getCurrent())
         {
            indx = mFirstServer - 1;
            mouseScrollTimer.reset();
         }
         else
            return;
      }
      else if(indx > mLastServer)
      {
         if(!mouseScrollTimer.getCurrent())
         {
            indx = mLastServer + 1;
            mouseScrollTimer.reset();
         }
         else
            return;
      }

      if(indx < 0)
         indx = 0;
      else if(indx >= servers.size())
         indx = servers.size() - 1;

      selectedId = servers[indx].id;

      mItemSelectedWithMouse = true; 
   }

   glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);            // Show cursor when user moves mouse
}


#undef MENU_ITEM_HEIGHT
#undef ITEMS_TOP
#undef COLUMNS_TOP
#undef COLUMN_HEADER_HEIGHT
#undef MOUSE_IN_HEADER_ROW



// Sort server list by various columns
static S32 QSORT_CALLBACK compareFuncName(const void *a, const void *b)
{
   return stricmp(((QueryServersUserInterface::ServerRef *) a)->serverName,
                  ((QueryServersUserInterface::ServerRef *) b)->serverName);
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

static S32 QSORT_CALLBACK compareFuncAddress(const void *a, const void *b)
{
   return S32(((QueryServersUserInterface::ServerRef *) a)->serverAddress.netNum[0] -
          ((QueryServersUserInterface::ServerRef *) b)->serverAddress.netNum[0]);
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

};

