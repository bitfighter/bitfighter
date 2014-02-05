//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIQueryServers.h"

#include "UIMenus.h"
#include "UIManager.h"

#include "masterConnection.h"
#include "gameNetInterface.h"
#include "ClientGame.h"
#include "Colors.h"
#include "DisplayManager.h"
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
static const U32 BANNER_HEIGHT = 64;  // Height of top banner area
static const U32 COLUMN_HEADER_TOP = BANNER_HEIGHT + 1;
static const U32 COLUMN_HEADER_HEIGHT = COLUMN_HEADER_TEXTSIZE + 6;

static const U32 TOP_OF_SERVER_LIST = BANNER_HEIGHT + COLUMN_HEADER_TEXTSIZE + 9;     // 9 provides some visual padding  [99]
static const U32 AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER = (SEL_SERVER_INSTR_SIZE + SERVER_DESCR_TEXTSIZE + SERVER_ENTRY_VERT_GAP + 10);

// "Chat window" includes chat composition, but not list of names of people in chatroom
#define BOTTOM_OF_CHAT_WINDOW (DisplayManager::getScreenInfo()->getGameCanvasHeight() - vertMargin / 2 - CHAT_NAMELIST_SIZE)


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
QueryServersUserInterface::ServerRef::ServerRef(S32 serverId, const Address &address, State initialState, bool isFromMaster) :
   serverAddress(address)
{
   this->serverId = serverId;
   this->state = initialState;
   this->isFromMaster = isFromMaster;      

   pingTimedOut = false;
   everGotQueryResponse = false;
   passwordRequired = false;
   test = false;
   dedicated = false;
   sendCount = 0;
   pingTime = 9999;
   setPlayerBotMax(-1, 01, -1);

   id = getNextId();
   identityToken = 0;
   lastSendTime = 0;
}


// Destructor
QueryServersUserInterface::ServerRef::~ServerRef()
{
   // Do nothing
}


void QueryServersUserInterface::ServerRef::setNameDescr(const string &name, const string &descr, const Color &color)
{
   this->serverName = name.substr(0, MaxServerNameLen);
   this->serverDescr = descr.substr(0, MaxServerDescrLen);
   this->msgColor = color;
}


void QueryServersUserInterface::ServerRef::setPlayerBotMax(U32 playerCount, U32 botCount, U32 maxPlayers)
{
   this->playerCount = playerCount;
   this->maxPlayers = maxPlayers;
   this->botCount = botCount;
}


U32 QueryServersUserInterface::ServerRef::getNextId()
{
   static U32 nextId = 0;

   nextId++;
   return nextId;
}

////////////////////////////////////////
////////////////////////////////////////

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


////////////////////////////////////////
////////////////////////////////////////

// Constructor
QueryServersUserInterface::QueryServersUserInterface(ClientGame *game) : 
   UserInterface(game), 
   ChatParent(game)
{
   mSortColumn = getGame()->getSettings()->getQueryServerSortColumn();
   mSortAscending = getGame()->getSettings()->getQueryServerSortAscending();
   mLastSortColumn = mSortColumn;
   mHighlightColumn = 0;
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
   TNLAssert(DisplayManager::getScreenInfo()->getGameCanvasWidth() - columns[4].xStart + 2 * MIN_PAD > getStringWidth(COLUMN_HEADER_TEXTSIZE, columns[4].name), "Col[4] too narrow!");
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
   Button nextButton = Button(getGame(), DisplayManager::getScreenInfo()->getGameCanvasWidth() - horizMargin - 50, ypos, 
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

      ServerRef s(0, ServerRef::ReceivedPing, true);

      s.setNameDescr(name, "This is my description.  There are many like it, but this one is mine.", Colors::yellow);
      s.setPlayerBotMax(Random::readF() * max / 2, Random::readF() * max / 2, max);
      s.pingTime = Random::readF() * 512;
      s.serverAddress.port = GameSettings::DEFAULT_GAME_PORT;
      s.serverAddress.netNum[0] = Random::readI();
      U32 max = Random::readF() * 16 + 8;
      s.pingTimedOut = false;
      s.everGotQueryResponse = false;
      servers.push_back(s);
   }
#endif
   
   mHighlightColumn = mSortColumn;
   pendingPings = 0;
   pendingQueries = 0;
   mNonce.getRandom();
   mEmergencyRemoteServerNonce.getRandom();

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
         getGame()->getNetInterface()->sendPing(Address(serverList->get(i).c_str()), mEmergencyRemoteServerNonce);

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


// Returns index of found server, -1 if it found none
static S32 findServerByAddress(const Vector<ServerAddr> &serverList, const Address &address)
{
   for(S32 i = 0; i < serverList.size(); i++)
      if(Address(serverList[i].first) == address)
         return i;

   return -1;
}


// Returns index of found server, -1 if it found none
static S32 findServerByAddressOrId(const Vector<QueryServersUserInterface::ServerRef> &serverList, 
                                   const Address &address, S32 serverId)
{
   for(S32 i = 0; i < serverList.size(); i++)
      if(serverList[i].serverAddress == address || (serverId != 0 && serverList[i].serverId == serverId))
         return i;

   return -1;
}


// Returns index of found server, -1 if it found none.  ServerId of 0 means id is uninitialized.
static S32 findServerByServerId(const Vector<QueryServersUserInterface::ServerRef> &serverList, S32 serverId)
{
   if(serverId != 0)
      for(S32 i = 0; i < serverList.size(); i++)
         if(serverList[i].serverId == serverId)
            return i;

   return -1;
}


// Returns index of found server, -1 if it found none
static S32 findServerByAddressNonceState(const Vector<QueryServersUserInterface::ServerRef> &serverList, const Address &address, 
                                         const Nonce &nonce, QueryServersUserInterface::ServerRef::State state)
{
   for(S32 i = 0; i < serverList.size(); i++)
      if(serverList[i].serverAddress == address && serverList[i].sendNonce == nonce && serverList[i].state == state)
         return i;

   return -1;
}


// The master has given us a list of servers it knows about.  We need to scan our local server list and remove any that are
// not on the updated list from the master.  These will be servers that were alive, but have now disappeared.
void QueryServersUserInterface::forgetServersNoLongerOnList(const Vector<ServerAddr> &serverListFromMaster)
{
   for(S32 i = servers.size() - 1; i >= 0; i--)
   {
      if(!servers[i].isFromMaster)  // Skip local servers
         continue;

      S32 index = findServerByAddress(serverListFromMaster, servers[i].serverAddress);

      if(index == -1)               // It's a defunct server...
         servers.erase_fast(i);     // ...bye-bye!
   }
}


// Save servers from the master -- we'll use these as a fallback next time if we can't connect to the server
static void saveServerListToIni(GameSettings *settings, const Vector<ServerAddr> &serverListFromMaster)
{
   if(serverListFromMaster.size() != 0)
   {
      Vector<string> &prevServerList = settings->getIniSettings()->prevServerListFromMaster;
      prevServerList.clear();    // Only clear the saved list if we have something to add... 

      for(S32 i = 0; i < serverListFromMaster.size(); i++)
         prevServerList.push_back(Address(serverListFromMaster[i].first).toString());
   }
}


void QueryServersUserInterface::gotServerListFromMaster(const Vector<ServerAddr> &serverList)
{
   mReceivedListOfServersFromMaster = true;
   addServersToPingList(serverList);
}


// Master server has returned a list of servers that match our original criteria (including being of the
// correct version).  Send a query packet to each.
void QueryServersUserInterface::addServersToPingList(const Vector<ServerAddr> &serverList)
{
   saveServerListToIni(getGame()->getSettings(), serverList);

   forgetServersNoLongerOnList(serverList);

   // Add any new servers to the server display
   for(S32 i = 0; i < serverList.size(); i++)
   {
      // Is this server already in our list?
      S32 index = findServerByAddressOrId(servers, Address(serverList[i].first), serverList[i].second);

      if(index == -1)  // Not found -- it's a new server; create a new entry in the servers list
      {
         ServerRef server(serverList[i].second, serverList[i].first, ServerRef::Start, true);
         server.setNameDescr("Internet Server",  "Internet Server -- attempting to connect", Colors::white);

         server.sendNonce.getRandom();
         servers.push_back(server);

         mShouldSort = true;
      }
   }

   mMasterRequeryTimer.reset(MasterRequeryTime);
   mWaitingForResponseFromMaster = false;
}


void QueryServersUserInterface::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken, S32 serverId)
{
   if(nonce == mNonce || nonce == mEmergencyRemoteServerNonce)     // From local broadcast ping or direct ping of remote server
   {
      // Most of the time, this will be a local network server, and isLocal will be true.  It will only be false if we
      // are having problems connecting to the master and we broadcast our own set of pings to previously seen
      // servers.
      bool isLocal = nonce == mNonce;     

      // If we already know about the server, move along.
      // Pass 0 here to disable id check... we're only interested in IP address matches at this point -- if we have
      // a remote server with the same ID, we want to clobber it below.
      if(findServerByAddressOrId(servers, address, serverId) != -1)     
         return;

      // See if we've already been told about server with this serverId by the master... if so, we'll remove that
      // entry and replace it with a new one for the LAN server.  Local servers represent!
      S32 index = findServerByServerId(servers, serverId);
      if(index != -1 && isLocal && servers[index].isFromMaster)
         servers.erase_fast(index);

      // Create a new server entry
      ServerRef s(serverId, address, ServerRef::ReceivedPing, false);

      if(isLocal)
         s.setNameDescr("LAN Server", "LAN Server -- attempting to connect", Colors::white);
      else
         s.setNameDescr("Internet Server",  "Internet Server -- attempting to connect", Colors::white);

      s.pingTime = Platform::getRealMilliseconds() - mBroadcastPingSendTime;
      s.identityToken = clientIdentityToken;
      s.sendNonce = nonce;
      s.isFromMaster = !isLocal;

      servers.push_back(s);
   } 

   else  // From a ping sent to a remote server
   {
      S32 index = findServerByAddressNonceState(servers, address, nonce, ServerRef::SentPing);

      if(index > -1)
      {
         ServerRef &s = servers[index];
         s.pingTime = Platform::getRealMilliseconds() - s.lastSendTime;
         s.identityToken = clientIdentityToken;
         s.state = ServerRef::ReceivedPing;

         pendingPings--;
      }
   }

   mShouldSort = true;
}


void QueryServersUserInterface::gotQueryResponse(const Address &address, S32 serverId, const Nonce &clientNonce, 
                                                 const char *serverName, const char *serverDescr, U32 playerCount, 
                                                 U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   for(S32 i = 0; i < servers.size(); i++)
   {
      ServerRef &s = servers[i];
      if(s.sendNonce == clientNonce && s.serverAddress == address && s.state == ServerRef::SentQuery)
      {
         s.setNameDescr(serverName, serverDescr, Colors::yellow);
         s.setPlayerBotMax(playerCount, botCount, maxPlayers);
         s.pingTime = Platform::getRealMilliseconds() - s.lastSendTime;

         s.dedicated = dedicated;
         s.test = test;
         s.passwordRequired = passwordRequired;
         s.sendCount = 0;              // Fix random "Query/ping timed out"
         s.everGotQueryResponse = true;

         // If serverId has changed, it means this is a locally hosted server that we first saw before it contacted
         // the master to get a serverId.  We want to make sure we don't have another version of this same server from 
         // the master.  Find the dupe and kill it.
         if(s.serverId != serverId && serverId != 0)
         {
            TNLAssert(!s.isFromMaster, "Expected a local server!");
            
            S32 index = findServerByServerId(servers, serverId);
            TNLAssert(servers[index].isFromMaster, "Expected a remote server!");

            servers.erase_fast(index);
         }

         s.serverId = serverId;

         // Record time our last query was received, so we'll know when to send again
         s.lastSendTime = Platform::getRealMilliseconds();     

         if(!s.isFromMaster)
            s.pingTimedOut = false;    // Cures problem with local servers incorrectly displaying ?s for first 15 seconds


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
      if(pendingPings < MaxPendingPings)
      {
         ServerRef &s = servers[i];
         if(s.state == ServerRef::Start)     // This server is at the beginning of the process
         {
            s.pingTimedOut = false;
            s.sendCount++;
            if(s.sendCount > PingQueryRetryCount)     // Ping has timed out, sadly
            {
               s.setNameDescr("Ping Timed Out", "No information: Server not responding to pings", Colors::red);
               s.setPlayerBotMax(0, 0, 0);
               s.pingTime = 999;

               s.state = ServerRef::ReceivedQuery;    // In effect, this will tell app not to send any more pings or queries to this server
               s.pingTimedOut = true;

               mShouldSort = true;
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
   for(S32 i = servers.size() - 1; i >= 0; i--)
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
               s.setNameDescr("Query Timed Out", "No information: Server not responding to status query", Colors::red);
               s.setPlayerBotMax(0, 0, 0);

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
            s.state = ServerRef::Start;            // Will trigger a new round of pinging
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

   if(mShouldSort)
   {
      mShouldSort = false;
      sort();
   }

}  // end idle


bool QueryServersUserInterface::mouseInHeaderRow(const Point *pos)
{
   return pos->y >= COLUMN_HEADER_TOP && pos->y < COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT - 1;
}


string QueryServersUserInterface::getLastSelectedServerName()
{
   return mLastSelectedServerName;
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
      S32 indx = S32(floor((DisplayManager::getScreenInfo()->getMousePos()->y - TOP_OF_SERVER_LIST + 2) / SERVER_ENTRY_HEIGHT) + 
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


static void setLocalRemoteColor(bool isRemote)
{
   if(isRemote)
      glColor(Colors::white);
   else
      glColor(Colors::cyan);
}


// Set color based on ping time
static void setPingTimeColor(U32 pingTime)
{
   if(pingTime < 100)
      glColor(Colors::green);
   else if(pingTime < 250)
      glColor(Colors::yellow);
   else
      glColor(Colors::red);
}


// Set color by number of players
static void setPlayerCountColor(S32 players, S32 maxPlayers)
{
   Color color;
   if(players == maxPlayers)
      color = Colors::red;       // max players
   else if(players == 0)
      color = Colors::yellow;    // no players
   else
      color = Colors::green;     // 1 or more players

   glColor(color * 0.5);         // dim color
}


void QueryServersUserInterface::render()
{
   const S32 canvasWidth =  DisplayManager::getScreenInfo()->getGameCanvasWidth();

   bool drawmsg1 = false;
   bool drawmsg2 = false;

   renderTopBanner();

   // Render buttons
   const Point *mousePos = DisplayManager::getScreenInfo()->getMousePos();

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

      // Color background of local servers
      S32 lastServer = min(servers.size() - 1, (mPage + 1) * getServersPerPage() - 1);

      for(S32 i = getFirstServerIndexOnCurrentPage(); i <= lastServer; i++)
      {
         U32 y = TOP_OF_SERVER_LIST + (i - getFirstServerIndexOnCurrentPage()) * SERVER_ENTRY_HEIGHT + 1;
         ServerRef &s = servers[i];

         if(!s.isFromMaster)
         {
            glColor(Colors::red, .25);
            drawFilledRect(0, y, canvasWidth, y + SERVER_ENTRY_TEXTSIZE + 4);
         }
      }


      U32 y = TOP_OF_SERVER_LIST + (selectedIndex - getFirstServerIndexOnCurrentPage()) * SERVER_ENTRY_HEIGHT;

      // Render box behind selected item -- do this first so that it will not obscure descenders on letters like g in the column above
      bool disabled = composingMessage() && !mJustMovedMouse;
      drawMenuItemHighlight(0, y, canvasWidth, y + SERVER_ENTRY_TEXTSIZE + 4, disabled);


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

         setLocalRemoteColor(s.isFromMaster);

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

         setPingTimeColor(s.pingTime);
         drawStringf(columns[2].xStart, y, SERVER_ENTRY_TEXTSIZE, "%d", s.pingTime);

         setPlayerCountColor(s.playerCount, s.maxPlayers);

         drawStringf(columns[3].xStart + 30, y, SERVER_ENTRY_TEXTSIZE, "/%d", s.maxPlayers);
         drawStringf(columns[3].xStart,      y, SERVER_ENTRY_TEXTSIZE, "%d",  s.playerCount);
         drawStringf(columns[3].xStart + 78, y, SERVER_ENTRY_TEXTSIZE, "%d",  s.botCount);

         setLocalRemoteColor(s.isFromMaster);
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
   const S32 canvasWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth();

   // Top banner
   glColor(Colors::black);
   F32 vertices[] = {
         0,                0,
         (F32)canvasWidth, 0,
         (F32)canvasWidth, (F32)BANNER_HEIGHT,
         0,                (F32)BANNER_HEIGHT
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

   glColor(Colors::green);
   drawCenteredString(vertMargin + 12, 24, "BITFIGHTER GAME LOBBY");

   const S32 FONT_SIZE = 12;
   glColor(Colors::white);
   drawStringf(horizMargin, vertMargin, FONT_SIZE, "SERVERS: %d", servers.size());
   drawStringfr(canvasWidth - horizMargin, vertMargin, FONT_SIZE, "PAGE %d/%d", mPage + 1, getLastPage() + 1);
}


void QueryServersUserInterface::renderColumnHeaders()
{
   S32 canvasWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth();

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

   drawFilledRect(x1, COLUMN_HEADER_TOP, x2, COLUMN_HEADER_TOP + COLUMN_HEADER_HEIGHT + 1, Colors::gray20, Colors::white);

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

   S32 canvasWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth();
   S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();

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

   S32 ypos = mShowChat ? TOP_OF_SERVER_LIST + 25 + (getDividerPos() - TOP_OF_SERVER_LIST) * 2 / 5 : canvasHeight / 2;
   ypos += (lines - 2) * (fontsize + fontgap) / 2;

   const S32 ypos1 = ypos - lines * (fontsize + fontgap) - msgboxMargin;
   const S32 ypos2 = ypos + msgboxMargin;

   const S32 xpos1 = (canvasWidth - strwid) / 2 - msgboxMargin; 
   const S32 xpos2 = (canvasWidth + strwid) / 2 + msgboxMargin;

   static const S32 CORNER_INSET = 15;

   drawFilledFancyBox(xpos1, ypos1, xpos2, ypos2, CORNER_INSET, Colors::red40, 1.0, Colors::red);

   // Draw text
   glColor(Colors::white);

   drawCenteredString(ypos - lines * (fontsize + fontgap), fontsize, msg1);
   drawCenteredString(ypos - (fontsize + fontgap), fontsize, msg2);
}


void QueryServersUserInterface::recalcCurrentIndex()
{
   S32 indx = mPage * getServersPerPage() + selectedId % getServersPerPage() - 1;
   if(indx < 0)
      indx = 0;
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

   if(checkInputCode(BINDING_OUTGAMECHAT, inputCode))
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
      const Point *mousePos = DisplayManager::getScreenInfo()->getMousePos();

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

               // Save this because we'll need the server name when connecting.  Kind of a hack.
               mLastSelectedServerName = servers[currentIndex].serverName;    

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
      getUIManager()->reactivatePrevUI();      // MainMenuUserInterface
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
   const Point *mousePos = DisplayManager::getScreenInfo()->getMousePos();    // (used in some of the macro expansions)

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

   // Finally, save the current sort column to the INI
   getGame()->getSettings()->setQueryServerSortColumn(mSortColumn, mSortAscending);
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

   const Point *mousePos = DisplayManager::getScreenInfo()->getMousePos();

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
      return DisplayManager::getScreenInfo()->getGameCanvasHeight();
}


S32 QueryServersUserInterface::getServersPerPage()
{
   if(mShowChat)
      return mServersPerPage;
   else
      return (DisplayManager::getScreenInfo()->getGameCanvasHeight() - TOP_OF_SERVER_LIST - AREA_BETWEEN_BOTTOM_OF_SERVER_LIST_AND_DIVIDER) / SERVER_ENTRY_HEIGHT;
}


S32 QueryServersUserInterface::getLastPage()
{
   return (servers.size() - 1) / mServersPerPage;
}


bool QueryServersUserInterface::isMouseOverDivider()
{
   if(!mShowChat)       // Divider is only in operation when window is split
      return false;

   F32 mouseY = DisplayManager::getScreenInfo()->getMousePos()->y;

   S32 hitMargin = 4;
   S32 dividerPos = getDividerPos();

   return (mouseY >= dividerPos - hitMargin) && (mouseY <= dividerPos + hitMargin);
}


#define CAST_AB_TO_SERVERAB_AND_PUT_LOCAL_SERVERS_ON_TOP                                        \
   QueryServersUserInterface::ServerRef *serverA = (QueryServersUserInterface::ServerRef *) a;  \
   QueryServersUserInterface::ServerRef *serverB = (QueryServersUserInterface::ServerRef *) b;  \
                                                                                                \
   if(serverA->isFromMaster != serverB->isFromMaster)                                           \
   {                                                                                            \
      if(!serverA->isFromMaster) return -1;                                                     \
      if(!serverB->isFromMaster) return 1;                                                      \
   }                                                                 


// Sort server list by various columns
static S32 QSORT_CALLBACK compareFuncName(const void *a, const void *b)
{
   CAST_AB_TO_SERVERAB_AND_PUT_LOCAL_SERVERS_ON_TOP;

   return stricmp(serverA->serverName.c_str(), serverB->serverName.c_str());
}


static S32 QSORT_CALLBACK compareFuncPing(const void *a, const void *b)
{
   CAST_AB_TO_SERVERAB_AND_PUT_LOCAL_SERVERS_ON_TOP;

   return S32(serverA->pingTime - serverB->pingTime);
}


static S32 QSORT_CALLBACK compareFuncPlayers(const void *a, const void *b)
{
   CAST_AB_TO_SERVERAB_AND_PUT_LOCAL_SERVERS_ON_TOP;

   S32 pc = S32(serverA->playerCount - serverB->playerCount);

   if(pc)
      return pc;

   return S32(serverA->maxPlayers - serverB->maxPlayers);
}


// First compare IPs, then, if equal, port numbers
static S32 QSORT_CALLBACK compareFuncAddress(const void *a, const void *b)
{
   CAST_AB_TO_SERVERAB_AND_PUT_LOCAL_SERVERS_ON_TOP;

   U32 netNumA = serverA->serverAddress.netNum[0];
   U32 netNumB = serverB->serverAddress.netNum[0];

   if(netNumA == netNumB)
      return (S32)(serverA->serverAddress.port - serverB->serverAddress.port);
   
   return S32(netNumA - netNumB);
}


void QueryServersUserInterface::sort()
{
   // In all cases, put local servers at the top of the list
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
      // Do nothing
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
   mBgColor = Colors::black;
   mOnClickCallback = onClickCallback;
}

// Destructor
Button::~Button()
{
   // Do nothing
}


bool Button::isMouseOver(F32 mouseX, F32 mouseY) const
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


void Button::render(F32 mouseX, F32 mouseY) const
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

   static const S32 INSET = 4;

   drawFilledFancyBox(mX, mY, mX + mPadding * 2 + labelLen, mY + mTextSize + mPadding * 2, INSET, mBgColor, 1.0f, color);
   drawString(mX + mPadding, mY + mPadding, mTextSize, mLabel);
}
 


};


