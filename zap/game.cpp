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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "barrier.h"
#include "config.h"
#include "EngineeredItem.h"      // For EngineerModuleDeployer
#include "game.h"
#include "gameLoader.h"
#include "gameNetInterface.h"
#include "gameObject.h"
#include "gameObjectRender.h"
#include "gameType.h"
#include "masterConnection.h"
#include "move.h"
#include "moveObject.h"
#include "projectile.h"          // For SpyBug class
#include "SoundSystem.h"
#include "SharedConstants.h"     // For ServerInfoFlags enum
#include "ship.h"
#include "GeomUtils.h"
#include "luaLevelGenerator.h"
#include "robot.h"
#include "shipItems.h"           // For moduleInfos
#include "stringUtils.h"
#include "NexusGame.h"           // For creating new NexusFlagItem
#include "teamInfo.h"            // For TeamManager def
#include "playerInfo.h"

#include "IniFile.h"             // For CIniFile def
#include "BanList.h"             // For banList kick duration

#include "BotNavMeshZone.h"      // For zone clearing code

#ifndef ZAP_DEDICATED
#include "voiceCodec.h"
#endif

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

#include "md5wrapper.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>


#include "soccerGame.h"


#ifdef PRINT_SOMETHING
#include "ClientGame.h"  // only used to print some variables in ClientGame...
#endif


using namespace TNL;

namespace Zap
{

// Constructor
ClientInfo::ClientInfo()
{
   mPlayerInfo = NULL;

   mScore = 0;
   mTotalScore = 0;
   mTeamIndex = (NO_TEAM + 0);
   mPing = 0;
   mIsAdmin = false;
   mIsLevelChanger = false;
   mIsRobot = false;
   mIsAuthenticated = false;
   mBadges = NO_BADGES;
   mNeedToCheckAuthenticationWithMaster = false;     // Does client report that they are verified
}


// Destructor
ClientInfo::~ClientInfo()
{
   delete mPlayerInfo;
}


void ClientInfo::setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges)
{
   mNeedToCheckAuthenticationWithMaster = false;     // Once we get here, we'll treat the ruling as definitive
   mIsAuthenticated = isAuthenticated; 
   mBadges = badges;
}


Int<BADGE_COUNT> ClientInfo::getBadges()
{
   return mBadges;
}


const StringTableEntry ClientInfo::getName()
{
   return mName;
}


void ClientInfo::setName(const StringTableEntry &name)
{
   mName = name;
}


S32 ClientInfo::getScore()
{
   return mScore;
}


void ClientInfo::setScore(S32 score)
{
   mScore = score;
}


void ClientInfo::addScore(S32 score)
{
   mScore += score;
}


void ClientInfo::setShip(Ship *ship)
{
   mShip = ship;
}


Ship *ClientInfo::getShip()
{
   return mShip;
}


void ClientInfo::setNeedToCheckAuthenticationWithMaster(bool needToCheck)
{
   mNeedToCheckAuthenticationWithMaster = needToCheck;
}


bool ClientInfo::getNeedToCheckAuthenticationWithMaster()
{
   return mNeedToCheckAuthenticationWithMaster;
}


// Check if player is "on hold" due to inactivity; bots are never on hold.  Server only!
bool ClientInfo::isSpawnDelayed()
{
   return mIsRobot ? false : getConnection()->getTimeSinceLastMove() > 20000;    // 20 secs
}


void ClientInfo::resetLoadout()
{
   mLoadout.clear();

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      mLoadout.push_back(DefaultLoadout[i]);
}


const Vector<U32> &ClientInfo::getLoadout()
{
   return mLoadout;
}


S32 ClientInfo::getPing()
{
   return mPing;
}


void ClientInfo::setPing(S32 ping)
{
   mPing = ping;
}


S32 ClientInfo::getTeamIndex()
{
   return mTeamIndex;
}


void ClientInfo::setTeamIndex(S32 teamIndex)
{
   mTeamIndex = teamIndex;
}


bool ClientInfo::isAuthenticated()
{
   return mIsAuthenticated;
}


bool ClientInfo::isLevelChanger()
{
   return mIsLevelChanger;
}


void ClientInfo::setIsLevelChanger(bool isLevelChanger)
{
   mIsLevelChanger = isLevelChanger;
}


bool ClientInfo::isAdmin()
{
   return mIsAdmin;
}


void ClientInfo::setIsAdmin(bool isAdmin)
{
   mIsAdmin = isAdmin;
}


bool ClientInfo::isRobot()
{
   return mIsRobot;
}


F32 ClientInfo::getCalculatedRating()
{
   return mStatistics.getCalculatedRating();
}


// Resets stats and the like
void ClientInfo::endOfGameScoringHandler()
{
   mStatistics.addGamePlayed();
   mStatistics.resetStatistics();
}


LuaPlayerInfo *ClientInfo::getPlayerInfo()
{
   // Lazily initialize
   if(!mPlayerInfo)
      mPlayerInfo = new PlayerInfo(this);   // Deleted in destructor

   return mPlayerInfo;
}

// Server only, robots can run this, bypassing the net interface. Return true if successfuly deployed. // TODO: Move this elsewhere
bool ClientInfo::sEngineerDeployObject(U32 type)
{
   Ship *ship = dynamic_cast<Ship *>(getShip());
   if(!ship)                                          // Not a good sign...
      return false;                                   // ...bail

   GameType *gameType = ship->getGame()->getGameType();

   if(!gameType->isEngineerEnabled())          // Something fishy going on here...
      return false;                                   // ...bail

   EngineerModuleDeployer deployer;

   if(!deployer.canCreateObjectAtLocation(ship->getGame()->getGameObjDatabase(), ship, type)) 
   {
      if(!isRobot())
         getConnection()->s2cDisplayErrorMessage(deployer.getErrorMessage().c_str());
   }
   else if(deployer.deployEngineeredItem(ship->getClientInfo(), type))
   {
      // Announce the build
      StringTableEntry msg( "%e0 has engineered a %e1." );

      Vector<StringTableEntry> e;
      e.push_back(getName());
      e.push_back(type == EngineeredTurret ? "turret" : "force field");
   

      gameType->broadcastMessage(GameConnection::ColorAqua, SFXNone, msg, e);

      return true;
   }

   // else... fail silently?
   return false;
}


void ClientInfo::sRequestLoadout(Vector<U32> &loadout)
{
   mLoadout = loadout;
   GameType *gt = gServerGame->getGameType();
   if(gt)
      gt->SRV_clientRequestLoadout(this, mLoadout);    // This will set loadout if ship is in loadout zone

   // Check if ship is in a loadout zone, in which case we'll make the loadout take effect immediately
   //Ship *ship = dynamic_cast<Ship *>(this->getControlObject());

   //if(ship && ship->isInZone(LoadoutZoneType))
      //ship->setLoadout(loadout);
}



// Return pointer to statistics tracker 
Statistics *ClientInfo::getStatistics()
{
   return &mStatistics;
}


Nonce *ClientInfo::getId()
{
   return &mId;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
FullClientInfo::FullClientInfo(GameConnection *gameConnection, bool isRobot) : ClientInfo()
{
   mClientConnection = gameConnection;
   mIsRobot = isRobot;
}


// Destructor
FullClientInfo::~FullClientInfo()
{
   // Do nothing
}


void FullClientInfo::setAuthenticated(bool isAuthenticated, Int<BADGE_COUNT> badges)
{
   TNLAssert(isAuthenticated || badges == NO_BADGES, "Unauthenticated players should never have badges!");
   Parent::setAuthenticated(isAuthenticated, badges);

   // Broadcast new connection status to all clients.  It does seem a little roundabout to use the game server to communicate
   // between the FullClientInfo and the RemoteClientInfo on each client; we could contact the RemoteClientInfo directly and
   // then not send a message to the client being authenticated below.  But I think this is cleaner architecturally, and this
   // message is not sent often.
   if(mClientConnection && mClientConnection->isConnectionToClient())      
      for(S32 i = 0; i < gServerGame->getClientCount(); i++)
         gServerGame->getClientInfo(i)->getConnection()->s2cSetAuthenticated(mName, isAuthenticated, badges);
}


F32 FullClientInfo::getRating()
{
   // Initial case: no one has scored
   if(mTotalScore == 0)      
      return .5;

   // Standard case: 
   else   
      return F32(mScore) / F32(mTotalScore);
}


GameConnection *FullClientInfo::getConnection()
{
   return mClientConnection;
}


void FullClientInfo::setConnection(GameConnection *conn)
{
   mClientConnection = conn;
}


void FullClientInfo::setRating(F32 rating)
{
   TNLAssert(false, "Ratings can't be set for this class!");
}


SoundEffect *FullClientInfo::getVoiceSFX()
{
   TNLAssert(false, "Can't access VoiceSFX from this class!");
   return NULL;
}


VoiceDecoder *FullClientInfo::getVoiceDecoder()
{
   TNLAssert(false, "Can't access VoiceDecoder from this class!");
   return NULL;
}


////////////////////////////////////////
////////////////////////////////////////


#ifndef ZAP_DEDICATED
// Constructor
RemoteClientInfo::RemoteClientInfo(const StringTableEntry &name, bool isAuthenticated, Int<BADGE_COUNT> badges, 
                                   bool isRobot, bool isAdmin) : ClientInfo()
{
   mName = name;
   mIsAuthenticated = isAuthenticated;
   mIsRobot = isRobot;
   mIsAdmin = isAdmin;
   mTeamIndex = NO_TEAM;
   mRating = 0;
   mBadges = badges;

   // Initialize speech stuff, will be deleted in destructor
   mDecoder = new SpeexVoiceDecoder();
   mVoiceSFX = new SoundEffect(SFXVoice, NULL, 1, Point(), Point());
}


RemoteClientInfo::~RemoteClientInfo()
{
   delete mDecoder;
}


GameConnection *RemoteClientInfo::getConnection()
{
   TNLAssert(false, "Can't get a GameConnection from a RemoteClientInfo!");
   return NULL;
}


void RemoteClientInfo::setConnection(GameConnection *conn)
{
   TNLAssert(false, "Can't set a GameConnection on a RemoteClientInfo!");
}


F32 RemoteClientInfo::getRating()
{
   return mRating;
}


void RemoteClientInfo::setRating(F32 rating)
{
   mRating = rating;
}


// Voice chat stuff -- these will be invalid on the server side
SoundEffect *RemoteClientInfo::getVoiceSFX()
{
   return mVoiceSFX;
}


VoiceDecoder *RemoteClientInfo::getVoiceDecoder()
{
   return mDecoder;
}

#endif


////////////////////////////////////////
////////////////////////////////////////

// Constructor
NameToAddressThread::NameToAddressThread(const char *address_string) :
      mAddress_string(address_string)
{
   mDone = false;
}

//Destructor
NameToAddressThread::~NameToAddressThread()
{
   // Do nothing
}


U32 NameToAddressThread::run()
{
   // this can take a lot of time converting name (such as "bitfighter.org:25955") into IP address.
   mAddress.set(mAddress_string);
   mDone = true;
   return 0;
}


////////////////////////////////////////
////////////////////////////////////////

#ifndef ZAP_DEDICATED
class ClientGame;
extern ClientGame *gClientGame; // only used to see if we have a ClientGame, for buildBotMeshZones
#endif

// Global Game objects
ServerGame *gServerGame = NULL;

static Vector<DatabaseObject *> fillVector2;


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// Constructor
Game::Game(const Address &theBindAddress, GameSettings *settings) : mGameObjDatabase(new GridDatabase())      //? was without new
{
   mSettings = settings;

   mNextMasterTryTime = 0;
   mReadyToConnectToMaster = false;

   mCurrentTime = 0;
   mGameSuspended = false;

   mRobotCount = 0;
   mPlayerCount = 0;

   mTimeUnconnectedToMaster = 0;

   mNetInterface = new GameNetInterface(theBindAddress, this);
   mHaveTriedToConnectToMaster = false;

   mWallSegmentManager = new WallSegmentManager();    // gets deleted in destructor

   mNameToAddressThread = NULL;

   mTeamManager = new TeamManager;                    // gets deleted in destructor 
   mActiveTeamManager = mTeamManager;
}


// Destructor
Game::~Game()
{
   delete mWallSegmentManager;
   delete mTeamManager;

   if(mNameToAddressThread)
      delete mNameToAddressThread;
}


F32 Game::getGridSize() const
{
   return mGridSize;
}


void Game::setGridSize(F32 gridSize) 
{ 
   mGridSize = max(min(gridSize, F32(MAX_GRID_SIZE)), F32(MIN_GRID_SIZE));
}


U32 Game::getCurrentTime()
{
   return mCurrentTime;
}


const Vector<SafePtr<GameObject> > &Game::getScopeAlwaysList()
{
   return mScopeAlwaysList;
}


void Game::setScopeAlwaysObject(GameObject *theObject)
{
   mScopeAlwaysList.push_back(theObject);
}


GameSettings *Game::getSettings()
{
   return mSettings;
}


bool Game::isSuspended()
{
   return mGameSuspended;
}


void Game::resetMasterConnectTimer()
{
   mNextMasterTryTime = 0;
}


void Game::setReadyToConnectToMaster(bool ready)
{
   mReadyToConnectToMaster = ready;
}


Point Game::getScopeRange(bool sensorIsActive)
{
   return
         sensorIsActive ?
               Point(PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL
                     + PLAYER_SCOPE_MARGIN, PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL
                     + PLAYER_SCOPE_MARGIN) :
               Point(PLAYER_VISUAL_DISTANCE_HORIZONTAL + PLAYER_SCOPE_MARGIN, PLAYER_VISUAL_DISTANCE_VERTICAL
                     + PLAYER_SCOPE_MARGIN);
}


S32 Game::getClientCount() const
{
   return mClientInfos.size();
}


S32 Game::getPlayerCount() const
{
   return mPlayerCount;
}


S32 Game::getRobotCount() const
{
   return mRobotCount;
}


ClientInfo *Game::getClientInfo(S32 index) 
{ 
   return mClientInfos[index]; 
}


const Vector<ClientInfo *> *Game::getClientInfos()
{
   return &mClientInfos;
}


// ClientInfo will be a RemoteClientInfo in ClientGame and a FullClientInfo in ServerGame
void Game::addToClientList(ClientInfo *clientInfo) 
{ 
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
      if(mClientInfos[i]->getName() == name) 
         return i;

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
   mClientInfos.clear(); 

   mRobotCount = 0;
   mPlayerCount = 0;
}


// Find clientInfo given a player name
ClientInfo *Game::findClientInfo(const StringTableEntry &name)
{
   S32 index = findClientIndex(name);

   return index >= 0 ? mClientInfos[index] : NULL;
}


GameNetInterface *Game::getNetInterface()
{
   return mNetInterface;
}


GridDatabase *Game::getGameObjDatabase()
{
   return mGameObjDatabase.get();
}


EditorObjectDatabase *Game::getEditorDatabase()  // TODO: Only for clientGame
{ 
   // Lazy init
   if(!mEditorDatabase.get())
      mEditorDatabase = boost::shared_ptr<EditorObjectDatabase>(new EditorObjectDatabase());
         
   return mEditorDatabase.get(); 
}  


void Game::setEditorDatabase(boost::shared_ptr<GridDatabase> database)
{
   mEditorDatabase =
         boost::dynamic_pointer_cast<EditorObjectDatabase>(database);
}


MasterServerConnection *Game::getConnectionToMaster()
{
   return mConnectionToMaster;
}


GameType *Game::getGameType() const
{
   return mGameType;    // This is a safePtr, so it can be NULL, but will never point off into space
}


S32 Game::getTeamCount() const
{
   return mActiveTeamManager->getTeamCount();
}


AbstractTeam *Game::getTeam(S32 team) const
{
   return mActiveTeamManager->getTeam(team);
}


// There is a bigger need to use StringTableEntry and not const char *
//    mainly to prevent errors on CTF neutral flag and out of range team number.
StringTableEntry Game::getTeamName(S32 teamIndex) const
{
   if(teamIndex >= 0 && teamIndex < getTeamCount())
      return getTeam(teamIndex)->getName();
   else if(teamIndex == TEAM_HOSTILE)
      return StringTableEntry("Hostile");
   else if(teamIndex == TEAM_NEUTRAL)
      return StringTableEntry("Neutral");
   else
      return StringTableEntry("UNKNOWN");
}


// Given a player's name, return his team
S32 Game::getTeamIndex(const StringTableEntry &playerName)
{
   ClientInfo *clientInfo = findClientInfo(playerName);              // Returns NULL if player can't be found
   
   return clientInfo ? clientInfo->getTeamIndex() : TEAM_NEUTRAL;    // If we can't find the team, let's call it neutral
}


// The following just delegate their work to the TeamManager
void Game::removeTeam(S32 teamIndex) { mActiveTeamManager->removeTeam(teamIndex); }

void Game::addTeam(AbstractTeam *team) { mActiveTeamManager->addTeam(team); }

void Game::addTeam(AbstractTeam *team, S32 index) { mActiveTeamManager->addTeam(team, index); }

void Game::replaceTeam(AbstractTeam *team, S32 index) { mActiveTeamManager->replaceTeam(team, index); }

void Game::clearTeams() { mActiveTeamManager->clearTeams(); }


// Makes sure that the mTeams[] structure has the proper player counts
// Needs to be called manually before accessing the structure
// Rating may only work on server... not tested on client
void Game::countTeamPlayers()
{
   for(S32 i = 0; i < getTeamCount(); i++)
   {
      TNLAssert(dynamic_cast<Team *>(getTeam(i)), "Invalid team");      // Assert for safety
      static_cast<Team *>(getTeam(i))->clearStats();                    // static_cast for speed
   }

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo =  getClientInfo(i);

      S32 teamIndex = clientInfo->getTeamIndex();

      if(teamIndex >= 0 && teamIndex < getTeamCount())
      { 
         // Robot could be neutral or hostile, skip out of range team numbers
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


void Game::setGameType(GameType *theGameType)      // TODO==> Need to store gameType as a shared_ptr or auto_ptr
{
   //delete mGameType;          // Cleanup, if need be
   mGameType = theGameType;
}


U32 Game::getTimeUnconnectedToMaster()
{
   return mTimeUnconnectedToMaster;
}


void Game::resetLevelInfo()
{
   setGridSize((F32)DefaultGridSize);
}


// Process a single line of a level file, loaded in gameLoader.cpp
// argc is the number of parameters on the line, argv is the params themselves
// Used by ServerGame and the editor
void Game::processLevelLoadLine(U32 argc, U32 id, const char **argv, GridDatabase *database, bool inEditor, const string &levelFileName)
{
   S32 strlenCmd = (S32) strlen(argv[0]);

   // This is a legacy from the old Zap! days... we do bots differently in Bitfighter, so we'll just ignore this line if we find it.
   if(!stricmp(argv[0], "BotsPerTeam"))
      return;

   else if(!stricmp(argv[0], "GridSize"))      // GridSize requires a single parameter (an int specifiying how many pixels in a grid cell)
   {                                           
      if(argc < 2)
         logprintf(LogConsumer::LogLevelError, "Improperly formed GridSize parameter");
      else
         setGridSize((F32)atof(argv[1]));

      return;
   }

   // Parse GameType line... All game types are of form XXXXGameType
   else if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
   {
      // validateGameType() will return a valid GameType string -- either what's passed in, or the default if something bogus was specified
      TNL::Object *theObject;
      if(!strcmp(argv[0], "HuntersGameType"))
         theObject = new NexusGameType();
      else
         theObject = TNL::Object::create(GameType::validateGameType(argv[0]));

      GameType *gt = dynamic_cast<GameType *>(theObject);  // Force our new object to be a GameObject

      if(gt)
      {
         bool validArgs = gt->processArguments(argc - 1, argv + 1, NULL);
         if(!validArgs)
            logprintf(LogConsumer::LogLevelError, "GameType have incorrect parameters");

         gt->addToGame(this, database);    
      }
      else
         logprintf(LogConsumer::LogLevelError, "Could not create a GameType");
      
      return;
   }

   if(getGameType() && processLevelParam(argc, argv)) 
   {
      // Do nothing here
   }
   else if(getGameType() && processPseudoItem(argc, argv, levelFileName, database))
   {
      // Do nothing here
   }
   
   else
   {
      char obj[LevelLoader::MaxArgLen + 1];

      // Convert any NexusFlagItem into FlagItem, only NexusFlagItem will show up on ship
      if(!stricmp(argv[0], "HuntersFlagItem") || !stricmp(argv[0], "NexusFlagItem"))
         strcpy(obj, "FlagItem");

      // Convert legacy Hunters* objects
      else if(!stricmp(argv[0], "HuntersNexusObject"))
         strcpy(obj, "NexusObject");

      else
         strncpy(obj, argv[0], LevelLoader::MaxArgLen);

      obj[LevelLoader::MaxArgLen] = '\0';
      TNL::Object *theObject = TNL::Object::create(obj);          // Create an object of the type specified on the line

      SafePtr<GameObject> object  = dynamic_cast<GameObject *>(theObject);  // Force our new object to be a GameObject
      EditorObject *eObject = dynamic_cast<EditorObject *>(theObject);


      if(!object && !eObject)    // Well... that was a bad idea!
      {
         logprintf(LogConsumer::LogLevelError, "Unknown object type \"%s\" in level \"%s\"", obj, levelFileName.c_str());
         delete theObject;
      }
      else  // object was valid
      {
         computeWorldObjectExtents();    // Make sure this is current if we process a robot that needs this for intro code

         bool validArgs = object->processArguments(argc - 1, argv + 1, this);

         // Mark the item as being a ghost (client copy of a server object) so that the object will not trigger server-side tests
         // The only time this code is run on the client is when loading into the editor.
         if(inEditor)
            object->markAsGhost();

         if(validArgs)
         {
            if(object.isValid())  // processArguments might delete this object (teleporter)
               object->addToGame(this, database);
         }
         else
         {
            logprintf(LogConsumer::LogLevelError, "Invalid arguments in object \"%s\" in level \"%s\"", obj, levelFileName.c_str());
            object->destroySelf();
         }
      }
   }
}


// Returns true if we've handled the line (even if it handling it means that the line was bogus); returns false if
// caller needs to create an object based on the line
bool Game::processLevelParam(S32 argc, const char **argv)
{
   if(!stricmp(argv[0], "Team"))
      onReadTeamParam(argc, argv);

   // TODO: Create better way to change team details from level scripts: https://code.google.com/p/bitfighter/issues/detail?id=106
   else if(!stricmp(argv[0], "TeamChange"))   // For level script. Could be removed when there is a better way to change team names and colors.
      onReadTeamChangeParam(argc, argv);

   else if(!stricmp(argv[0], "Specials"))
      onReadSpecialsParam(argc, argv);

   else if(!strcmp(argv[0], "Script"))
      onReadScriptParam(argc, argv);

   else if(!stricmp(argv[0], "LevelName"))
      onReadLevelNameParam(argc, argv);
   
   else if(!stricmp(argv[0], "LevelDescription"))
      onReadLevelDescriptionParam(argc, argv);

   else if(!stricmp(argv[0], "LevelCredits"))
      onReadLevelCreditsParam(argc, argv);
   else if(!stricmp(argv[0], "MinPlayers"))     // Recommend a min number of players for this map
   {
      if(argc > 1)
         getGameType()->setMinRecPlayers(atoi(argv[1]));
   }
   else if(!stricmp(argv[0], "MaxPlayers"))     // Recommend a max number of players for this map
   {
      if(argc > 1)
         getGameType()->setMaxRecPlayers(atoi(argv[1]));
   }
   else
      return false;     // Line not processed; perhaps the caller can handle it?

   return true;         // Line processed; caller can ignore it
}


// Write out the game processed above; returns multiline string
string Game::toString()
{
   string str;

   GameType *gameType = getGameType();

   str = gameType->toString() + "\n";

   str += string("LevelName ") + writeLevelString(gameType->getLevelName()->getString()) + "\n";
   str += string("LevelDescription ") + writeLevelString(gameType->getLevelDescription()->getString()) + "\n";
   str += string("LevelCredits ") + writeLevelString(gameType->getLevelCredits()->getString()) + "\n";

   str += string("GridSize ") + ftos(mGridSize) + "\n";

   for(S32 i = 0; i < mActiveTeamManager->getTeamCount(); i++)
      str += mActiveTeamManager->getTeam(i)->toString() + "\n";

   str += gameType->getSpecialsLine() + "\n";

   if(gameType->getScriptName() != "")
      str += "Script " + gameType->getScriptLine() + "\n";

   str += string("MinPlayers") + (gameType->getMinRecPlayers() > 0 ? " " + itos(gameType->getMinRecPlayers()) : "") + "\n";
   str += string("MaxPlayers") + (gameType->getMaxRecPlayers() > 0 ? " " + itos(gameType->getMaxRecPlayers()) : "") + "\n";

   return str;
}


// Only occurs in scripts; could be in editor or on server
void Game::onReadTeamChangeParam(S32 argc, const char **argv)
{
   if(argc >= 2)   // Enough arguments?
   {
      S32 teamNumber = atoi(argv[1]);   // Team number to change

      if(teamNumber >= 0 && teamNumber < getTeamCount())
      {
         AbstractTeam *team = getNewTeam();
         team->processArguments(argc-1, argv+1);          // skip one arg
         replaceTeam(team, teamNumber);
      }
   }
}


void Game::onReadSpecialsParam(S32 argc, const char **argv)
{         
   for(S32 i = 1; i < argc; i++)
      if(!getGameType()->processSpecialsParam(argv[i]))
         logprintf(LogConsumer::LogLevelError, "Invalid specials parameter: %s", argv[i]);
}


void Game::onReadScriptParam(S32 argc, const char **argv)
{
   Vector<string> args;

   // argv[0] is always "Script"
   for(S32 i = 1; i < argc; i++)
      args.push_back(argv[i]);

   getGameType()->setScript(args);
}


static string getString(S32 argc, const char **argv)
{
   string s;
   for(S32 i = 1; i < argc; i++)
   {
      s += argv[i];
      if(i < argc - 1)
         s += " ";
   }

   return s;
}


void Game::onReadLevelNameParam(S32 argc, const char **argv)
{
   string s = getString(argc, argv);
   getGameType()->setLevelName(s.substr(0, MAX_GAME_NAME_LEN).c_str());
}


void Game::onReadLevelDescriptionParam(S32 argc, const char **argv)
{
   string s = getString(argc, argv);
   getGameType()->setLevelDescription(s.substr(0, MAX_GAME_DESCR_LEN).c_str());
}


void Game::onReadLevelCreditsParam(S32 argc, const char **argv)
{
   string s = getString(argc, argv);
   getGameType()->setLevelCredits(s.substr(0, MAX_GAME_DESCR_LEN).c_str());
}


// Only used during level load process...  actually, used at all?  If so, should be combined with similar code in gameType
// Not used during normal game load... perhaps by lua loader?
void Game::setGameTime(F32 time)
{
   GameType *gt = getGameType();

   TNLAssert(gt, "Null gametype!");

   if(gt)
      gt->setGameTime(time * 60);
}


extern OGLCONSOLE_Console gConsole;

bool Game::runLevelGenScript(const FolderManager *folderManager, const string &scriptName, const Vector<string> &scriptArgs, 
                                   GridDatabase *targetDatabase)
{
   string fullname = folderManager->findLevelGenScript(scriptName);  // Find full name of levelgen script

   if(fullname == "")
   {
      Vector<StringTableEntry> e;
      e.push_back(scriptName.c_str());
      getGameType()->broadcastMessage(GameConnection::ColorRed, SFXNone, "!!! Error running levelgen %e0: Could not find file", e);
      logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\"", scriptName.c_str());
      return false;
   }

   // The script file will be the first argument, subsequent args will be passed on to the script
   LuaLevelGenerator levelgen = LuaLevelGenerator(fullname, folderManager->luaDir, scriptArgs, getGridSize(), 
                                                  targetDatabase, this);
   if(!levelgen.runScript())
   {
      Vector<StringTableEntry> e;
      e.push_back(scriptName.c_str());

      getGameType()->broadcastMessage(GameConnection::ColorRed, SFXNone, "!!! Error running levelgen %e0: See server log for details", e);
      return false;
   }

   return true;
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
      Vector<string> *masterServerList = mSettings->getMasterServerList();

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
                  TNLAssert(!mConnectionToMaster, "Already have connection to master!");
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
}

Game::DeleteRef::DeleteRef(GameObject *o, U32 d)
{
   theObject = o;
   delay = d;
}


void Game::addToDeleteList(GameObject *theObject, U32 delay)
{
   mPendingDeleteObjects.push_back(DeleteRef(theObject, delay));
}


// Cycle through our pending delete list, and either delete an object or update its timer
void Game::processDeleteList(U32 timeDelta)
{
   for(S32 i = 0; i < mPendingDeleteObjects.size(); )    // no i++
   {     // braces required
      if(timeDelta > mPendingDeleteObjects[i].delay)
      {
         GameObject *g = mPendingDeleteObjects[i].theObject;
         delete g;
         mPendingDeleteObjects.erase_fast(i);
      }
      else
      {
         mPendingDeleteObjects[i].delay -= timeDelta;
         i++;
      }
   }
}


// Delete all objects of specified type  --> currently only used to remove all walls from the game
void Game::deleteObjects(U8 typeNumber)
{
   fillVector.clear();
   mGameObjDatabase->findObjects(typeNumber, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *obj = dynamic_cast<GameObject *>(fillVector[i]);
      obj->deleteObject(0);
   }
}


void Game::deleteObjects(TestFunc testFunc)
{
   fillVector.clear();
   mGameObjDatabase->findObjects(testFunc, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *obj = dynamic_cast<GameObject *>(fillVector[i]);
      obj->deleteObject(0);
   }
}


const ModuleInfo *Game::getModuleInfo(ShipModule module)
{
   TNLAssert(U32(module) < U32(ModuleCount), "out of range module");
   return &gModuleInfo[(U32) module];
}


WallSegmentManager *Game::getWallSegmentManager() const
{
   return mWallSegmentManager;
}


void Game::computeWorldObjectExtents()
{
   fillVector.clear();
   mGameObjDatabase->findObjects(fillVector);

   if(fillVector.size() == 0)     // No objects ==> no extents!
   {
      mWorldExtents = Rect();
      return;
   }

   // All this rigamarole is to make world extent correct for levels that do not overlap (0,0)
   // The problem is that the GameType is treated as an object, and has the extent (0,0), and
   // a mask of UnknownType.  Fortunately, the GameType tends to be first, so what we do is skip
   // all objects until we find an UnknownType object, then start creating our extent from there.
   // We have to assign theRect to an extent object initially to avoid getting the default coords
   // of (0,0) that are assigned by the constructor.
   Rect theRect;

   S32 first = -1;

   // Look for first non-UnknownType object
   for(S32 i = 0; i < fillVector.size() && first == -1; i++)
      if(fillVector[i]->getObjectTypeNumber() != UnknownTypeNumber)
      {
         theRect = fillVector[i]->getExtent();
         first = i;
      }

   if(first == -1)      // No suitable objects found, return empty extents
   {
      mWorldExtents = Rect();
      return;
   }

   // Now start unioning the extents of remaining objects.  Should be all of them.
   for(S32 i = first + 1; i < fillVector.size(); i++)
      theRect.unionRect(fillVector[i]->getExtent());

   mWorldExtents = theRect;
}


Rect Game::computeBarrierExtents()
{
   Rect theRect;

   fillVector.clear();
   mGameObjDatabase->findObjects((TestFunc)isWallType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      theRect.unionRect(fillVector[i]->getExtent());

   return theRect;
}


Point Game::computePlayerVisArea(Ship *ship) const
{
   F32 fraction = ship->getSensorZoomFraction();

   Point regVis(PLAYER_VISUAL_DISTANCE_HORIZONTAL, PLAYER_VISUAL_DISTANCE_VERTICAL);
   Point sensVis(PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL, PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL);

   if(ship->hasModule(ModuleSensor))
      return regVis + (sensVis - regVis) * fraction;
   else
      return sensVis + (regVis - sensVis) * fraction;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LevelInfo::LevelInfo()      
{
   initialize();
}


// Constructor, only used on client
LevelInfo::LevelInfo(const StringTableEntry &name, GameTypes type)
{
   initialize();

   levelName = name;  
   levelType = type; 
}


// Constructor
LevelInfo::LevelInfo(const string &levelFile)
{
   initialize();

   levelFileName = levelFile.c_str();
}


void LevelInfo::initialize()
{
   levelName = "";
   levelType = BitmatchGame;
   levelFileName = "";
   minRecPlayers = 0;
   maxRecPlayers = 0;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ServerGame::ServerGame(const Address &address, GameSettings *settings, bool testMode, bool dedicated) : 
      Game(address, settings)
{
   mVoteTimer = 0;
   mNextLevel = NEXT_LEVEL;

   mShuttingDown = false;

   EventManager::get()->setPaused(false);

   hostingModePhase = ServerGame::NotHosting;

   mInfoFlags = 0;                  // Currently only used to specify test mode
   mCurrentLevelIndex = 0;

   if(testMode)
      mInfoFlags = TestModeFlag;

   mTestMode = testMode;

   mNetInterface->setAllowsConnections(true);
   mMasterUpdateTimer.reset(UpdateServerStatusTime);

   mSuspendor = NULL;

#ifdef ZAP_DEDICATED
   TNLAssert(dedicated, "Dedicated should be true here!");
#endif

   mDedicated = dedicated;

   mGameSuspended = true; // server starts at zero players

   U32 stutter = mSettings->getSimulatedStutter();

   mStutterTimer.reset(1001 - stutter);    // Use 1001 to ensure timer is never set to 0
   mStutterSleepTimer.reset(stutter);
   mAccumulatedSleepTime = 0;
}


// Destructor
ServerGame::~ServerGame()
{
   cleanUp();
}


// Called when ClientGame and ServerGame are destructed, and new levels are loaded on the server
void Game::cleanUp()
{
   // Delete any objects on the delete list
   processDeleteList(0xFFFFFFFF);

   // Delete any game objects that may exist  --> not sure this will be needed when we're using shared_ptr
   // sam: should be deleted to properly get removed from server's database and to remove client's net objects.
   fillVector.clear();
   mGameObjDatabase->findObjects(fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      mGameObjDatabase->removeFromDatabase(fillVector[i], fillVector[i]->getExtent());
      delete dynamic_cast<Object *>(fillVector[i]); // dynamic_cast might be needed to avoid errors
   }

   mActiveTeamManager->clearTeams();
}


void ServerGame::cleanUp()
{
   fillVector.clear();
   mDatabaseForBotZones.findObjects(fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      delete dynamic_cast<Object *>(fillVector[i]);

   Parent::cleanUp();
}


// Return true when handled
bool ServerGame::voteStart(ClientInfo *clientInfo, S32 type, S32 number)
{
   GameConnection *conn = clientInfo->getConnection();

   if(!mSettings->getIniSettings()->voteEnable)
      return false;

   U32 VoteTimer;
   if(type == 4) // change team
      VoteTimer = mSettings->getIniSettings()->voteLengthToChangeTeam * 1000;
   else
      VoteTimer = mSettings->getIniSettings()->voteLength * 1000;
   if(VoteTimer == 0)
      return false;

   if(mVoteTimer != 0)
   {
      conn->s2cDisplayErrorMessage("Can't start a new vote when there is one pending.");
      return true;
   }

   if(conn->mVoteTime != 0)
   {
      Vector<StringTableEntry> e;
      Vector<StringPtr> s;
      Vector<S32> i;

      i.push_back(conn->mVoteTime / 1000);
      conn->s2cDisplayMessageESI(GameConnection::ColorRed, SFXNone, "Can't start vote, try again %i0 seconds later.", e, s, i);

      return true;
   }

   mVoteTimer = VoteTimer;
   mVoteType = type;
   mVoteNumber = number;
   mVoteClientName = clientInfo->getName();

   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->getConnection()->mVote = 0;

   conn->mVote = 1;
   conn->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, "Vote started, waiting for others to vote.");
   return true;
}


void ServerGame::voteClient(ClientInfo *clientInfo, bool voteYes)
{
   GameConnection *conn = clientInfo->getConnection();

   if(mVoteTimer == 0)
      conn->s2cDisplayErrorMessage("!!! Nothing to vote on");
   else if(conn->mVote == (voteYes ? 1 : 2))
      conn->s2cDisplayErrorMessage("!!! Already voted");
   else if(conn->mVote == 0)
      conn->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, voteYes ? "Voted Yes" : "Voted No");
   else
      conn->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, voteYes ? "Changed vote to Yes" : "Changed vote to No");

   conn->mVote = voteYes ? 1 : 2;
}


S32 ServerGame::getLevelNameCount()
{
   return mLevelInfos.size();
}


S32 ServerGame::getCurrentLevelIndex()
{
   return mCurrentLevelIndex;
}


S32 ServerGame::getLevelCount()
{
   return mLevelInfos.size();
}


LevelInfo ServerGame::getLevelInfo(S32 index)
{
   return mLevelInfos[index];
}


void ServerGame::clearLevelInfos()
{
   mLevelInfos.clear();
}


void ServerGame::addLevelInfo(const LevelInfo &levelInfo)
{
   mLevelInfos.push_back(levelInfo);
}


bool ServerGame::isTestServer()
{
   return mTestMode;
}


AbstractTeam *ServerGame::getNewTeam()
{
   return new Team;
}


// Creates a set of LevelInfos that are empty except for the filename.  They will be fleshed out later.
// This gets called when you first load the host menu
void ServerGame::buildBasicLevelInfoList(const Vector<string> &levelList)
{
   mLevelInfos.clear();

   for(S32 i = 0; i < levelList.size(); i++)
      mLevelInfos.push_back(LevelInfo(levelList[i]));
}


void ServerGame::resetLevelLoadIndex()
{
   mLevelLoadIndex = 0;
}


// This is only used while we're building a list of levels to display on the host during loading.
string ServerGame::getLastLevelLoadName()
{
   if(mLevelInfos.size() == 0)     // Could happen if there are no valid levels specified wtih -levels param, for example
      return "";
   else if(mLevelLoadIndex == 0)    // Still not sure when this would happen
      return "";
   else
      return mLevelInfos[mLevelLoadIndex - 1].levelName.getString();
}


// Return true if the only client connected is the one we passed; don't consider bots
bool ServerGame::onlyClientIs(GameConnection *client)
{
   GameType *gameType = gServerGame->getGameType();

   if(!gameType)
      return false;

   for(S32 i = 0; i < getClientCount(); i++)
      if(!mClientInfos[i]->isRobot() && mClientInfos[i]->getConnection() != client)
         return false;

   return true;
}



// Control whether we're in shut down mode or not
void ServerGame::setShuttingDown(bool shuttingDown, U16 time, GameConnection *who, StringPtr reason)
{
   mShuttingDown = shuttingDown;
   if(shuttingDown)
   {
      mShutdownOriginator = who;
      const char *name = mShutdownOriginator->getClientInfo()->getName().getString();

      // If there's no other clients, then just shutdown now
      if(onlyClientIs(mShutdownOriginator))
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown requested by %s.  No other players, so shutting down now.", name);
         mShutdownTimer.reset(1);
      }
      else
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown in %d seconds, requested by %s, for reason [%s].", time, name, reason.getString());
         mShutdownTimer.reset(time * 1000);
      }
   }
   else
      logprintf(LogConsumer::ServerFilter, "Server shutdown canceled.");
}


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
// This is only used on the server to provide quick level information without having to load the level
// (like with playlists or menus)
// Warning: Mungs chunk!
LevelInfo getLevelInfoFromFileChunk(char *chunk, S32 size, LevelInfo &levelInfo)
{
   S32 cur = 0;
   S32 startingCur = 0;

   bool foundGameType = false;
   bool foundLevelName = false;
   bool foundMinPlayers = false;
   bool foundMaxPlayers = false;

   while(cur < size && !(foundGameType && foundLevelName && foundMinPlayers && foundMaxPlayers))
   {
      if(chunk[cur] < 32)
      {
         if(cur - startingCur > 5)
         {
            char c = chunk[cur];
            chunk[cur] = 0;
            Vector<string> list = parseString(&chunk[startingCur]);
            chunk[cur] = c;

            if(list.size() >= 1 && list[0].find("GameType") != string::npos)
            {
               // Convert legacy Hunters game
               if(!stricmp(list[0].c_str(), "HuntersGameType"))
                  list[0] = "NexusGameType";

               TNL::Object *theObject = TNL::Object::create(list[0].c_str());  // Instantiate a gameType object
               GameType *gt = dynamic_cast<GameType*>(theObject);              // and cast it
               if(gt)
               {
                  levelInfo.levelType = gt->getGameType();
                  delete gt;
                  foundGameType = true;
               }
            }
            else if(list.size() >= 2 && list[0] == "LevelName")
            {
               string levelName = list[1];

               for(S32 i = 2; i < list.size(); i++)
                  levelName += " " + list[i];

               levelInfo.levelName = levelName;

               foundLevelName = true;
            }
            else if(list.size() >= 2 && list[0] == "MinPlayers")
            {
               levelInfo.minRecPlayers = atoi(list[1].c_str());
               foundMinPlayers = true;
            }
            else if(list.size() >= 2 && list[0] == "MaxPlayers")
            {
               levelInfo.maxRecPlayers = atoi(list[1].c_str());
               foundMaxPlayers = true;
            }
         }
         startingCur = cur + 1;
      }
      cur++;
   }
   return levelInfo;
}


void ServerGame::loadNextLevelInfo()
{
   FolderManager *folderManager = getSettings()->getFolderManager();

   string levelFile = folderManager->findLevelFile(mLevelInfos[mLevelLoadIndex].levelFileName.getString());

   if(getLevelInfo(levelFile, mLevelInfos[mLevelLoadIndex]))    // Populate mLevelInfos[i] with data from levelFile
      mLevelLoadIndex++;
   else
      mLevelInfos.erase(mLevelLoadIndex);

   if(mLevelLoadIndex == mLevelInfos.size())
      hostingModePhase = DoneLoadingLevels;
}


// Populates levelInfo with data from fullFilename
bool ServerGame::getLevelInfo(const string &fullFilename, LevelInfo &levelInfo)
{
   TNLAssert(levelInfo.levelFileName != "", "Invalid assumption");

   FILE *f = fopen(fullFilename.c_str(), "rb");

   if(f)
   {
      char data[1024 * 4];  // 4 kb should be enough to fit all parameters at the beginning of level; we don't need to read everything
      S32 size = (S32)fread(data, 1, sizeof(data), f);
      fclose(f);

      getLevelInfoFromFileChunk(data, size, levelInfo);

      // Provide a default levelname
      if(levelInfo.levelName == "")
         levelInfo.levelName = levelInfo.levelFileName;     // was mLevelInfos[mLevelLoadIndex].levelFileName

      return true;
   }
   else
   {
      // was mLevelInfos[mLevelLoadIndex].levelFileName.getString()
      logprintf(LogConsumer::LogWarning, "Could not load level %s [%s].  Skipping...", levelInfo.levelFileName.getString(), fullFilename.c_str());
      return false;
   }
}


// Helps resolve meta-indices such as NEXT_LEVEL.  If you pass it a normal level index, you'll just get that back.
S32 ServerGame::getAbsoluteLevelIndex(S32 indx)
{
   if(indx == NEXT_LEVEL)
      return (mCurrentLevelIndex + 1) % mLevelInfos.size();

   else if(indx == PREVIOUS_LEVEL)
   {
      indx = mCurrentLevelIndex - 1;
      return (indx >= 0) ? indx : mLevelInfos.size() - 1;
   }

   else if(indx == REPLAY_LEVEL)
      return mCurrentLevelIndex;

   else if(indx < 0 || indx >= mLevelInfos.size())    // Out of bounds index specified
      return 0;

   return indx;
}


// Get the level name, as defined in the level file
StringTableEntry ServerGame::getLevelNameFromIndex(S32 indx)
{
   return mLevelInfos[getAbsoluteLevelIndex(indx)].levelName;
}


// Get the filename the level is saved under
string ServerGame::getLevelFileNameFromIndex(S32 indx)
{
   if(indx < 0 || indx >= mLevelInfos.size())
      return "";
   else
      return mLevelInfos[indx].levelFileName.getString();
}


// Return filename of level currently in play
StringTableEntry ServerGame::getCurrentLevelFileName()
{
   return mLevelInfos[mCurrentLevelIndex].levelFileName;
}


// Return name of level currently in play
StringTableEntry ServerGame::getCurrentLevelName()
{
   return mLevelInfos[mCurrentLevelIndex].levelName;
}


// Return type of level currently in play
GameTypes ServerGame::getCurrentLevelType()
{
   return mLevelInfos[mCurrentLevelIndex].levelType;
}


StringTableEntry ServerGame::getCurrentLevelTypeName()
{
   return GameType::getGameTypeName(getCurrentLevelType());
}


bool ServerGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName, GridDatabase *database)
{
   if(!stricmp(argv[0], "Spawn"))
   {
      if(argc >= 4)
      {
         S32 teamIndex = atoi(argv[1]);
         Point p;
         p.read(argv + 2);
         p *= getGridSize();

         if(teamIndex >= 0 && teamIndex < getTeamCount())   // Normal teams; ignore if invalid
            ((Team *)getTeam(teamIndex))->addSpawnPoint(p);
         else if(teamIndex == -1)                           // Neutral spawn point, add to all teams
            for(S32 i = 0; i < getTeamCount(); i++)
               ((Team *)getTeam(i))->addSpawnPoint(p);
      }
   }
   else if(!stricmp(argv[0], "FlagSpawn"))      // FlagSpawn <team> <x> <y> [timer]
   {
      if(argc >= 4)
      {
         S32 teamIndex = atoi(argv[1]);
         Point p;
         p.read(argv + 2);
         p *= getGridSize();
   
         S32 time = (argc > 4) ? atoi(argv[4]) : FlagSpawn::DEFAULT_RESPAWN_TIME;
   
         FlagSpawn spawn = FlagSpawn(p, time);
   
         // Following works for Nexus & Soccer games because they are not TeamFlagGame.  Currently, the only
         // TeamFlagGame is CTF.
   
         if(getGameType()->isTeamFlagGame() && (teamIndex >= 0 && teamIndex < getTeamCount()) )   // If we can't find a valid team...
            ((Team *)getTeam(teamIndex))->addFlagSpawn(spawn);
         else
            getGameType()->addFlagSpawn(spawn);                                                   // ...then put it in the non-team list
      }
   }
   else if(!stricmp(argv[0], "AsteroidSpawn"))      // AsteroidSpawn <x> <y> [timer]      // TODO: Move this to AsteroidSpawn class?
   {
      AsteroidSpawn *spawn = new AsteroidSpawn();

      if(spawn->processArguments(argc - 1, argv + 1, this))   // processArguments don't like "AsteroidSpawn" as argv[0]
         getGameType()->addItemSpawn(spawn);
   }
   else if(!stricmp(argv[0], "CircleSpawn"))      // CircleSpawn <x> <y> [timer]      // TODO: Move this to CircleSpawn class?
   {
      CircleSpawn *spawn = new CircleSpawn();

      if(spawn->processArguments(argc - 1, argv + 1, this))
         getGameType()->addItemSpawn(spawn);
   }
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      if(argc >= 2)
      {
         WallRec barrier;
         barrier.width = F32(atof(argv[1]));

         if(barrier.width < Barrier::MIN_BARRIER_WIDTH)
            barrier.width = Barrier::MIN_BARRIER_WIDTH;
         else if(barrier.width > Barrier::MAX_BARRIER_WIDTH)
            barrier.width = Barrier::MAX_BARRIER_WIDTH;

   
         for(S32 i = 2; i < argc; i++)
            barrier.verts.push_back(F32(atof(argv[i])) * getGridSize());
   
         if(barrier.verts.size() > 3)
         {
            barrier.solid = false;
            getGameType()->addWall(barrier, this);
         }
      }
   }
   // TODO: Integrate code above with code above!!  EASY!!
   else if(!stricmp(argv[0], "BarrierMakerS") || !stricmp(argv[0], "PolyWall"))
   {
      bool width = false;

      if(!stricmp(argv[0], "BarrierMakerS"))
      {
         logprintf(LogConsumer::LogLevelError, "BarrierMakerS has been deprecated.  Please use PolyWall instead.");
         width = true;
      }

      if(argc >= 2)
      { 
         WallRec barrier;
         
         if(width)      // BarrierMakerS still width, though we ignore it
            barrier.width = F32(atof(argv[1]));
         else           // PolyWall does not have width specified
            barrier.width = 1;

         for(S32 i = 2 - (width ? 0 : 1); i < argc; i++)
            barrier.verts.push_back(F32(atof(argv[i])) * getGridSize());

         if(barrier.verts.size() > 3)
         {
            barrier.solid = true;
            getGameType()->addWall(barrier, this);
         }
      }
   }
   else 
      return false;

   return true;
}


// Highest ratings first -- runs on server only, so these should be FullClientInfos
static S32 QSORT_CALLBACK RatingSort(ClientInfo **a, ClientInfo **b)
{
   F32 diff = (*a)->getCalculatedRating() - (*b)->getCalculatedRating();

   if(diff == 0) 
      return 0;

   return diff > 0 ? 1 : -1;
}


// Pass -1 to go to next level, otherwise pass an absolute level number
void ServerGame::cycleLevel(S32 nextLevel)
{
   cleanUp();
   mLevelSwitchTimer.clear();
   mScopeAlwaysList.clear();

   // mGameType will be NULL here the first time this gets run, but should never be NULL thereafter
   if(mGameType)
   {
      for(S32 i = 0; i < getClientCount(); i++)
      {
         ClientInfo *clientInfo = getClientInfo(i);
         GameConnection *conn = clientInfo->getConnection();

         conn->resetGhosting();
         clientInfo->mOldLoadout.clear();
         
         clientInfo->resetLoadout();
         conn->switchedTeamCount = 0;

         clientInfo->setScore(0); // Reset player scores, for non team game types
      }
   }

   setCurrentLevelIndex(nextLevel, getPlayerCount());
   
   string levelFile = getLevelFileNameFromIndex(mCurrentLevelIndex);

   logprintf(LogConsumer::ServerFilter, "Loading %s [%s]... \\", getLevelNameFromIndex(mCurrentLevelIndex).getString(), levelFile.c_str());

   // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
   loadLevel(levelFile);

   // Make sure we have a gameType... if we don't we'll add a default one here
   if(!getGameType())   // loadLevel can fail (missing file) and not create GameType
   {
      logprintf(LogConsumer::LogLevelError, "Warning: Missing game type parameter in level \"%s\"", levelFile.c_str());

      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }

   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogLevelError, "Warning: Missing team in level \"%s\"", levelFile.c_str());

   logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());

   computeWorldObjectExtents();                       // Compute world Extents nice and early

   // Not sure if this is needed, but might be as long as we are still loading zones from level files... but I think we aren't anymore
   mDatabaseForBotZones.removeEverythingFromDatabase();    

   // Try and load Bot Zones for this level, set flag if failed
   // We need to run buildBotMeshZones in order to set mAllZones properly, which is why I (sort of) disabled the use of hand-built zones in level files
#ifdef ZAP_DEDICATED
   mGameType->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, false);
#else
   mGameType->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, gClientGame != NULL);
#endif
   //}

   // Clear team info for all clients
   resetAllClientTeams();

   // Now add players to the gameType, from highest rating to lowest in an attempt to create ratings-based teams
   mClientInfos.sort(RatingSort);

   if(mGameType.isValid())
      for(S32 i = 0; i < getClientCount(); i++)
      {
         mGameType->serverAddClient(getClientInfo(i));
         getClientInfo(i)->getConnection()->activateGhosting();      // Tell clients we're done sending objects and are ready to start playing
      }
}


// Resets all player team assignments
void ServerGame::resetAllClientTeams()
{
   for(S32 i = 0; i < getClientCount(); i++)
      getClientInfo(i)->setTeamIndex(NO_TEAM);
}


// Set mCurrentLevelIndex to refer to the next level we'll play
void ServerGame::setCurrentLevelIndex(S32 nextLevel, S32 playerCount)
{
   if(nextLevel >= FIRST_LEVEL)          // Go to specified level
      mCurrentLevelIndex = (nextLevel < mLevelInfos.size()) ? nextLevel : FIRST_LEVEL;

   else if(nextLevel == NEXT_LEVEL)      // Next level
   {
      // If game is supended, then we are waiting for another player to join.  That means that (probably)
      // there are either 0 or 1 players, so the next game will need to be good for 1 or 2 players.
      if(mGameSuspended)
         playerCount++;

      bool first = true;
      bool found = false;

      U32 currLevel = mCurrentLevelIndex;

      // Cycle through the levels looking for one that matches our player counts
      while(first || mCurrentLevelIndex != currLevel)
      {
         mCurrentLevelIndex = (mCurrentLevelIndex + 1) % mLevelInfos.size();

         S32 minPlayers = mLevelInfos[mCurrentLevelIndex].minRecPlayers;
         S32 maxPlayers = mLevelInfos[mCurrentLevelIndex].maxRecPlayers;

         if(maxPlayers <= 0)        // i.e. limit doesn't apply or is invalid (note if limit doesn't apply on the minPlayers, 
            maxPlayers = S32_MAX;   // then it works out because the smallest number of players is 1).

         if(playerCount >= minPlayers && playerCount <= maxPlayers)
         {
            found = true;
            break;
         }

         // else do nothing to play the next level, which we found above
         first = false;
      }

      // We didn't find a suitable level... just proceed to the next one in the list
      if(!found)
      {
         mCurrentLevelIndex++;
         if(S32(mCurrentLevelIndex) >= mLevelInfos.size())
            mCurrentLevelIndex = FIRST_LEVEL;
      }
   } 
   else if(nextLevel == PREVIOUS_LEVEL)
      mCurrentLevelIndex = mCurrentLevelIndex > 0 ? mCurrentLevelIndex - 1 : mLevelInfos.size() - 1;

   //else if(nextLevel == REPLAY_LEVEL)    // Replay level, do nothing
   //   mCurrentLevelIndex += 0;
}


// Enter suspended animation mode
void ServerGame::suspendGame()
{
   mGameSuspended = true;
   cycleLevel(NEXT_LEVEL);    // Advance to beginning of next level
}


 // Suspend at player's request
void ServerGame::suspendGame(GameConnection *requestor)
{
   if(getPlayerCount() > 1)        // Should never happen, but will protect against hacked clients
      return;

   mGameSuspended = true;    
   mSuspendor = requestor;
   //mCommanderZoomDelta = CommanderMapZoomTime;     // When suspended, we show cmdr's map, need to make sure it's fully zoomed out

   cycleLevel(REPLAY_LEVEL);  // Restart current level to make setting traps more difficult
}

 
// Resume game after it is no longer suspended
void ServerGame::unsuspendGame(bool remoteRequest)
{
   mGameSuspended = false;
   if(mSuspendor && !remoteRequest)     // If the request is from remote server, don't need to alert that server!
      mSuspendor->s2cUnsuspend();

   mSuspendor = NULL;
}


void ServerGame::suspenderLeftGame()
{
   mSuspendor = NULL;
}


GameConnection *ServerGame::getSuspendor()
{
   return mSuspendor;
}


// Need to handle both forward and backward slashes... will return pathname with trailing delimeter.
inline string getPathFromFilename(const string &filename)
{
   size_t pos1 = filename.rfind("/");
   size_t pos2 = filename.rfind("\\");

   if(pos1 == string::npos)
      pos1 = 0;

   if(pos2 == string::npos)
      pos2 = 0;

   return filename.substr( 0, max(pos1, pos2) + 1 );
}


extern md5wrapper md5;

bool ServerGame::loadLevel(const string &levelFileName)
{
   FolderManager *folderManager = getSettings()->getFolderManager();

   resetLevelInfo();

   mObjectsLoaded = 0;

   string filename = folderManager->findLevelFile(levelFileName);

   //cleanUp();
   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }

#ifdef PRINT_SOMETHING
   logprintf("1 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());
#endif
   if(loadLevelFromFile(filename, false, getGameObjDatabase()))
      mLevelFileHash = md5.getHashFromFile(filename);    // TODO: Combine this with the reading of the file we're doing anyway in initLevelFromFile()
   else
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }
#ifdef PRINT_SOMETHING
   logprintf("2 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());
#endif

   // We should have a gameType by the time we get here... but in case we don't, we'll add a default one now
   if(!getGameType())
   {
      logprintf(LogConsumer::LogWarning, "Warning: Missing game type parameter in level \"%s\"", levelFileName.c_str());
      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }


   // If there was a script specified in the level file, now might be a fine time to try running it!
   string scriptName = getGameType()->getScriptName();

   if(scriptName != "")
      runLevelGenScript(folderManager, scriptName, *getGameType()->getScriptArgs(), getGameObjDatabase());

   // Script specified in INI globalLevelLoadScript
   if(mSettings->getIniSettings()->globalLevelScript != "")
      runLevelGenScript(folderManager, mSettings->getIniSettings()->globalLevelScript, *getGameType()->getScriptArgs(), getGameObjDatabase());

   //  Check after script, script might add Teams
   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogWarning, "Warning: Missing Team in level \"%s\"", levelFileName.c_str());

   getGameType()->onLevelLoaded();

   return true;
}


void ServerGame::addClient(ClientInfo *clientInfo)
{
   TNLAssert(!clientInfo->isRobot(), "This only gets called for players");

   GameConnection *conn = clientInfo->getConnection();

   // If client has level change or admin permissions, send a list of levels and their types to the connecting client
   if(clientInfo->isLevelChanger() || clientInfo->isAdmin())
      conn->sendLevelList();

   // If we're shutting down, display a notice to the user... but still let them connect normally
   if(mShuttingDown)
      conn->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000, mShutdownOriginator->getClientInfo()->getName(), 
                                         "Sorry -- server shutting down", false);

   if(mGameType.isValid())
   {
      mGameType->serverAddClient(clientInfo);
      addToClientList(clientInfo);
   }

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(ClientInfo *clientInfo)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(clientInfo);

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);
}


GridDatabase *ServerGame::getBotZoneDatabase()
{
   return &mDatabaseForBotZones;
}


U32 ServerGame::getMaxPlayers()
{
   return mSettings->getMaxPlayers();
}


bool ServerGame::isDedicated()
{
   return mDedicated;
}


void ServerGame::setDedicated(bool dedicated)
{
   mDedicated = dedicated;
}


bool ServerGame::isFull()
{
   return (U32) getPlayerCount() >= mSettings->getMaxPlayers();
}


// Only called from outside ServerGame
bool ServerGame::isReadyToShutdown(U32 timeDelta)
{
   return mShuttingDown && (mShutdownTimer.update(timeDelta) || onlyClientIs(mShutdownOriginator));
}


bool ServerGame::isServer()
{
   return true;
}


// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   // Simulate CPU stutter without impacting gClientGame
   if(mSettings->getSimulatedStutter() > 0)
   {
      if(mStutterTimer.getCurrent() > 0)      
      {
         if(mStutterTimer.update(timeDelta))
            mStutterSleepTimer.reset();               // Go to sleep
      }
      else     // We're sleeping
      {
         if(mStutterSleepTimer.update(timeDelta))     // Wake up!
         {
            mStutterTimer.reset();
            timeDelta += mAccumulatedSleepTime;       // Give serverGame credit for time we slept
            mAccumulatedSleepTime = 0;
         }
         else
         {
            mAccumulatedSleepTime += timeDelta;
            return;
         }
      }
   }

   if(mVoteTimer != 0)
   {
      if(timeDelta < mVoteTimer)  // voting continue
      {
         if((mVoteTimer-timeDelta) % 1000 > mVoteTimer % 1000) // show message
         {
            Vector<StringTableEntry> e;
            Vector<StringPtr> s;
            Vector<S32> i;
            StringTableEntry msg;
            i.push_back(mVoteTimer / 1000);
            switch(mVoteType)
            {
            case 0:
               msg = "/YES or /NO : %i0 : Change Level to %e0";
               e.push_back(getLevelNameFromIndex(mVoteNumber));
               break;
            case 1:
               msg = "/YES or /NO : %i0 : Add time %i1 minutes";
               i.push_back(mVoteNumber / 60000);
               break;
            case 2:
               msg = "/YES or /NO : %i0 : Set time %i1 minutes %i2 seconds";
               i.push_back(mVoteNumber / 60000);
               i.push_back((mVoteNumber / 1000) % 60);
               break;
            case 3:
               msg = "/YES or /NO : %i0 : Set score %i1";
               i.push_back(mVoteNumber);
               break;
            case 4:
               msg = "/YES or /NO : %i0 : Change team %e0 to %e1";
               e.push_back(mVoteClientName);
               e.push_back(getTeamName(mVoteNumber));
               break;
            case 5:
               msg = "/YES or /NO : %i0 : %s0";
               s.push_back(mVoteString);
               break;
            }
            
            bool WaitingToVote = false;

            for(S32 i = 0; i < getClientCount(); i++)
            {
               ClientInfo *clientInfo = getClientInfo(i);
               GameConnection *conn = clientInfo->getConnection();

               if(conn->mVote == 0 && !clientInfo->isRobot())
               {
                  WaitingToVote = true;
                  conn->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, msg, e, s, i);
               }
            }

            if(!WaitingToVote)
               mVoteTimer = timeDelta + 1;  // no more waiting when everyone have voted.
         }
         mVoteTimer -= timeDelta;
      }
      else                        // Vote ends
      {
         S32 voteYes = 0;
         S32 voteNo = 0;
         S32 voteNothing = 0;
         mVoteTimer = 0;

         for(S32 i = 0; i < getClientCount(); i++)
         {
            ClientInfo *clientInfo = getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            if(conn->mVote == 1)
               voteYes++;
            else if(conn->mVote == 2)
               voteNo++;
            else if(!clientInfo->isRobot())
               voteNothing++;
         }

         bool votePass = voteYes     * mSettings->getIniSettings()->voteYesStrength + 
                         voteNo      * mSettings->getIniSettings()->voteNoStrength  + 
                         voteNothing * mSettings->getIniSettings()->voteNothingStrength > 0;
         if(votePass)
         {
            mVoteTimer = 0;
            switch(mVoteType)
            {
               case 0:
                  mNextLevel = mVoteNumber;
                  if(mGameType)
                     mGameType->gameOverManGameOver();
                  break;
               case 1:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(mVoteNumber);                                  // Increase "official time"
                     mGameType->s2cSetTimeRemaining(mGameType->getRemainingGameTimeInMs());   // Broadcast time to clients
                  }
                  break;   
               case 2:
                  if(mGameType)
                  {
                     mGameType->extendGameTime(S32(mVoteNumber - mGameType->getRemainingGameTimeInMs()));
                     mGameType->s2cSetTimeRemaining(mGameType->getRemainingGameTimeInMs());    // Broadcast time to clients
                  }
                  break;
               case 3:
                  if(mGameType)
                  {
                     mGameType->setWinningScore(mVoteNumber);
                     mGameType->s2cChangeScoreToWin(mVoteNumber, mVoteClientName);    // Broadcast score to clients
                  }
                  break;
               case 4:
                  if(mGameType)
                  {
                     for(S32 i = 0; i < getClientCount(); i++)
                     {
                        ClientInfo *clientInfo = getClientInfo(i);

                        if(clientInfo->getName() == mVoteClientName)
                           mGameType->changeClientTeam(clientInfo, mVoteNumber);
                     }
                  }
                  break;
               case 5:
                  if(mGameType)
                  {
                     for(S32 i = 0; i < getClientCount(); i++)
                     {
                        ClientInfo *clientInfo = getClientInfo(i);

                        if(clientInfo->getName() == mVoteClientName)
                           mGameType->changeClientTeam(clientInfo, mVoteNumber);
                     }
                  }
                  break;
               default:
                  TNLAssert(false, "Invalid option in switch statement!");
            } // switch
         } // if


         Vector<StringTableEntry> e;
         Vector<StringPtr> s;
         Vector<S32> i;

         i.push_back(voteYes);
         i.push_back(voteNo);
         i.push_back(voteNothing);
         e.push_back(votePass ? "Pass" : "Fail");

         for(S32 i = 0; i < getClientCount(); i++)
         {
            ClientInfo *clientInfo = getClientInfo(i);
            GameConnection *conn = clientInfo->getConnection();

            conn->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, "Vote %e0 - %i0 yes, %i1 no, %i2 did not vote", e, s, i);

            if(!votePass && clientInfo->getName() == mVoteClientName)
               conn->mVoteTime = mSettings->getIniSettings()->voteRetryLength * 1000;
         }
      }
   }

   // If there are no players on the server, we can enter "suspended animation" mode, but not during the first half-second of hosting.
   // This will prevent locally hosted game from immediately suspending for a frame, giving the local client a chance to 
   // connect.  A little hacky, but works!
   if(getPlayerCount() == 0 && !mGameSuspended && mCurrentTime != 0)
      suspendGame();
   else if(mGameSuspended && ( (getPlayerCount() > 0 && !mSuspendor) || getPlayerCount() > 1 ))
      unsuspendGame(false);

   if(timeDelta > 2000)   // Prevents timeDelta from going too high, usually when after the server was frozen.
      timeDelta = 100;

   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);    // Connect to master server if not connected

   
   mSettings->getBanList()->updateKickList(timeDelta);    // Unban players who's bans have expired

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
   {
      MasterServerConnection *masterConn = gServerGame->getConnectionToMaster();

      static StringTableEntry prevCurrentLevelName;   // Using static, so it holds the value when it comes back here.
      static GameTypes prevCurrentLevelType;
      static S32 prevRobotCount;
      static S32 prevPlayerCount;

      if(masterConn && masterConn->isEstablished())
      {
         // Only update if something is different
         if(prevCurrentLevelName != getCurrentLevelName() || prevCurrentLevelType != getCurrentLevelType() || 
            prevRobotCount != getRobotCount()             || prevPlayerCount != getPlayerCount())
         {
            prevCurrentLevelName = getCurrentLevelName();
            prevCurrentLevelType = getCurrentLevelType();
            prevRobotCount = getRobotCount();
            prevPlayerCount = getPlayerCount();

            masterConn->updateServerStatus(getCurrentLevelName(), getCurrentLevelTypeName(), getRobotCount(), 
                                           getPlayerCount(), mSettings->getMaxPlayers(), mInfoFlags);
            mMasterUpdateTimer.reset(UpdateServerStatusTime);
         }
         else
            mMasterUpdateTimer.reset(CheckServerStatusTime);
      }
      else
      {
         prevPlayerCount = -1;   //Not sure if needed, but if disconnected, we need to update to master.
         mMasterUpdateTimer.reset(CheckServerStatusTime);
      }
   }

   mNetInterface->processConnections();

   // If we have a data transfer going on, process it
   if(!dataSender.isDone())
      dataSender.sendNextLine();


   if(mGameSuspended)     // If game is suspended, we need do nothing more
      return;

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(!clientInfo->isRobot())
      {
         GameConnection *conn = clientInfo->getConnection();

         if(conn->mChatTimer > timeDelta)
            conn->mChatTimer -= timeDelta;
         else
         {
            conn->mChatTimer = 0;
            conn->mChatTimerBlocked = false;
         }

         conn->addToTimeCredit(timeDelta);
         conn->updateAuthenticationTimer(timeDelta);

         if(conn->mVoteTime <= timeDelta)
            conn->mVoteTime = 0;
         else
            conn->mVoteTime -= timeDelta;
      }
   }

   // Compute new world extents -- these might change if a ship flies far away, for example...
   // In practice, we could probably just set it and forget it when we load a level.
   // Compute it here to save recomputing it for every robot and other method that relies on it.
   computeWorldObjectExtents();

   // For robots
   Robot::idleAllBots(timeDelta);

   fillVector2.clear();  // need to have our own local fillVector
   mGameObjDatabase->findObjects(fillVector2);


   // Visit each game object, handling moves and running its idle method
   for(S32 i = 0; i < fillVector2.size(); i++)
   {
      if(fillVector2[i]->isDeleted())
         continue;

      GameObject *obj = dynamic_cast<GameObject *>(fillVector2[i]);
      TNLAssert(obj, "Bad cast!");

      // Here is where the time gets set for all the various object moves
      Move thisMove = obj->getCurrentMove();
      thisMove.time = timeDelta;

      // Give the object its move, then have it idle
      obj->setCurrentMove(thisMove);
      obj->idle(GameObject::ServerIdleMainLoop);
   }

   if(mGameType)
      mGameType->idle(GameObject::ServerIdleMainLoop, timeDelta);

   processDeleteList(timeDelta);


   // Load a new level if the time is out on the current one
   if(mLevelSwitchTimer.update(timeDelta))
   {
      // Normalize ratings for this game
      getGameType()->updateRatings();
      cycleLevel(mNextLevel);
      mNextLevel = NEXT_LEVEL;
   }


   // Lastly, play any sounds server might have made...
   if(isDedicated())   // non-dedicated will process sound in client side.
      SoundSystem::processAudio(mSettings->getIniSettings()->alertsVolLevel, 0, 0);    // No music or voice on server!
}


bool ServerGame::startHosting()
{
   if(mSettings->getFolderManager()->levelDir == "")     // Never did resolve a leveldir... no hosting for you!
      return false;

   hostingModePhase = Hosting;

   for(S32 i = 0; i < getLevelNameCount(); i++)
      logprintf(LogConsumer::ServerFilter, "\t%s [%s]", getLevelNameFromIndex(i).getString(), 
                getLevelFileNameFromIndex(i).c_str());

   if(!getLevelNameCount())      // No levels loaded... we'll crash if we try to start a game       
      return false;      

   cycleLevel(FIRST_LEVEL);      // Start with the first level

   return true;
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset(LevelSwitchTime);
}


S32 ServerGame::addUploadedLevelInfo(const char *filename, LevelInfo &levelInfo)
{
   if(levelInfo.levelName == "")            // Make sure we have something in the name field
      levelInfo.levelName = filename;

   levelInfo.levelFileName = filename; 

   // Check if we already have this one
   for(S32 i = 0; i < mLevelInfos.size(); i++)
      if(mLevelInfos[i].levelFileName == levelInfo.levelFileName)
         return i;

   // We don't... so add it!
   mLevelInfos.push_back(levelInfo);

   // Let levelChangers know about the new level

   for(S32 i = 0; i < getClientCount(); i++)
   {
      ClientInfo *clientInfo = getClientInfo(i);

      if(clientInfo->isLevelChanger())
         clientInfo->getConnection()->s2cAddLevel(levelInfo.levelName, levelInfo.levelType);
   }

   return mLevelInfos.size() - 1;
}


Rect Game::getWorldExtents()
{
   return mWorldExtents;
}


bool Game::isTestServer()
{
   return false;
}


const Color *Game::getTeamColor(S32 teamId) const
{
   return mActiveTeamManager->getTeamColor(teamId);
}


void Game::onReadTeamParam(S32 argc, const char **argv)
{
   if(getTeamCount() < GameType::MAX_TEAMS)     // Too many teams?
   {
      AbstractTeam *team = getNewTeam();
      if(team->processArguments(argc, argv))
         addTeam(team);
   }
}


void Game::setActiveTeamManager(TeamManager *teamManager)
{
   mActiveTeamManager = teamManager;
}

};


