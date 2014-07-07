//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "game.h"

#include "GameManager.h"

#include "gameType.h"
#include "config.h"
#include "masterConnection.h"
#include "move.h"
#include "robot.h"
#include "stringUtils.h"
#include "SlipZone.h"  
#include "Teleporter.h"
#include "ServerGame.h"
#include "gameNetInterface.h"
#include "Level.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>

#include "../master/DatabaseAccessThread.h"

using namespace TNL;

namespace Zap
{


////////////////////////////////////////
////////////////////////////////////////

// Constructor
NameToAddressThread::NameToAddressThread(const char *address_string) : mAddress_string(address_string)
{
   mDone = false;
}

// Destructor
NameToAddressThread::~NameToAddressThread()
{
   // Do nothing
}


U32 NameToAddressThread::run()
{
   // This can take a lot of time converting name (such as "bitfighter.org:25955") into IP address.
   mAddress.set(mAddress_string);
   mDone = true;
   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

static Vector<DatabaseObject *> fillVector2;

////////////////////////////////////
////////////////////////////////////

// Statics -- should this really be a static??
//static Level *mObjectAddTarget = NULL;


// Constructor
Game::Game(const Address &theBindAddress)
{
   mLevelDatabaseId = 0;

   mNextMasterTryTime = 0;
   mReadyToConnectToMaster = false;

   mCurrentTime = 0;
   mGameSuspended = false;

   mRobotCount = 0;
   mPlayerCount = 0;

   mTimeUnconnectedToMaster = 0;

   mNetInterface = new GameNetInterface(theBindAddress, this);
   mHaveTriedToConnectToMaster = false;

   mNameToAddressThread = NULL;

   mObjectsLoaded = 0;

   mSecondaryThread = new Master::DatabaseAccessThread();
}


// Destructor
Game::~Game()
{
   if(mNameToAddressThread)
      delete mNameToAddressThread;
   delete mSecondaryThread;
}


F32 Game::getLegacyGridSize() const
{
   return mLevel->getLegacyGridSize();
}


U32 Game::getCurrentTime()
{
   return mCurrentTime;
}


const Vector<SafePtr<BfObject> > &Game::getScopeAlwaysList()
{
   return mScopeAlwaysList;
}


void Game::setScopeAlwaysObject(BfObject *theObject)
{
   mScopeAlwaysList.push_back(theObject);
}


//void Game::setAddTarget()
//{
//   mObjectAddTarget = mLevel.get();
//}
//
//
//// Clear the addTarget, but only if it's us -- this prevents the ServerGame destructor from wiping this out
//// after it has already been set by the editor after testing a level.
//void Game::clearAddTarget()
//{
//   if(mObjectAddTarget == mLevel.get())
//      mObjectAddTarget = NULL;
//}
//
//
//// When we're adding an object and don't know where to put it... put it here!
//// Static method
//Level *Game::getAddTarget()
//{
//   return mObjectAddTarget;
//}


bool Game::isSuspended() const
{
   return mGameSuspended;
}


GameSettings *Game::getSettings() const
{
   return &gSettings;      // For now until we're sure this is the way we want to go
}


S32    Game::getBotCount() const                          { TNLAssert(false, "Not implemented for this class!"); return 0; }
Robot *Game::findBot(const char *id)                      { TNLAssert(false, "Not implemented for this class!"); return NULL; }
string Game::addBot(const Vector<string> &args, ClientInfo::ClientClass clientClass)     
                                                          { TNLAssert(false, "Not implemented for this class!"); return "";    }

void   Game::kickSingleBotFromLargestTeamWithBots()  { TNLAssert(false, "Not implemented for this class!"); }
void   Game::moreBots()                              { TNLAssert(false, "Not implemented for this class!"); }
void   Game::fewerBots()                             { TNLAssert(false, "Not implemented for this class!"); }
Robot *Game::getBot(S32 index)                       { TNLAssert(false, "Not implemented for this class!"); return NULL; }
void   Game::addBot(Robot *robot)                    { TNLAssert(false, "Not implemented for this class!"); }
void   Game::removeBot(Robot *robot)                 { TNLAssert(false, "Not implemented for this class!"); }
void   Game::deleteBot(const StringTableEntry &name) { TNLAssert(false, "Not implemented for this class!"); }
void   Game::deleteBot(S32 i)                        { TNLAssert(false, "Not implemented for this class!"); }
void   Game::deleteBotFromTeam(S32 teamIndex)        { TNLAssert(false, "Not implemented for this class!"); }
void   Game::deleteAllBots()                         { TNLAssert(false, "Not implemented for this class!"); }
void   Game::balanceTeams()                          { TNLAssert(false, "Not implemented for this class!"); }



void Game::setReadyToConnectToMaster(bool ready)
{
   mReadyToConnectToMaster = ready;
}


// Static method
Point Game::getScopeRange(bool sensorEquipped)
{
   static const Point sensorScopeRange(PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL + PLAYER_SCOPE_MARGIN,
                                       PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL   + PLAYER_SCOPE_MARGIN);

   static const Point normalScopeRange(PLAYER_VISUAL_DISTANCE_HORIZONTAL + PLAYER_SCOPE_MARGIN,
                                       PLAYER_VISUAL_DISTANCE_VERTICAL   + PLAYER_SCOPE_MARGIN);

   return sensorEquipped ? sensorScopeRange : normalScopeRange;
}


S32 Game::getClientCount() const
{
   return mClientInfos.size();
}


// Return the number of human players (does not include bots)
S32 Game::getPlayerCount() const
{
   return mPlayerCount;
}


// Return the number of human players on a given team (does not include bots, used for testing)
S32 Game::getPlayerCount(S32 teamIndex) const
{
   S32 playerCount = 0;

   for(S32 i = 0; i < mClientInfos.size(); i++)
      if(mClientInfos[i]->getTeamIndex() == teamIndex)
         playerCount++;

   return playerCount;
}


S32 Game::getAuthenticatedPlayerCount() const
{
   S32 count = 0;
   for(S32 i = 0; i < mClientInfos.size(); i++)
      if(!mClientInfos[i]->isRobot() && mClientInfos[i]->isAuthenticated())
         count++;

   return count;
}


S32 Game::getRobotCount() const
{
   return mRobotCount;
}


ClientInfo *Game::getClientInfo(S32 index) const
{ 
   return mClientInfos[index]; 
}


const Vector<RefPtr<ClientInfo> > *Game::getClientInfos()
{
   return &mClientInfos;
}


// ClientInfo will be a RemoteClientInfo in ClientGame and a FullClientInfo in ServerGame
void Game::addToClientList(ClientInfo *clientInfo) 
{ 
   // Adding the same ClientInfo twice is never The Right Thing To Do
   //
   // NOTE - This can happen when a Robot line is found in a level file.  For some reason
   // it tries to get added twice to the game
   for(S32 i = 0; i < mClientInfos.size(); i++)
      if(mClientInfos[i] == clientInfo)
         return;

   mClientInfos.push_back(clientInfo);

   if(clientInfo->isRobot())
      mRobotCount++;
   else
      mPlayerCount++;
}     


// Helper function for other find functions
S32 Game::findClientIndex(const StringTableEntry &name)
{
   for(S32 i = 0; i < mClientInfos.size(); i++)
   {
      if(mClientInfos[i]->getName() == name) 
         return i;
   }

   return -1;     // Not found
}


void Game::removeFromClientList(const StringTableEntry &name)
{
   S32 index = findClientIndex(name);

   if(index >= 0)
   {
      if(mClientInfos[index]->isRobot())
         mRobotCount--;
      else
         mPlayerCount--;

      mClientInfos.erase_fast(index);
   }
}


void Game::removeFromClientList(ClientInfo *clientInfo)
{
   for(S32 i = 0; i < mClientInfos.size(); i++)
      if(mClientInfos[i] == clientInfo)
      {
         if(mClientInfos[i]->isRobot())
            mRobotCount--;
         else
            mPlayerCount--;

         mClientInfos.erase_fast(i);
         return;
      }
}


void Game::clearClientList() 
{ 
   mClientInfos.clear();   // ClientInfos are refPtrs, so this will delete them

   mRobotCount = 0;
   mPlayerCount = 0;
}


// Find clientInfo given a player name
ClientInfo *Game::findClientInfo(const StringTableEntry &name)
{
   S32 index = findClientIndex(name);

   return index >= 0 ? mClientInfos[index] : NULL;
}


// Currently only used on client, for various effects
// Will return NULL if ship is out-of-scope... we have ClientInfos for all players, but not aways their ships
Ship *Game::findShip(const StringTableEntry &clientName)
{
   ClientInfo *clientInfo = findClientInfo(clientName);
   
   if(clientInfo)
      return clientInfo->getShip();
   else
      return NULL;
}


GameNetInterface *Game::getNetInterface()
{
   return mNetInterface;
}


Level *Game::getGameObjDatabase()
{
   return mLevel.get();
}


MasterServerConnection *Game::getConnectionToMaster()
{
   return mConnectionToMaster;
}


S32 Game::getClientId()
{
   if(mConnectionToMaster)
      return mConnectionToMaster->getClientId();

   return 0;
}


// Only used for testing
void Game::setConnectionToMaster(MasterServerConnection *connection)
{
   TNLAssert(mConnectionToMaster.isNull(), "mConnectionToMaster not NULL");
   mConnectionToMaster = connection;
}


// Retrieve the current GameType.  Should always be a valid value on the Server, can
// be NULL on the client in the early stages of hosting before the GameType object
// has replicated from the server.
GameType *Game::getGameType() const
{
   TNLAssert(mLevel,                               "Should have a level by now!");
   TNLAssert(!isServer() || mLevel->getGameType(), "ServerGame should have a GameType by now!");

   return mLevel->getGameType();
}


S32 Game::getPlayerScore(S32 index) const
{
   return getClientInfo(index)->getScore();
}


void Game::setPlayerScore(S32 index, S32 score)
{
   if(index >= getClientCount())    // Could happen if server sends bad info
      return;

   getClientInfo(index)->setScore(score);
}


// There is a bigger need to use StringTableEntry and not const char *
// mainly to prevent errors on CTF neutral flag and out of range team number.
StringTableEntry Game::getTeamName(S32 teamIndex) const
{
   if(teamIndex >= 0 && teamIndex < getTeamCount())
      return mLevel->getTeamName(teamIndex);
   else if(teamIndex == TEAM_HOSTILE)
   {
      static const StringTableEntry hostile("Hostile");
      return hostile;
   }
   else if(teamIndex == TEAM_NEUTRAL)
   {
      static const StringTableEntry neutral("Neutral");
      return neutral;
   }
   else
   {
      static const StringTableEntry unknown("UNKNOWN"); 
      return unknown;
   }
}


// Given a player's name, return his team
S32 Game::getTeamIndex(const StringTableEntry &playerName)
{
   ClientInfo *clientInfo = findClientInfo(playerName);              // Returns NULL if player can't be found
   
   return clientInfo ? clientInfo->getTeamIndex() : TEAM_NEUTRAL;    // If we can't find the team, let's call it neutral
}


// The following just delegate their work to the TeamManager.  TeamManager will handle cleanup of any added teams.
void Game::removeTeam(S32 teamIndex)                  { mLevel->removeTeam(teamIndex);    }
void Game::addTeam(AbstractTeam *team)                { mLevel->addTeam(team);            }
void Game::addTeam(AbstractTeam *team, S32 index)     { mLevel->addTeam(team, index);     }
void Game::replaceTeam(AbstractTeam *team, S32 index) { mLevel->replaceTeam(team, index); }
void Game::clearTeams()                               { mLevel->clearTeams();             }



void Game::addPolyWall(BfObject *polyWall, GridDatabase *database)
{
   polyWall->addToGame(this, database);
}


// Is overridden in ClientGame
void Game::addWallItem(WallItem *wallItem, GridDatabase *database)
{
   //wallItem->addToGame(this, database);

   // Generate a series of 2-point wall segments, which are added to the spatial database
   Barrier::constructWalls(this, *wallItem->getOutline(), false, (F32)wallItem->getWidth());
   //wallItem->onAddedToGame();
}


const Vector<WallItem *> &Game::getWallList() const
{
   TNLAssert(false, "Not implemented for this class!");
   return NULL;
}
   

const Vector<PolyWall *> &Game::getPolyWallList() const
{
   TNLAssert(false, "Not implemented for this class!");
   return NULL;
}


void Game::setTeamHasFlag(S32 teamIndex, bool hasFlag)
{
   mLevel->setTeamHasFlag(teamIndex, hasFlag);
}


// Get slowing factor if we are in a slip zone; could be used if we have go faster zones
F32 Game::getShipAccelModificationFactor(const Ship *ship) const
{
   BfObject *obj = ship->isInZone(SlipZoneTypeNumber);

   if(obj)
   {
      SlipZone *slipzone = static_cast<SlipZone *>(obj);
      return slipzone->slipAmount;
   }

   return 1.0f;
}


void Game::teleporterDestroyed(Teleporter *teleporter)
{
   if(teleporter)
      teleporter->onDestroyed();       
}


S32           Game::getTeamCount()                const { return mLevel->getTeamCount();            } 
AbstractTeam *Game::getTeam(S32 teamIndex)        const { return mLevel->getTeam(teamIndex);        }
bool          Game::getTeamHasFlag(S32 teamIndex) const { return mLevel->getTeamHasFlag(teamIndex); }


// Find winner of a team-based game.
// Team with the most points wins.
// If multple teams are tied for most points, a tie is declared.
// If multiple teams are tied for most points, but only one has players, that team is declared the winner.
// If multiple teams are tied for most points, but none have players, those teams are declared winners by tie.
// Bots are considered players for this purpose.
TeamGameResults Game::getTeamBasedGameWinner() const
{
   S32 teamCount = getTeamCount();
   countTeamPlayers();

   TNLAssert(teamCount > 0, "Expect at least one team here!");
   if(teamCount == 0)
      return TeamGameResults(OnlyOnePlayerOrTeam, 0);

   if(teamCount == 1)
      return TeamGameResults(OnlyOnePlayerOrTeam, 0);

   S32 winningTeam = -1;
   S32 winningScore = S32_MIN;
   S32 winningTeamsWithPlayers = 0;

   GameEndStatus status = HasWinner;

   for(S32 i = 0; i < teamCount; i++)
   {
      if(getTeam(i)->getScore() == winningScore)
      {
         if(getTeam(i)->getPlayerBotCount() > 0)
         {
            // Is this the first team with players with the high score?
            if(winningTeamsWithPlayers == 0)
            {
               winningTeam = i;
               status = HasWinner;
            }
            else
               status = Tied;

            winningTeamsWithPlayers++;
         }

         // If no other teams with this score has players, then we can call it a tie
         else if(winningTeamsWithPlayers == 0)
            status = TiedByTeamsWithNoPlayers;
      }

      else if(getTeam(i)->getScore() > winningScore)
      {
         winningTeam = i;
         winningScore = getTeam(i)->getScore();
         status = HasWinner;
         winningTeamsWithPlayers = (getTeam(i)->getPlayerBotCount() == 0) ? 0 : 1;
      }
   }

   return TeamGameResults(status, winningTeam);
}


// Find winner of a non-team based game
IndividualGameResults Game::getIndividualGameWinner() const
{
   S32 clientCount = getClientCount();

   if(clientCount == 1)
      return IndividualGameResults(OnlyOnePlayerOrTeam, getClientInfo(0));

   ClientInfo *winningClient = getClientInfo(0);
   GameEndStatus status = HasWinner;

   for(S32 i = 1; i < clientCount; i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(clientInfo->getScore() == winningClient->getScore())
         status = Tied;

      else if(clientInfo->getScore() > winningClient->getScore())
      {
         winningClient = clientInfo;
         status = HasWinner;
      }
   }

   return IndividualGameResults(status, winningClient);
}



S32 Game::getTeamIndexFromTeamName(const char *teamName) const 
{ 
   for(S32 i = 0; i < mLevel->getTeamCount(); i++)
      if(stricmp(teamName, getTeamName(i).getString()) == 0)
         return i;

   if(stricmp(teamName, "Hostile") == 0)
      return TEAM_HOSTILE;
   if(stricmp(teamName, "Neutral") == 0)
      return TEAM_NEUTRAL;

   return NO_TEAM;
}


// Makes sure that the mTeams[] structure has the proper player counts
// Needs to be called manually before accessing the structure
// Bot counts do work on client.  Yay!
// Rating may only work on server... not tested on client
void Game::countTeamPlayers() const
{
   for(S32 i = 0; i < getTeamCount(); i++)
   {
      TNLAssert(dynamic_cast<Team *>(getTeam(i)), "Invalid team");      // Assert for safety
      static_cast<Team *>(getTeam(i))->clearStats();                    // static_cast for speed
   }

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      S32 teamIndex = clientInfo->getTeamIndex();

      if(teamIndex >= 0 && teamIndex < getTeamCount())
      { 
         // Robot could be neutral or hostile, skip out-of-range team numbers
         TNLAssert(dynamic_cast<Team *>(getTeam(teamIndex)), "Invalid team");
         Team *team = static_cast<Team *>(getTeam(teamIndex));            

         if(clientInfo->isRobot())
            team->incrementBotCount();
         else
            team->incrementPlayerCount();

         // The following bit won't work on the client... 
         if(isServer())
         {
            const F32 BASE_RATING = .1f;
            team->addRating(max(clientInfo->getCalculatedRating(), BASE_RATING));    
         }
      }
   }
}


// Finds biggest team that has bots; if two teams are tied for largest, will return index of the first
S32 Game::findLargestTeamWithBots() const
{
   countTeamPlayers();

   S32 largestTeamCount = 0;
   S32 largestTeamIndex = NONE;

   for(S32 i = 0; i < getTeamCount(); i++)
   {
      TNLAssert(dynamic_cast<Team *>(getTeam(i)), "Invalid team");
      Team *team = static_cast<Team *>(getTeam(i));

      // Must have at least one bot to be the largest team with bots!
      if(team->getPlayerBotCount() > largestTeamCount && team->getBotCount() > 0)
      {
         largestTeamCount = team->getPlayerBotCount();
         largestTeamIndex = i;
      }
   }

   return largestTeamIndex;
}


// Only called from editor
void Game::setGameType(GameType *gameType)
{
   mLevel->setGameType(gameType);
}


U32 Game::getTimeUnconnectedToMaster()
{
   return mTimeUnconnectedToMaster;
}


// Note: lots of stuff for this method in child classes!
void Game::onConnectedToMaster()
{
   getSettings()->saveMasterAddressListInIniUnlessItCameFromCmdLine();
}


// Only used during level load process...  actually, used at all?  If so, should be combined with similar code in gameType
// Not used during normal game load... used by tests and lua_setGameTime()
void Game::setGameTime(F32 timeInMinutes)
{
   GameType *gt = getGameType();

   TNLAssert(gt, "Null gametype!");

   if(gt)
      gt->setGameTime(timeInMinutes * 60);
}


// If there is no valid connection to master server, perodically try to create one.
// If user is playing a game they're hosting, they should get one master connection
// for the client and one for the server.
// Called from both clientGame and serverGame idle fuctions, so think of this as a kind of idle
void Game::checkConnectionToMaster(U32 timeDelta)
{
   if(mConnectionToMaster.isValid() && mConnectionToMaster->isEstablished())
      mTimeUnconnectedToMaster = 0;
   else if(mReadyToConnectToMaster)
      mTimeUnconnectedToMaster += timeDelta;

   if(!mConnectionToMaster.isValid())      // It's valid if it isn't null, so could be disconnected and would still be valid
   {
      Vector<string> *masterServerList = gSettings.getMasterServerList();

      if(masterServerList->size() == 0)
         return;

      if(mNextMasterTryTime < timeDelta && mReadyToConnectToMaster)
      {
         if(!mNameToAddressThread)
         {
            if(mHaveTriedToConnectToMaster && masterServerList->size() >= 2)
            {  
               // Rotate the list so as to try each one until we find one that works...
               masterServerList->push_back(string(masterServerList->get(0)));  // don't remove string(...), or else this line is a mystery why push_back an empty string.
               masterServerList->erase(0);
            }

            const char *addr = masterServerList->get(0).c_str();

            mHaveTriedToConnectToMaster = true;
            logprintf(LogConsumer::LogConnection, "%s connecting to master [%s]", isServer() ? "Server" : "Client", addr);

            mNameToAddressThread = new NameToAddressThread(addr);
            mNameToAddressThread->start();
         }
         else
         {
            if(mNameToAddressThread->mDone)
            {
               if(mNameToAddressThread->mAddress.isValid())
               {
                  TNLAssert(!mConnectionToMaster.isValid(), "Already have connection to master!");
                  mConnectionToMaster = new MasterServerConnection(this);

                  mConnectionToMaster->connect(mNetInterface, mNameToAddressThread->mAddress);
               }
   
               mNextMasterTryTime = GameConnection::MASTER_SERVER_FAILURE_RETRY_TIME;     // 10 secs, just in case this attempt fails
               delete mNameToAddressThread;
               mNameToAddressThread = NULL;
            }
         }
      }
      else if(!mReadyToConnectToMaster)
         mNextMasterTryTime = 0;
      else
         mNextMasterTryTime -= timeDelta;
   }

   processAnonymousMasterConnection();
}


string Game::toLevelCode() const
{
   return mLevel->toLevelCode();
}


void Game::processAnonymousMasterConnection()
{
   // Connection doesn't exist yet
   if(!mAnonymousMasterServerConnection.isValid())
      return;

   // Connection has already been initiated
   if(mAnonymousMasterServerConnection->isInitiator())
      return;

   // Try to open a socket to master server
   if(!mNameToAddressThread)
   {
      Vector<string> *masterServerList = gSettings.getMasterServerList();

      // No master server addresses?
      if(masterServerList->size() == 0)
         return;

      const char *addr = masterServerList->get(0).c_str();

      mNameToAddressThread = new NameToAddressThread(addr);
      mNameToAddressThread->start();
   }
   else
   {
      if(mNameToAddressThread->mDone)
      {
         if(mNameToAddressThread->mAddress.isValid())
            mAnonymousMasterServerConnection->connect(mNetInterface, mNameToAddressThread->mAddress);

         delete mNameToAddressThread;
         mNameToAddressThread = NULL;
      }
   }
}


// Called by both ClientGame::idle and ServerGame::idle
void Game::idle(U32 timeDelta)
{
   mSecondaryThread->idle();
}


Game::DeleteRef::DeleteRef(BfObject *o, U32 d)
{
   theObject = o;
   delay = d;
}


void Game::addToDeleteList(BfObject *theObject, U32 delay)
{
   TNLAssert(!theObject->isGhost(), "Can't delete ghosting Object");
   mPendingDeleteObjects.push_back(DeleteRef(theObject, delay));
}


// Cycle through our pending delete list, and either delete an object or update its timer
void Game::processDeleteList(U32 timeDelta)
{
   for(S32 i = 0; i < mPendingDeleteObjects.size(); i++) 
      if(timeDelta > mPendingDeleteObjects[i].delay)
      {
         BfObject *g = mPendingDeleteObjects[i].theObject;
         delete g;
         mPendingDeleteObjects.erase_fast(i);
         i--;
      }
      else
         mPendingDeleteObjects[i].delay -= timeDelta;
}


// Delete all objects of specified type  --> currently only used to remove all walls from the game
void Game::deleteObjects(U8 typeNumber)
{
   fillVector.clear();
   mLevel->findObjects(typeNumber, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(fillVector[i]);
      obj->deleteObject(0);
   }
}


// Not currently used
void Game::deleteObjects(TestFunc testFunc)
{
   fillVector.clear();
   mLevel->findObjects(testFunc, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      BfObject *obj = static_cast<BfObject *>(fillVector[i]);
      obj->deleteObject(0);
   }
}


void Game::computeWorldObjectExtents()
{
   mWorldExtents = mLevel->getExtents();
}


Rect Game::computeBarrierExtents()
{
   Rect extents;

   fillVector.clear();
   mLevel->findObjects((TestFunc)isWallType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      extents.unionRect(fillVector[i]->getExtent());

   return extents;
}


Point Game::computePlayerVisArea(const Ship *ship) const
{
   F32 fraction = ship->getSensorZoomFraction();

   static const Point regVis(PLAYER_VISUAL_DISTANCE_HORIZONTAL, PLAYER_VISUAL_DISTANCE_VERTICAL);
   static const Point sensVis(PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL, PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL);

   if(ship->hasModule(ModuleSensor))
      return regVis + (sensVis - regVis) * fraction;
   else
      return sensVis + (regVis - sensVis) * fraction;
}


// Returns the render scale based on whether sensor is active.  If sensor is not active, scale will be 1.0.
// Currently this is used to upscale the name displayed with ships when they are rendered.
F32 Game::getRenderScale(bool sensorActive) const
{
   static const F32 sensorScale = (F32)PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL / 
                                  (F32)PLAYER_VISUAL_DISTANCE_HORIZONTAL;

   return sensorActive ? sensorScale : 1;
}


extern void exitToOs(S32 errcode);

// Make sure name is unique.  If it's not, make it so.  The problem is that then the client doesn't know their official name.
// This makes the assumption that we'll find a unique name before testing U32_MAX combinations.
string Game::makeUnique(const char *name)
{
   if(name[0] == 0)  // No zero-length name allowed
      name = "ChumpChange";

   U32 index = 0;
   string proposedName = name;

   bool unique = false;

   while(!unique)
   {
      unique = true;

      for(S32 i = 0; i < getClientCount(); i++)
      {
         if(proposedName == getClientInfo(i)->getName().getString())     // Collision detected!
         {
            unique = false;

            char numstr[U32_MAX_DIGITS + 2];    // + 1 for the ., +1 for the \0

            dSprintf(numstr, ARRAYSIZE(numstr), ".%d", index);

            // Max length name can be such that when number is appended, it's still less than MAX_PLAYER_NAME_LENGTH
            S32 maxNamePos = MAX_PLAYER_NAME_LENGTH - (S32)strlen(numstr); 
            proposedName = string(name).substr(0, maxNamePos) + numstr;     // Make sure name won't grow too long

            index++;

            if(index == U32_MAX)
            {
               logprintf(LogConsumer::LogError, "Too many players using the same name!  Aaaargh!");
               exitToOs(1);
            }
            break;
         }
      }
   }

   return proposedName;
}


// Called when ClientGame and ServerGame are destructed, and new levels are loaded on the server
void Game::cleanUp()
{
   // Delete any objects on the delete list
   processDeleteList(U32_MAX);

   // Manually remove the objects before wiping the level because some objects need mLevel to be set in their
   // destructors... CoreItems, for example
   if(mLevel)
   {
      mLevel->removeEverythingFromDatabase();
      mLevel.reset();    // mLevel is a shared_ptr, so cleanup will be handled automatically
   }
}


const Rect *Game::getWorldExtents() const
{
   return &mWorldExtents;
}


const Color &Game::getTeamColor(S32 teamId) const
{
   return mLevel->getTeamColor(teamId);
}


void Game::setPreviousLevelName(const string &name)
{
   // Do nothing (but will be overidded in ClientGame)
}


// Overridden on client
void Game::setLevelDatabaseId(U32 id)
{
   mLevel->setLevelDatabaseId(id);
}


U32 Game::getLevelDatabaseId() const
{
   return mLevel->getLevelDatabaseId();
}


// Server only
void Game::onFlagMounted(S32 teamIndex)
{
   getGameType()->onFlagMounted(teamIndex);
}


// Server only
void Game::itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode)
{
   getGameType()->itemDropped(ship, item, dismountMode);
}


const Color &Game::getObjTeamColor(const BfObject *obj) const
{ 
   return getGameType()->getTeamColor(obj);
}


bool Game::objectCanDamageObject(BfObject *damager, BfObject *victim) const
{
   return getGameType()->objectCanDamageObject(damager, victim);
}


void Game::releaseFlag(const Point &pos, const Point &vel, const S32 count) const
{
   getGameType()->releaseFlag(pos, vel, count);
}


S32 Game::getRenderTime() const
{
   return getGameType()->getRemainingGameTimeInMs() + getGameType()->getRenderingOffset();
}


Vector<AbstractSpawn *> Game::getSpawnPoints(TypeNumber typeNumber, S32 teamIndex)
{
   return getGameType()->getSpawnPoints(typeNumber, teamIndex);
}


void Game::addFlag(FlagItem *flag)
{
   getGameType()->addFlag(flag);
}


void Game::shipTouchFlag(Ship *ship, FlagItem *flag)
{
   getGameType()->shipTouchFlag(ship, flag);
}


void Game::shipTouchZone(Ship *ship, GoalZone *zone)
{
   getGameType()->shipTouchZone(ship, zone);
}


bool Game::isTeamGame() const
{
   return getGameType()->isTeamGame();
}


Timer &Game::getGlowZoneTimer()
{
   return getGameType()->mZoneGlowTimer;
}


S32 Game::getGlowingZoneTeam()
{
   return getGameType()->mGlowingZoneTeam;
}


string Game::getScriptName() const
{
   return getGameType()->getScriptName();
}


bool Game::levelHasLoadoutZone()
{
   return getGameType()->levelHasLoadoutZone();
}


void Game::updateShipLoadout(BfObject *shipObject)
{
   return getGameType()->updateShipLoadout(shipObject);
}


void Game::sendChat(const StringTableEntry &senderName, ClientInfo *senderClientInfo, const StringPtr &message, bool global, S32 teamIndex)
{
   getGameType()->sendChat(senderName, senderClientInfo, message, global, teamIndex);
}


void Game::sendPrivateChat(const StringTableEntry &senderName, const StringTableEntry &receiverName, const StringPtr &message)
{
   getGameType()->sendPrivateChat(senderName, receiverName, message);
}


void Game::sendAnnouncementFromController(const StringPtr &message)
{
   getGameType()->sendAnnouncementFromController(message);
}


void Game::updateClientChangedName(ClientInfo *clientInfo, const StringTableEntry &newName)
{
   getGameType()->updateClientChangedName(clientInfo, newName);
}


// Static method - only used for "illegal" activities
const Level *Game::getServerGameObjectDatabase()
{
   return GameManager::getServerGame()->getGameObjDatabase();
}


// This is not a very good way of seeding the prng, but it should generate unique, if not cryptographicly secure, streams.
// We'll get 4 bytes from the time, up to 12 bytes from the name, and any left over slots will be filled with unitialized junk.
// Static method
void Game::seedRandomNumberGenerator(const string &name)
{
   U32 time = Platform::getRealMilliseconds();
   const S32 timeByteCount = 4;
   const S32 totalByteCount = 16;

   S32 nameBytes = min((S32)name.length(), totalByteCount - timeByteCount);     // # of bytes we get from the provided name

   unsigned char buf[totalByteCount] = {0};

   // Bytes from the time
   buf[0] = U8(time);
   buf[1] = U8(time >> 8);
   buf[2] = U8(time >> 16);
   buf[3] = U8(time >> 24);

   // Bytes from the name
   for(S32 i = 0; i < nameBytes; i++)
      buf[i + timeByteCount] = name.at(i);

   Random::addEntropy(buf, totalByteCount);     // May be some uninitialized bytes at the end of the buffer, but that's ok
}


bool Game::objectCanDamageObject(BfObject *damager, BfObject *victim)
{
   if(!getGameType())
      return true;
   else
      return getGameType()->objectCanDamageObject(damager, victim);
}


U32 Game::getMaxPlayers() const 
{
   TNLAssert(false, "Not implemented for this class!");
   return 0;
}

   
void Game::gotPingResponse(const Address &address, const Nonce &nonce, U32 clientIdentityToken, S32 clientId)
{
   TNLAssert(false, "Not implemented for this class!");
}


void Game::gotQueryResponse(const Address &address, S32 serverId, 
                            const Nonce &nonce, const char *serverName, const char *serverDescr, 
                            U32 playerCount, U32 maxPlayers, U32 botCount, bool dedicated, bool test, bool passwordRequired)
{
   TNLAssert(false, "Not implemented for this class!");
}


void Game::displayMessage(const Color &msgColor, const char *format, ...) const
{
   TNLAssert(false, "Not implemented for this class!");
}


ClientInfo *Game::getLocalRemoteClientInfo() const
{
   TNLAssert(false, "Not implemented for this class!");
   return NULL;
}


void Game::quitEngineerHelper()
{
   TNLAssert(false, "Not implemented for this class!");
}


bool Game::isDedicated() const  
{
   return false;
}


Point Game::worldToScreenPoint(const Point *p, S32 canvasWidth, S32 canvasHeight) const
{
   TNLAssert(false, "Not implemented for this class!");
   return Point(0,0);
}


F32 Game::getCommanderZoomFraction() const
{
   TNLAssert(false, "Not implemented for this class!");
   return 0;
}


void Game::renderBasicInterfaceOverlay() const
{
   TNLAssert(false, "Not implemented for this class!");
}


void Game::emitTextEffect(const string &text, const Color &color, const Point &pos, bool relative) const
{
   TNLAssert(false, "Not implemented for this class!");
}


void Game::emitDelayedTextEffect(U32 delay, const string &text, const Color &color, const Point &pos, bool relative) const
{
   TNLAssert(false, "Not implemented for this class!");
}

string Game::getPlayerName() const
{
   TNLAssert(false, "Not implemented for this class!");
   return "";
}


void Game::addInlineHelpItem(HelpItem item) const
{
   TNLAssert(false, "Not implemented for this class!");
}


void Game::removeInlineHelpItem(HelpItem item, bool markAsSeen) const
{
   TNLAssert(false, "Not implemented for this class!");
}


F32 Game::getObjectiveArrowHighlightAlpha() const
{
   TNLAssert(false, "Not implemented for this class!");
   return 0;
}


// In seconds
S32 Game::getRemainingGameTime() const
{
   if(mLevel && getGameType())     // Can be NULL at the end of a game
      return getGameType()->getRemainingGameTime();
   else
      return 0;
}


Master::DatabaseAccessThread *Game::getSecondaryThread()
{
   return  mSecondaryThread;
}


};


