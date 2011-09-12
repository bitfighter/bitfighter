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
#include "EngineeredItem.h"    // For EngineerModuleDeployer
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
#include "huntersGame.h"         // for creating new HuntersFlagItem

#include "IniFile.h"             // For CIniFile def


#include "BotNavMeshZone.h"      // For zone clearing code

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

// Global Game objects
ServerGame *gServerGame = NULL;

static Vector<DatabaseObject *> fillVector2;

// Declare some statics 
Vector<string> Game::mMasterAddressList;



#ifndef ZAP_DEDICATED
class ClientGame;
extern ClientGame *gClientGame; // only used to see if we have a ClientGame, for buildBotMeshZones
#endif

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// Constructor
Game::Game(const Address &theBindAddress, GameSettings *settings) : mGameObjDatabase(new GridDatabase())      //? was without new
{
   mSettings = settings;

   buildModuleInfos();
   mNextMasterTryTime = 0;
   mReadyToConnectToMaster = false;

   mCurrentTime = 0;
   mGameSuspended = false;

   mTimeUnconnectedToMaster = 0;

   mNetInterface = new GameNetInterface(theBindAddress, this);
   mHaveTriedToConnectToMaster = false;

   mWallSegmentManager = new WallSegmentManager();    // gets deleted in destructor
}


// Destructor
Game::~Game()
{
   delete mWallSegmentManager;
}


// Info about modules -- access via getModuleInfo()  -- static
void Game::buildModuleInfos()    
{
   if(mModuleInfos.size() > 0)         // Already built?
      return;

   for(S32 i = 0; i < ModuleCount; i++)
      mModuleInfos.push_back(ModuleInfo((ShipModule) i));
}


void Game::setGridSize(F32 gridSize) 
{ 
   mGridSize = max(min(gridSize, F32(MAX_GRID_SIZE)), F32(MIN_GRID_SIZE));
}


void Game::setScopeAlwaysObject(GameObject *theObject)
{
   mScopeAlwaysList.push_back(theObject);
}


GameNetInterface *Game::getNetInterface()
{
   return mNetInterface;
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
   return mTeams.size();
}


AbstractTeam *Game::getTeam(S32 team) const
{
   return mTeams[team].get();
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


void Game::removeTeam(S32 teamIndex)
{
   mTeams.erase(teamIndex);
}


void Game::addTeam(boost::shared_ptr<AbstractTeam> team)
{
   mTeams.push_back(team);
}


void Game::addTeam(boost::shared_ptr<AbstractTeam> team, S32 index)
{
   mTeams.insert(index);
   mTeams[index] = team;
}


void Game::replaceTeam(boost::shared_ptr<AbstractTeam> team, S32 index)
{
   mTeams[index] = team;
}


void Game::clearTeams()
{
   mTeams.clear();
}


void Game::setGameType(GameType *theGameType)      // TODO==> Need to store gameType as a shared_ptr or auto_ptr
{
   //delete mGameType;          // Cleanup, if need be
   mGameType = theGameType;
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
         throw LevelLoadException("Improperly formed GridSize parameter");

      setGridSize((F32)atof(argv[1]));
      return;
   }

   // Parse GameType line... All game types are of form XXXXGameType
   else if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
   {
      // validateGameType() will return a valid GameType string -- either what's passed in, or the default if something bogus was specified
      TNL::Object *theObject = TNL::Object::create(GameType::validateGameType(argv[0]));      
      GameType *gt = dynamic_cast<GameType *>(theObject);  // Force our new object to be a GameObject

      bool validArgs = gt->processArguments(argc - 1, argv + 1, NULL);

      gt->addToGame(this, database);    

      if(!validArgs || strcmp(gt->getClassName(), argv[0]))
         throw LevelLoadException("Improperly formed GameType parameter");
      
      return;
   }

   if(getGameType() && processLevelParam(argc, argv)) 
   {
      // Do nothing here
   }
   else if(getGameType() && processPseudoItem(argc, argv, levelFileName)) 
   {
      // Do nothing here
   }
   
   else
   {
      char obj[LevelLoader::MaxArgLen + 1];

      // Convert any HuntersFlagItem into FlagItem, only HuntersFlagItem will show up on ship
      if(!stricmp(argv[0], "HuntersFlagItem") || !stricmp(argv[0], "NexusFlagItem"))
         strcpy(obj, "FlagItem");
      else
         strncpy(obj, argv[0], LevelLoader::MaxArgLen);

      obj[LevelLoader::MaxArgLen] = '\0';
      TNL::Object *theObject = TNL::Object::create(obj);          // Create an object of the type specified on the line

      SafePtr<GameObject> object  = dynamic_cast<GameObject *>(theObject);  // Force our new object to be a GameObject
      EditorObject *eObject = dynamic_cast<EditorObject *>(theObject);


      if(!object && !eObject)    // Well... that was a bad idea!
      {
         logprintf(LogConsumer::LogWarning, "Unknown object type \"%s\" in level \"%s\"", obj, levelFileName.c_str());
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
            logprintf(LogConsumer::LogWarning, "Invalid arguments in object \"%s\" in level \"%s\"", obj, levelFileName.c_str());
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
   
   else if(!stricmp(argv[0], "SoccerPickup"))  // option for old style soccer, this option might get moved or removed
      onReadSoccerPickupParam(argc, argv);

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

   str += string("LevelName ") + gameType->getLevelName()->getString() + "\n";
   str += string("LevelDescription ") + gameType->getLevelDescription()->getString() + "\n";
   str += string("LevelCredits ") + gameType->getLevelCredits()->getString() + "\n";

   str += string("GridSize ") + ftos(mGridSize) + "\n";

   for(S32 i = 0; i < mTeams.size(); i++)
      str += mTeams[i]->toString() + "\n";

   str += gameType->getSpecialsLine() + "\n";

   if(gameType->getScriptName() != "")
      str += "Script " + gameType->getScriptLine() + "\n";

   str += string("MinPlayers") + (gameType->getMinRecPlayers() > 0 ? " " + itos(gameType->getMinRecPlayers()) : "") + "\n";
   str += string("MaxPlayers") + (gameType->getMaxRecPlayers() > 0 ? " " + itos(gameType->getMaxRecPlayers()) : "") + "\n";

   return str;
}


void Game::onReadTeamParam(S32 argc, const char **argv)
{
   if(getTeamCount() < GameType::MAX_TEAMS)   // Too many teams?
   {
      boost::shared_ptr<AbstractTeam> team = getNewTeam();
      if(team->processArguments(argc, argv))
         addTeam(team);
   }
}


// Only occurs in scripts
void Game::onReadTeamChangeParam(S32 argc, const char **argv)
{
   if(argc >= 2)   // Enough arguments?
   {
      S32 teamNumber = atoi(argv[1]);   // Team number to change

      if(teamNumber >= 0 && teamNumber < getTeamCount())
      {
         boost::shared_ptr<AbstractTeam> team = getNewTeam();
         team->processArguments(argc-1, argv+1);          // skip one arg
         replaceTeam(team, teamNumber);
      }
   }
}


void Game::onReadSpecialsParam(S32 argc, const char **argv)
{         
   for(S32 i = 1; i < argc; i++)
      if(!getGameType()->processSpecialsParam(argv[i]))
         logprintf(LogConsumer::LogWarning, "Invalid specials parameter: %s", argv[i]);
}


void Game::onReadSoccerPickupParam(S32 argc, const char **argv)
{
   logprintf(LogConsumer::LogWarning, "Level uses deprecated SoccerPickup line... parameter will be removed in 017!");

   if(argc < 2)
   {
      logprintf(LogConsumer::LogWarning, "Improperly formed (and deprecated!!) SoccerPickup parameter");
      return;
   }

   SoccerGameType *sgt = dynamic_cast<SoccerGameType *>(getGameType());

   if(sgt)
   {
      sgt->setSoccerPickupAllowed(
         !stricmp(argv[1], "yes") ||
         !stricmp(argv[1], "enable") ||
         !stricmp(argv[1], "on") ||
         !stricmp(argv[1], "activate") ||
         !stricmp(argv[1], "1") );
   }
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
      if(mMasterAddressList.size() == 0)
         return;

      if(mNextMasterTryTime < timeDelta && mReadyToConnectToMaster)
      {
         if(mHaveTriedToConnectToMaster)
         {
            mMasterAddressList.push_back(mMasterAddressList[0]);  // Rotate the list so as to try each one until we find one that works...
            mMasterAddressList.erase(0);
         }

         mHaveTriedToConnectToMaster = true;
         logprintf(LogConsumer::LogConnection, "%s connecting to master [%s]", isServer() ? "Server" : "Client", 
                    mMasterAddressList[0].c_str());

         Address addr(mMasterAddressList[0].c_str());
         if(addr.isValid())
         {
            mConnectionToMaster = new MasterServerConnection(this);
            mConnectionToMaster->connect(mNetInterface, addr);
         }

         mNextMasterTryTime = GameConnection::MASTER_SERVER_FAILURE_RETRY_TIME;     // 10 secs, just in case this attempt fails
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

// Static method
void Game::setMasterAddress(const string &firstChoice, const string &secondChoice)
{
   parseString((firstChoice != "" ? firstChoice : secondChoice).c_str(), mMasterAddressList, ',');
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
LevelInfo::LevelInfo(const StringTableEntry &name, const StringTableEntry &type)
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
   levelType = "Bitmatch";
   levelFileName = "";
   minRecPlayers = 0;
   maxRecPlayers = 0;
}


////////////////////////////////////////
////////////////////////////////////////

extern CmdLineSettings gCmdLineSettings;

// Constructor
ServerGame::ServerGame(const Address &address, GameSettings *settings, bool testMode, bool dedicated) : 
      Game(address, settings)
{
   mVoteTimer = 0;
   mNextLevel = NEXT_LEVEL;
   mPlayerCount = 0;

   mShuttingDown = false;

   Robot::setPaused(false);

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

   mStutterTimer.reset(1001 - gCmdLineSettings.stutter);    // Use 1001 to ensure timer is never set to 0
   mStutterSleepTimer.reset(gCmdLineSettings.stutter);
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

   mTeams.resize(0);
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
bool ServerGame::voteStart(GameConnection *client, S32 type, S32 number)
{
   if(!gIniSettings.voteEnable)
      return false;

   U32 VoteTimer;
   if(type == 4) // change team
      VoteTimer = gIniSettings.voteLengthToChangeTeam * 1000;
   else
      VoteTimer = gIniSettings.voteLength * 1000;
   if(VoteTimer == 0)
      return false;

   if(mVoteTimer != 0)
   {
      client->s2cDisplayErrorMessage("Can't start a new vote when there is one pending.");
      return true;
   }

   if(client->mVoteTime != 0)
   {
      Vector<StringTableEntry> e;
      Vector<StringPtr> s;
      Vector<S32> i;
      i.push_back(client->mVoteTime / 1000);
      client->s2cDisplayMessageESI(GameConnection::ColorRed, SFXNone, "Can't start vote, try again %i0 seconds later.", e, s, i);
      return true;
   }
   mVoteTimer = VoteTimer;
   mVoteType = type;
   mVoteNumber = number;
   mVoteClientName = client->getClientName();

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      walk->mVote = 0;

   client->mVote = 1;
   client->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, "Vote started, waiting for others to vote.");
   return true;
}


void ServerGame::voteClient(GameConnection *client, bool voteYes)
{
   if(mVoteTimer == 0)
      client->s2cDisplayErrorMessage("!!! Nothing to vote on");
   else if(client->mVote == (voteYes ? 1 : 2))
      client->s2cDisplayErrorMessage("!!! Already voted");
   else if(client->mVote == 0)
   {
      if(voteYes)
         client->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, "Voted Yes");
      else
         client->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, "Voted No");
   }
   else
   {
      if(voteYes)
         client->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, "Changed vote to Yes");
      else
         client->s2cDisplayMessage(GameConnection::ColorGreen, SFXNone, "Changed vote to No");
   }
   client->mVote = voteYes ? 1 : 2;
}

S32 ServerGame::getLevelNameCount()
{
   return mLevelInfos.size();
}


class Robot;

S32 ServerGame::getRobotCount()
{
   return Robot::getRobotCount();
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


// Control whether we're in shut down mode or not
void ServerGame::setShuttingDown(bool shuttingDown, U16 time, ClientRef *who, StringPtr reason)
{
   mShuttingDown = shuttingDown;
   if(shuttingDown)
   {
      mShutdownOriginator = who->clientConnection;

      // If there's no other clients, then just shutdown now
      if(GameConnection::onlyClientIs(mShutdownOriginator))
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown requested by %s.  No other players, so shutting down now.", 
                                                   mShutdownOriginator->getClientName().getString());
         mShutdownTimer.reset(1);
      }
      else
      {
         logprintf(LogConsumer::ServerFilter, "Server shutdown in %d seconds, requested by %s, for reason [%s].", 
            time, mShutdownOriginator->getClientName().getString(), reason.getString());
         mShutdownTimer.reset(time * 1000);
      }
   }
   else
      logprintf(LogConsumer::ServerFilter, "Server shutdown canceled.");
}


// Parse through the chunk of data passed in and find parameters to populate levelInfo with
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
            Vector<string> list = parseString(string(&chunk[startingCur]));
            chunk[cur] = c;

            if(list.size() >= 1 && list[0].find("GameType") != string::npos)
            {
               TNL::Object *theObject = TNL::Object::create(list[0].c_str());  // Instantiate a gameType object
               GameType *gt = dynamic_cast<GameType*>(theObject);              // and cast it
               if(gt)
               {
                  levelInfo.levelType = gt->getGameTypeString();
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
   ConfigDirectories *folderManager = getSettings()->getConfigDirs();

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
   return StringTableEntry( mLevelInfos[getAbsoluteLevelIndex(indx)].levelName.getString() );
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
StringTableEntry ServerGame::getCurrentLevelType()
{
   return mLevelInfos[mCurrentLevelIndex].levelType;
}


boost::shared_ptr<AbstractTeam> ServerGame::getNewTeam()
{
   return boost::shared_ptr<AbstractTeam>(new Team());
}


bool ServerGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName)
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
   
         FlagSpawn spawn = FlagSpawn(p, time * 1000);
   
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

      if(spawn->processArguments(argc, argv, this))
         getGameType()->addItemSpawn(spawn);
   }
   else if(!stricmp(argv[0], "CircleSpawn"))      // CircleSpawn <x> <y> [timer]      // TODO: Move this to CircleSpawn class?
   {
      CircleSpawn *spawn = new CircleSpawn();

      if(spawn->processArguments(argc, argv, this))
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
         logprintf(LogConsumer::LogWarning, "BarrierMakerS has been deprecated.  Please use PolyWall instead.");
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


// Should return a number between -1 and 1
F32 getCurrentRating(GameConnection *conn)
{
   if(conn->mTotalScore == 0 && conn->mGamesPlayed == 0)
      return .5;
   else if(conn->mTotalScore == 0)
      return (((conn->mGamesPlayed) * conn->mRating)) / (conn->mGamesPlayed);
   else
      return ((conn->mGamesPlayed * conn->mRating) + ((F32) conn->mScore / (F32) conn->mTotalScore)) / (conn->mGamesPlayed + 1);
}


// Highest ratings first
static S32 QSORT_CALLBACK RatingSort(GameConnection **a, GameConnection **b)
{
   F32 diff = getCurrentRating(*b) - getCurrentRating(*a);
   if(diff == 0) return 0;
   return diff > 0 ? 1 : -1;
}

// Pass -1 to go to next level, otherwise pass an absolute level number
void ServerGame::cycleLevel(S32 nextLevel)
{
   cleanUp();
   mLevelSwitchTimer.clear();
   mScopeAlwaysList.clear();

   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
   {
      walk->resetGhosting();
      walk->mOldLoadout.clear();
      walk->switchedTeamCount = 0;
   }

   if(nextLevel >= FIRST_LEVEL)          // Go to specified level
      mCurrentLevelIndex = (nextLevel < mLevelInfos.size()) ? nextLevel : FIRST_LEVEL;

   else if(nextLevel == NEXT_LEVEL)      // Next level
   {
      S32 players = mPlayerCount;
       
      // If game is supended, then we are waiting for another player to join.  That means that (probably)
      // there are either 0 or 1 players, so the next game will need to be good for 1 or 2 players.
      if(mGameSuspended)
         players++;

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

         if(players >= minPlayers && players <= maxPlayers)
         {
            found = true;
            break;
         }

         // else do nothing to play the next level, which we found above
         first = false;
      }

      // We didn't find a suitable level... just proceed to the next one
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


   logprintf(LogConsumer::ServerFilter, "Loading %s [%s]... \\", gServerGame->getLevelNameFromIndex(mCurrentLevelIndex).getString(), 
                                             gServerGame->getLevelFileNameFromIndex(mCurrentLevelIndex).c_str());

   // Load the level for real this time (we loaded it once before, when we started the server, but only to grab a few params)
   loadLevel(getLevelFileNameFromIndex(mCurrentLevelIndex));

   // Make sure we have a gameType... if we don't we'll add a default one here
   if(!getGameType())   // loadLevel can fail (missing file) and not create GameType
   {
      logprintf(LogConsumer::LogWarning, "Warning: Missing game type parameter in level \"%s\"", gServerGame->getLevelFileNameFromIndex(mCurrentLevelIndex).c_str());
      GameType *gameType = new GameType;
      gameType->addToGame(this, getGameObjDatabase());
   }

   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogWarning, "Warning: Missing Team in level \"%s\"", getLevelFileNameFromIndex(mCurrentLevelIndex).c_str());

   logprintf(LogConsumer::ServerFilter, "Done. [%s]", getTimeStamp().c_str());

   computeWorldObjectExtents();                       // Compute world Extents nice and early

   if(mDatabaseForBotZones.getObjectCount() != 0)     // There are some zones loaded in the level...
   {
      getGameType()->mBotZoneCreationFailed = false;
      BotNavMeshZone::IDBotMeshZones(this);
      BotNavMeshZone::buildBotNavMeshZoneConnections(this);
   }
   else
      // Try and load Bot Zones for this level, set flag if failed
#ifdef ZAP_DEDICATED
      getGameType()->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, false);
#else
      getGameType()->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this, gClientGame);
#endif


   // Build a list of our current connections
   Vector<GameConnection *> connectionList;
   GameType *gt = getGameType();
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
   {
      connectionList.push_back(walk);
   }

   // Now add the connections to the game type, from highest rating to lowest --> will create ratings-based teams
   connectionList.sort(RatingSort);

   for(S32 i = 0; i < connectionList.size(); i++)
   {
      GameConnection *gc = connectionList[i];

      if(mGameType.isValid())
         mGameType->serverAddClient(gc);
      gc->activateGhosting();
   }
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


// Need to handle both forward and backward slashes... will return pathname with trailing delimeter.
inline string getPathFromFilename( const string& filename )
{
   size_t pos1 = filename.rfind("/");
   size_t pos2 = filename.rfind("\\");

   if(pos1 == string::npos)
      pos1 = 0;

   if(pos2 == string::npos)
      pos2 = 0;

   return filename.substr( 0, max(pos1, pos2) + 1 );
}


extern OGLCONSOLE_Console gConsole;
extern md5wrapper md5;

bool ServerGame::loadLevel(const string &levelFileName)
{
   ConfigDirectories *folderManager = getSettings()->getConfigDirs();

   resetLevelInfo();

   mObjectsLoaded = 0;

   string filename = folderManager->findLevelFile(levelFileName);

   cleanUp();
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
   {
      string name = folderManager->findLevelGenScript(scriptName);  // Find full name of levelgen script

      if(name == "")
      {
         logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\" in level \"%s\"", 
                                    scriptName.c_str(), levelFileName.c_str());
         return false;
      }

      // The script file will be the first argument, subsequent args will be passed on to the script.
      // Now we've crammed all our action into the constructor... is this ok design?
      string *dir = &folderManager->levelDir;
      const Vector<string> *args = getGameType()->getScriptArgs();
      LuaLevelGenerator levelgen = LuaLevelGenerator(name, *dir, args, getGridSize(), getGameObjDatabase(), this, gConsole);
   }

   // Script specified in INI globalLevelLoadScript
   if(gIniSettings.globalLevelScript != "")
   {
      string name = folderManager->findLevelGenScript(gIniSettings.globalLevelScript);  // Find full name of levelgen script

      if(name == "")
      {
         logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\" in globalLevelScript", 
                                    getGameType()->getScriptName().c_str(), levelFileName.c_str());
         return false;
      }

      // The script file will be the first argument, subsequent args will be passed on to the script.
      // Now we've crammed all our action into the constructor... is this ok design?
      string *dir = &folderManager->levelDir;
      const Vector<string> *args = getGameType()->getScriptArgs();
      LuaLevelGenerator levelgen = LuaLevelGenerator(name, *dir, args, getGridSize(), getGameObjDatabase(), this, gConsole);
   }

   //  Check after script, script might add Teams
   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogWarning, "Warning: Missing Team in level \"%s\"", levelFileName.c_str());

   getGameType()->onLevelLoaded();

   return true;
}


void ServerGame::addClient(GameConnection *theConnection)
{
   // If client has level change or admin permissions, send a list of levels and their types to the connecting client
   if(theConnection->isLevelChanger() || theConnection->isAdmin())
      theConnection->sendLevelList();

   // If we're shutting down, display a notice to the user
   if(mShuttingDown)
      theConnection->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000, mShutdownOriginator->getClientName(), 
                                         "Sorry -- server shutting down", false);

   if(mGameType.isValid())
      mGameType->serverAddClient(theConnection);

   mPlayerCount++;

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(GameConnection *theConnection)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(theConnection);
   mPlayerCount--;

   if(mDedicated)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);
}


extern void shutdownBitfighter();      // defined in main.cpp

// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   if(mShuttingDown && (mShutdownTimer.update(timeDelta) || GameConnection::onlyClientIs(mShutdownOriginator)))
   {
      shutdownBitfighter();
      return;
   }

   // Simulate CPU stutter without impacting gClientGame
   if(gCmdLineSettings.stutter > 0)
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
            for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
            {
               if(walk->mVote == 0 && !walk->isRobot())
               {
                  WaitingToVote = true;
                  walk->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, msg, e, s, i);
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
         for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
         {
            if(walk->mVote == 1)
               voteYes++;
            else if(walk->mVote == 2)
               voteNo++;
            else if(!walk->isRobot())
               voteNothing++;
         }
         bool votePass = voteYes * gIniSettings.voteYesStrength + voteNo * gIniSettings.voteNoStrength + voteNothing * gIniSettings.voteNothingStrength > 0;
         if(votePass)
         {
            GameType *gt = getGameType();
            mVoteTimer = 0;
            switch(mVoteType)
            {
            case 0:
               mNextLevel = mVoteNumber;
               if(gt)
                  gt->gameOverManGameOver();
               break;
            case 1:
               if(gt)
               {
                  gt->extendGameTime(mVoteNumber);                           // Increase "official time"
                  gt->s2cSetTimeRemaining(gt->getRemainingGameTimeInMs());   // Broadcast time to clients
               }
               break;
            case 2:
               if(gt)
               {
                  gt->extendGameTime(S32(mVoteNumber - gt->getRemainingGameTimeInMs()));
                  gt->s2cSetTimeRemaining(gt->getRemainingGameTimeInMs());    // Broadcast time to clients
               }
               break;
            case 3:
               if(gt)
               {
                  gt->setWinningScore(mVoteNumber);
                  gt->s2cChangeScoreToWin(mVoteNumber, mVoteClientName);    // Broadcast score to clients
               }
               break;
            case 4:
               if(gt)
               {
                  for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
                     if(walk->getClientName() == mVoteClientName)
                        gt->changeClientTeam(walk, mVoteNumber);
               }
               break;
            case 5:
               if(gt)
               {
                  for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
                     if(walk->getClientName() == mVoteClientName)
                        gt->changeClientTeam(walk, mVoteNumber);
               }
               break;
            }
         }
         Vector<StringTableEntry> e;
         Vector<StringPtr> s;
         Vector<S32> i;
         i.push_back(voteYes);
         i.push_back(voteNo);
         i.push_back(voteNothing);
         e.push_back(votePass ? "Pass" : "Fail");
         for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
         {
            walk->s2cDisplayMessageESI(GameConnection::ColorAqua, SFXNone, "Vote %e0 - %i0 yes, %i1 no, %i2 not voted", e, s, i);
            if(!votePass && walk->getClientName() == mVoteClientName)
               walk->mVoteTime = gIniSettings.voteRetryLength * 1000;
         }
      }
   }

   // If there are no players on the server, we can enter "suspended animation" mode, but not during the first half-second of hosting.
   // This will prevent locally hosted game from immediately suspending for a frame, giving the local client a chance to 
   // connect.  A little hacky, but works!
   if(mPlayerCount == 0 && !mGameSuspended && mCurrentTime != 0)
      suspendGame();
   else if( mGameSuspended && ((mPlayerCount > 0 && !mSuspendor) || mPlayerCount > 1) )
      unsuspendGame(false);

   if(timeDelta > 2000)   // prevents timeDelta from going too high, usually when after the server was frozen.
      timeDelta = 100;

   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();
   checkConnectionToMaster(timeDelta);    // Connect to master server if not connected

   
   mNetInterface->checkBanlistTimeouts(timeDelta);    // Unban players who's bans have expired

   // Periodically update our status on the master, so they know what we're doing...
   if(mMasterUpdateTimer.update(timeDelta))
   {
      MasterServerConnection *masterConn = gServerGame->getConnectionToMaster();
      static StringTableEntry prevCurrentLevelName;   // Using static, so it holds the value when it comes back here.
      static StringTableEntry prevCurrentLevelType;
      static S32 prevRobotCount;
      static U32 prevPlayerCount;
      if(masterConn && masterConn->isEstablished())
      {
         // Only update if something is different.
         if(prevCurrentLevelName != getCurrentLevelName() || prevCurrentLevelType != getCurrentLevelType() || prevRobotCount != getRobotCount() || prevPlayerCount != mPlayerCount)
         {
            //logprintf("UPDATE");
            prevCurrentLevelName = getCurrentLevelName();
            prevCurrentLevelType = getCurrentLevelType();
            prevRobotCount = getRobotCount();
            prevPlayerCount = mPlayerCount;
            masterConn->updateServerStatus(getCurrentLevelName(), getCurrentLevelType(), getRobotCount(), mPlayerCount, mSettings->getMaxPlayers(), mInfoFlags);
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

   for(GameConnection *walk = GameConnection::getClientList(); walk ; walk = walk->getNextClient())
   {
      if(! walk->isRobot())
      {
         walk->addToTimeCredit(timeDelta);
         walk->updateAuthenticationTimer(timeDelta);
         if(walk->mVoteTime <= timeDelta)
            walk->mVoteTime = 0;
         else
            walk->mVoteTime -= timeDelta;
      }
   }

   // Compute new world extents -- these might change if a ship flies far away, for example...
   // In practice, we could probably just set it and forget it when we load a level.
   // Compute it here to save recomputing it for every robot and other method that relies on it.
   computeWorldObjectExtents();

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
   {
      mGameType->idle(GameObject::ServerIdleMainLoop, timeDelta);
   }

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
      SoundSystem::processAudio(gIniSettings.alertsVolLevel);
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset(LevelSwitchTime);
}


S32 ServerGame::addUploadedLevelInfo(const char *filename, LevelInfo &info)
{
   if(info.levelName == "")            // Make sure we have something in the name field
      info.levelName = filename;

   info.levelFileName = filename; 

   // Check if we already have this one
   for(S32 i = 0; i < mLevelInfos.size(); i++)
      if(mLevelInfos[i].levelFileName == info.levelFileName)
         return i;

   // We don't... so add it!
   mLevelInfos.push_back(info);

   // Let levelChangers know about the new level
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
      if(walk->isLevelChanger())
         walk->s2cAddLevel(info.levelName, info.levelType);

   return mLevelInfos.size() - 1;
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

// Generic color function, works in most cases (static method)
const Color *Game::getBasicTeamColor(const Game *game, S32 teamId)
{
   //TNLAssert(teamId < game->getTeamCount() || teamId < Item::TEAM_HOSTILE, "Invalid team id!");

   if(teamId == TEAM_NEUTRAL)
      return &gNeutralTeamColor;
   else if(teamId == TEAM_HOSTILE)
      return &gHostileTeamColor;
   else if((U32)teamId < (U32)game->getTeamCount())
      return game->getTeam(teamId)->getColor();
   else
      return &Colors::magenta;  // a level can make team number out of range, throw in some rarely used color to let user know an object is out of range team number
}


};


