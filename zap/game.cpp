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
#include "engineeredObjects.h"    // For EngineerModuleDeployer
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
#include "sparkManager.h"
#include "GeomUtils.h"
#include "luaLevelGenerator.h"
#include "robot.h"
#include "shipItems.h"           // For moduleInfos
#include "stringUtils.h"

#include "IniFile.h"             // For CIniFile def

#include "UIQueryServers.h"
#include "UIErrorMessage.h"

#include "BotNavMeshZone.h"      // For zone clearing code
#include "ScreenInfo.h"
#include "Joystick.h"

#include "UIGame.h"
#include "UIGameParameters.h"
#include "UIEditor.h"
#include "UINameEntry.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"

#include "md5wrapper.h"

#include "SDL/SDL_opengl.h"

#include <boost/shared_ptr.hpp>
#include <sys/stat.h>
#include <cmath>


#include "soccerGame.h"


using namespace TNL;

namespace Zap
{
bool showDebugBots = false;


// Global Game objects
ServerGame *gServerGame = NULL;

ClientGame *gClientGame = NULL;
ClientGame *gClientGame1 = NULL;
ClientGame *gClientGame2 = NULL;

extern ScreenInfo gScreenInfo;

static Vector<DatabaseObject *> fillVector2;

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// Constructor
Game::Game(const Address &theBindAddress) : mGameObjDatabase(new GridDatabase())     //? was without new
{
   mUIManager = new UIManager(this);                  // gets deleted in destructor

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
   delete mUIManager;
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
   else if(teamIndex == -2)
      return StringTableEntry("Hostile");
   else if(teamIndex == -1)
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

      // Kind of hacky, but if we encounter a FlagItem in a Nexus game, we'll convert it to a NexusFlag item.  This seems to make more sense.
      // This will work so long as FlagItem and HuntersFlagItem share a common attribute list.
      if(!stricmp(argv[0], "FlagItem") && !mGameType.isNull() && mGameType->getGameType() == GameType::NexusGame)
         strcpy(obj, "HuntersFlagItem");
      // Also, while we're here, we'll add advanced support for the NexusFlagItem, which HuntersFlagItem will eventually be renamed to...
      else if(!stricmp(argv[0], "NexusFlagItem"))
         strcpy(obj, "HuntersFlagItem");
      else
      {
         strncpy(obj, argv[0], LevelLoader::MaxArgLen);
         obj[LevelLoader::MaxArgLen] = '\0';
      }

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

         // Mark the item as being a ghost (client copy of a server obect) that the object will not trigger server-side tests
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

   for(S32 i = 0; i < argc; i++)
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
      //if(gMasterAddress == Address())     // Check for a valid address
      //   return;
      if(gMasterAddress.size() == 0)
         return;

      if(mNextMasterTryTime < timeDelta && mReadyToConnectToMaster)
      {
         if(mHaveTriedToConnectToMaster)
         {
            gMasterAddress.push_back(string(gMasterAddress[0]));  // Try all the address in the list, one at a time..
            gMasterAddress.erase(0);
         }
         mHaveTriedToConnectToMaster = true;
         logprintf(LogConsumer::LogConnection, "%s connecting to master [%s]", isServer() ? "Server" : "Client", 
                    gMasterAddress[0].c_str());

         Address addr(gMasterAddress[0].c_str());
         if(addr.isValid())
         {
            mConnectionToMaster = new MasterServerConnection(isServer());
            mConnectionToMaster->connect(mNetInterface, addr);
         }

         mNextMasterTryTime = GameConnection::MASTER_SERVER_FAILURE_RETRY;     // 10 secs, just in case this attempt fails
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
void Game::deleteObjects(BITMASK typeMask)
{
   fillVector.clear();
   mGameObjDatabase->findObjects(typeMask, fillVector);
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GameObject *obj = dynamic_cast<GameObject *>(fillVector[i]);
      obj->deleteObject(0);
   }
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
      if(fillVector[i]->getObjectTypeMask() != UnknownType)
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
   mGameObjDatabase->findObjects(BarrierType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      theRect.unionRect(fillVector[i]->getExtent());

   return theRect;
}


Point Game::computePlayerVisArea(Ship *ship) const
{
   F32 fraction = ship->getSensorZoomFraction();

   Point regVis(PLAYER_VISUAL_DISTANCE_HORIZONTAL, PLAYER_VISUAL_DISTANCE_VERTICAL);
   Point sensVis(PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL, PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL);

   if(ship->isModuleActive(ModuleSensor))
      return regVis + (sensVis - regVis) * fraction;
   else
      return sensVis + (regVis - sensVis) * fraction;
}


////////////////////////////////////////
////////////////////////////////////////

extern string gHostName;
extern string gHostDescr;

// Constructor
ServerGame::ServerGame(const Address &theBindAddress, U32 maxPlayers, const char *hostName, bool testMode) : Game(theBindAddress)
{
   mVoteTimer = 0;
   mNextLevel = NEXT_LEVEL;
   mPlayerCount = 0;
   mMaxPlayers = maxPlayers;
   mHostName = gHostName;
   mHostDescr = gHostDescr;
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
      delete dynamic_cast<Object *>(fillVector[i]); // dynamic_cast might be needed to avoid errors.
   }
   mTeams.resize(0);
}


void ServerGame::cleanUp()
{
   fillVector.clear();
   mDatabaseForBotZones.findObjects(fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      delete dynamic_cast<Object *>(fillVector[i]);
   Game::cleanUp();
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
      client->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Can't start vote when there is pending vote.");
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
   {
      walk->mVote = 0;
   }
   client->mVote = 1;
   client->s2cDisplayMessage(GameConnection::ColorAqua, SFXNone, "Vote started, waiting for others to vote.");
   return true;
}

void ServerGame::voteClient(GameConnection *client, bool voteYes)
{
   if(mVoteTimer == 0)
      client->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Nothing to vote");
   else if(client->mVote == (voteYes ? 1 : 2))
      client->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Already voted");
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


// This gets called when you first load the host menu
void ServerGame::buildLevelList(Vector<string> &levelList)
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


void ServerGame::loadNextLevel()
{

   string path = ConfigDirectories::findLevelFile(string(mLevelInfos[mLevelLoadIndex].levelFileName.getString()));
   FILE *f = fopen(path.c_str(), "rb");
   if(f)
   {
      char data[1024*4];  // 4 kb should be big enough to fit all parameters at the beginning of level, don't need to read everything.
      S32 size = (S32)fread(data, 1, sizeof(data), f);
      fclose(f);
      LevelInfo info = getLevelInfo(data, size);
      if(info.levelName == "")
         info.levelName = mLevelInfos[mLevelLoadIndex].levelFileName;
      mLevelInfos[mLevelLoadIndex].setInfo(info.levelName, info.levelType, info.maxRecPlayers, info.maxRecPlayers);
      mLevelLoadIndex++;
   }
   else
   {
      logprintf(LogConsumer::LogWarning, "Could not load level %s.  Skipping...", mLevelInfos[mLevelLoadIndex].levelFileName.getString());
      mLevelInfos.erase(mLevelLoadIndex);
   }


/*
   // Here we cycle through all the levels, reading them in, and grabbing their names for the level list
   // Seems kind of wasteful...  could we quit after we found the name? (probably not easily, and we get the benefit of learning early on which levels are b0rked)
   // How about storing them in memory for rapid recall? People sometimes load hundreds of levels, so it's probably not feasible.
   if(mLevelLoadIndex < mLevelInfos.size())
   {
      string levelName = mLevelInfos[mLevelLoadIndex].levelFileName.getString();

      if(loadLevel(levelName))    // loadLevel returns true if the load was successful
      {
         string lname = trim(getGameType()->mLevelName.getString());
         StringTableEntry name;
         if(lname == "")
            name = mLevelInfos[mLevelLoadIndex].levelFileName;      // Use filename if level name is blank
         else
            name = lname.c_str();

         StringTableEntry type(getGameType()->getGameTypeString());

         // Save some key level parameters
         mLevelInfos[mLevelLoadIndex].setInfo(name, type, getGameType()->minRecPlayers, getGameType()->maxRecPlayers);

         // We got what we need, so get rid of this level.  Delete any objects that may exist
         while(mGameObjects.size())    // Don't just to a .clear() because we want to make sure destructors run and memory gets cleared.
            delete mGameObjects[0];    // But would they anyway?  Apparently not...  using .clear() causes all kinds of hell.

         mScopeAlwaysList.clear();

         logprintf(LogConsumer::LogLevelLoaded, "Loaded level %s of type %s [%s]", name.getString(), type.getString(), levelName.c_str());
         mLevelLoadIndex++;
      }
      else     // Level could not be loaded -- it's either missing or invalid.  Remove it from our level list.
      {
         logprintf(LogConsumer::LogWarning, "Could not load level %s.  Skipping...", levelName.c_str());
         mLevelInfos.erase(mLevelLoadIndex);
      }
   }*/

   if(mLevelLoadIndex == mLevelInfos.size())
      ServerGame::hostingModePhase = DoneLoadingLevels;
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
      AsteroidSpawn spawn = AsteroidSpawn();

      if(spawn.processArguments(argc, argv, this))
         getGameType()->addAsteroidSpawn(spawn);
   }
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      if(argc >= 2)
      {
         BarrierRec barrier;
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
            getGameType()->addBarrier(barrier, this);
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
         BarrierRec barrier;
         
         if(width)      // BarrierMakerS still width, though we ignore it
            barrier.width = F32(atof(argv[1]));
         else           // PolyWall does not have width specified
            barrier.width = 1;

         for(S32 i = 2 - (width ? 0 : 1); i < argc; i++)
            barrier.verts.push_back(F32(atof(argv[i])) * getGridSize());

         if(barrier.verts.size() > 3)
         {
            barrier.solid = true;
            getGameType()->addBarrier(barrier, this);
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
      BotNavMeshZone::buildBotNavMeshZoneConnections(getBotZoneDatabase());
   }
   else
      // Try and load Bot Zones for this level, set flag if failed
      getGameType()->mBotZoneCreationFailed = !BotNavMeshZone::buildBotMeshZones(this);


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
   resetLevelInfo();

   mObjectsLoaded = 0;

   string filename = ConfigDirectories::findLevelFile(levelFileName);

   cleanUp();
   if(filename == "")
   {
      logprintf("Unable to find level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }

   logprintf("1 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());
   if(loadLevelFromFile(filename.c_str(), false, getGameObjDatabase()))
      mLevelFileHash = md5.getHashFromFile(filename);    // TODO: Combine this with the reading of the file we're doing anyway in initLevelFromFile()
   else
   {
      logprintf("Unable to process level file \"%s\".  Skipping...", levelFileName.c_str());
      return false;
   }
   logprintf("2 server: %d, client %d", gServerGame->getGameObjDatabase()->getObjectCount(),gClientGame->getGameObjDatabase()->getObjectCount());

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
      string name = ConfigDirectories::findLevelGenScript(scriptName);  // Find full name of levelgen script

      if(name == "")
      {
         logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\" in level\"%s\"", 
                                    scriptName.c_str(), levelFileName.c_str());
         return false;
      }

      // The script file will be the first argument, subsequent args will be passed on to the script.
      // Now we've crammed all our action into the constructor... is this ok design?
      LuaLevelGenerator levelgen = LuaLevelGenerator(name, getGameType()->getScriptArgs(), getGridSize(), getGameObjDatabase(), this, gConsole);
   }

   // Script specified in INI globalLevelLoadScript
   if(gIniSettings.globalLevelScript != "")
   {
      string name = ConfigDirectories::findLevelGenScript(gIniSettings.globalLevelScript);  // Find full name of levelgen script

      if(name == "")
      {
         logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\" in globalLevelScript", 
                                    getGameType()->getScriptName().c_str(), levelFileName.c_str());
         return false;
      }

      // The script file will be the first argument, subsequent args will be passed on to the script.
      // Now we've crammed all our action into the constructor... is this ok design?
      LuaLevelGenerator levelgen = LuaLevelGenerator(name, getGameType()->getScriptArgs(), getGridSize(), getGameObjDatabase(), this, gConsole);
   }

   //  Check after script, script might add Teams
   if(getGameType()->makeSureTeamCountIsNotZero())
      logprintf(LogConsumer::LogWarning, "Warning: Missing Team in level \"%s\"", levelFileName.c_str());

   getGameType()->onLevelLoaded();

   return true;
}


extern bool gDedicatedServer;

void ServerGame::addClient(GameConnection *theConnection)
{
   // Send our list of levels and their types to the connecting client
   for(S32 i = 0; i < mLevelInfos.size(); i++)
      theConnection->s2cAddLevel(mLevelInfos[i].levelName, mLevelInfos[i].levelType);

   // If we're shutting down, display a notice to the user
   if(mShuttingDown)
      theConnection->s2cInitiateShutdown(mShutdownTimer.getCurrent() / 1000, mShutdownOriginator->getClientName(), 
                                         "Sorry -- server shutting down", false);

   if(mGameType.isValid())
      mGameType->serverAddClient(theConnection);

   mPlayerCount++;

   if(gDedicatedServer)
      SoundSystem::playSoundEffect(SFXPlayerJoined, 1);
}


void ServerGame::removeClient(GameConnection *theConnection)
{
   if(mGameType.isValid())
      mGameType->serverRemoveClient(theConnection);
   mPlayerCount--;

   if(gDedicatedServer)
      SoundSystem::playSoundEffect(SFXPlayerLeft, 1);
}


// Top-level idle loop for server, runs only on the server by definition
void ServerGame::idle(U32 timeDelta)
{
   if( mShuttingDown && (mShutdownTimer.update(timeDelta) || GameConnection::onlyClientIs(mShutdownOriginator)) )
   {
      endGame();
      return;
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
            masterConn->updateServerStatus(getCurrentLevelName(), getCurrentLevelType(), getRobotCount(), mPlayerCount, mMaxPlayers, mInfoFlags);
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
      if(fillVector2[i]->getObjectTypeMask() & DeletedType)
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
   SoundSystem::processAudio();
}


void ServerGame::gameEnded()
{
   mLevelSwitchTimer.reset(LevelSwitchTime);
}

//extern ConfigDirectories gConfigDirs;

S32 ServerGame::addLevelInfo(const char *filename, LevelInfo &info)
{
   if(info.levelName == StringTableEntry(""))
      info.levelName = filename;

   info.levelFileName = filename; //strictjoindir(gConfigDirs.levelDir, filename).c_str();

   for(S32 i=0; i<mLevelInfos.size(); i++)
   {
      if(mLevelInfos[i].levelFileName == info.levelFileName)
         return i;
   }

   mLevelInfos.push_back(info);
   for(GameConnection *walk = GameConnection::getClientList(); walk; walk = walk->getNextClient())
   {
      walk->s2cAddLevel(info.levelName, info.levelType);
   }
   return mLevelInfos.size() - 1;
}


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------


// Constructor
ClientGame::ClientGame(const Address &bindAddress) : Game(bindAddress)
{
   mGameUserInterface = new GameUserInterface(this);
   mUserInterfaceData = new UserInterfaceData();
   mInCommanderMap = false;
   mCommanderZoomDelta = 0;

   // Create some random stars
   for(U32 i = 0; i < NumStars; i++)
   {
      mStars[i].x = TNL::Random::readF();      // Between 0 and 1
      mStars[i].y = TNL::Random::readF();
   }

   // //Create some random hexagons
   //for(U32 i = 0; i < NumStars; i++)
   //{
   //   F32 x = TNL::Random::readF();
   //   F32 y = TNL::Random::readF();
   //   F32 ang = TNL::Random::readF() * Float2Pi;
   //   F32 size = TNL::Random::readF() * .1;


   //   for(S32 j = 0; j < 6; j++)
   //   {
   //      mStars[i * 6 + j].x = x + sin(ang + Float2Pi / 6 * j) * size;      // Between 0 and 1
   //      mStars[i * 6 + j].y = y + cos(ang + Float2Pi / 6 * j) * size;
   //   }
   //}


   mScreenSaverTimer.reset(59 * 1000);         // Fire screen saver supression every 59 seconds
}

ClientGame::~ClientGame()
{
   cleanUp();
   delete mUserInterfaceData;
   delete mGameUserInterface;
}

bool ClientGame::hasValidControlObject()
{
   return mConnectionToServer.isValid() && mConnectionToServer->getControlObject();
}

bool ClientGame::isConnectedToServer()
{
   return mConnectionToServer.isValid() && mConnectionToServer->getConnectionState() == NetConnection::Connected;
}

GameConnection *ClientGame::getConnectionToServer()
{
   return mConnectionToServer;
}

void ClientGame::setConnectionToServer(GameConnection *theConnection)
{
   TNLAssert(theConnection, "Passing null connection.  Bah!");
   TNLAssert(mConnectionToServer.isNull(), "Error, a connection already exists here.");
   mConnectionToServer = theConnection;
   theConnection->mClientGame = this;
}


extern bool gShowAimVector;

static void joystickUpdateMove(Move *theMove)
{
   // One of each of left/right axis and up/down axis should be 0 by this point
   // but let's guarantee it..   why?
   theMove->x = Joystick::JoystickInputData[MoveAxesRight].value - Joystick::JoystickInputData[MoveAxesLeft].value;
   theMove->y = Joystick::JoystickInputData[MoveAxesDown].value - Joystick::JoystickInputData[MoveAxesUp].value;

   //logprintf(
   //      "Joystick axis values. Move: Left: %f, Right: %f, Up: %f, Down: %f\nShoot: Left: %f, Right: %f, Up: %f, Down: %f ",
   //      Joystick::JoystickInputData[MoveAxesLeft].value, Joystick::JoystickInputData[MoveAxesRight].value,
   //      Joystick::JoystickInputData[MoveAxesUp].value, Joystick::JoystickInputData[MoveAxesDown].value,
   //      Joystick::JoystickInputData[ShootAxesLeft].value, Joystick::JoystickInputData[ShootAxesRight].value,
   //      Joystick::JoystickInputData[ShootAxesUp].value, Joystick::JoystickInputData[ShootAxesDown].value
   //      );

   //logprintf(
   //         "Move values. Move: Left: %f, Right: %f, Up: %f, Down: %f",
   //         theMove->left, theMove->right,
   //         theMove->up, theMove->down
   //         );


   // Goofball implementation of enableExperimentalAimMode here replicates old behavior when setting is disabled

   //logprintf("XY from shoot axes. x: %f, y: %f", x, y);

   Point p(Joystick::JoystickInputData[ShootAxesRight].value - Joystick::JoystickInputData[ShootAxesLeft].value, Joystick::JoystickInputData[ShootAxesDown].value - Joystick::JoystickInputData[ShootAxesUp].value);
   F32 plen = p.len();

   F32 maxplen = max(fabs(p.x), fabs(p.y));

   F32 fact = gIniSettings.enableExperimentalAimMode ? maxplen : plen;

   if(fact > (gIniSettings.enableExperimentalAimMode ? 0.95 : 0.50))    // It requires a large movement to actually fire...
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = true;
      gShowAimVector = true;
   }
   else if(fact > 0.25)   // ...but you can change aim with a smaller one
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = false;
      gShowAimVector = true;
   }
   else
   {
      theMove->fire = false;
      gShowAimVector = false;
   }
}

U32 prevTimeDelta = 0;

void ClientGame::idle(U32 timeDelta)
{
   mCurrentTime += timeDelta;
   mNetInterface->checkIncomingPackets();

   checkConnectionToMaster(timeDelta);   // If no current connection to master, create (or recreate) one

   if(isSuspended())
   {
      mNetInterface->processConnections();
      SoundSystem::processAudio();                        // Process sound effects (SFX)
      return;
   }

   computeWorldObjectExtents();

   // Only update at most MaxMoveTime milliseconds
   if(timeDelta > Move::MaxMoveTime)
      timeDelta = Move::MaxMoveTime;

   if(!mInCommanderMap && mCommanderZoomDelta != 0)                        // Zooming into normal view
   {
      if(timeDelta > mCommanderZoomDelta)
         mCommanderZoomDelta = 0;
      else
         mCommanderZoomDelta -= timeDelta;

      mGameUserInterface->onMouseMoved();     // Keep ship pointed towards mouse
   }
   else if(mInCommanderMap && mCommanderZoomDelta != CommanderMapZoomTime)    // Zooming out to commander's map
   {
      mCommanderZoomDelta += timeDelta;

      if(mCommanderZoomDelta > CommanderMapZoomTime)
         mCommanderZoomDelta = CommanderMapZoomTime;

      mGameUserInterface->onMouseMoved();  // Keep ship pointed towards mouse
   }
   // else we're not zooming in or out, which is most of the time


   Move *theMove = mGameUserInterface->getCurrentMove();       // Get move from keyboard input

   // Overwrite theMove if we're using joystick (also does some other essential joystick stuff)
   // We'll also run this while in the menus so if we enter keyboard mode accidentally, it won't
   // kill the joystick.  The design of combining joystick input and move updating really sucks.
   if(getIniSettings()->inputMode == InputModeJoystick || UserInterface::current == getUIManager()->getOptionsMenuUserInterface())
      joystickUpdateMove(theMove);


   theMove->time = timeDelta + prevTimeDelta;
   if(theMove->time > Move::MaxMoveTime) 
      theMove->time = Move::MaxMoveTime;

   if(mConnectionToServer.isValid())
   {
      // Disable controls if we are going too fast (usually by being blasted around by a GoFast or mine or whatever)
      GameObject *controlObject = mConnectionToServer->getControlObject();
      Ship *ship = dynamic_cast<Ship *>(controlObject);

     // Don't saturate server with moves...
     if(theMove->time >= 6)     // Why 6?  Can this be related to some other factor?
     { 
         // Limited MaxPendingMoves only allows sending a few moves at a time. changing MaxPendingMoves may break compatibility with old version server/client.
         // If running at 1000 FPS and 300 ping it will try to add more then MaxPendingMoves, losing control horribly.
         // Without the unlimited shield fix, the ship would also go super slow motion with over the limit MaxPendingMoves.
         // With 100 FPS limit, time is never less then 10 milliseconds. (1000 / millisecs = FPS), may be changed in .INI [Settings] MinClientDelay
         mConnectionToServer->addPendingMove(theMove);
         prevTimeDelta = 0;
      }
      else
         prevTimeDelta += timeDelta;
     
      theMove->time = timeDelta;
      theMove->prepare();           // Pack and unpack the move for consistent rounding errors

      fillVector2.clear();  // need to have our own local fillVector
      mGameObjDatabase->findObjects(fillVector2);

      for(S32 i = 0; i < fillVector2.size(); i++)
      {
         if(fillVector2[i]->getObjectTypeMask() & DeletedType)
            continue;

         GameObject *obj = dynamic_cast<GameObject *>(fillVector2[i]);
         TNLAssert(obj, "Bad cast!");

         if(obj == controlObject)
         {
            obj->setCurrentMove(*theMove);
            obj->idle(GameObject::ClientIdleControlMain);  // on client, object is our control object
         }
         else
         {
            Move m = obj->getCurrentMove();
            m.time = timeDelta;
            obj->setCurrentMove(m);
            obj->idle(GameObject::ClientIdleMainRemote);    // on client, object is not our control object
         }
      }
      if(mGameType)
      {
         mGameType->idle(GameObject::ClientIdleMainRemote, timeDelta);
      }

      if(controlObject)
         SoundSystem::setListenerParams(controlObject->getRenderPos(), controlObject->getRenderVel());
   }

   processDeleteList(timeDelta);                // Delete any objects marked for deletion
   FXManager::tick((F32)timeDelta * 0.001f);    // Processes sparks and teleporter effects
   SoundSystem::processAudio();                        // Process sound effects (SFX)

   mNetInterface->processConnections();         // Here we can pass on our updated ship info to the server

   if(mScreenSaverTimer.update(timeDelta))
   {
      supressScreensaver();
      mScreenSaverTimer.reset();
   }
}


void ClientGame::gotServerListFromMaster(const Vector<IPAddress> &serverList)
{
   getUIManager()->getQueryServersUserInterface()->gotServerListFromMaster(serverList);
}


void ClientGame::gotChatMessage(const char *playerNick, const char *message, bool isPrivate, bool isSystem)
{
   getUIManager()->getChatUserInterface()->newMessage(playerNick, message, isPrivate, isSystem);
}


void ClientGame::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   getUIManager()->getChatUserInterface()->setPlayersInGlobalChat(playerNicks);
}


void ClientGame::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->getChatUserInterface()->playerJoinedGlobalChat(playerNick);
}


void ClientGame::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   getUIManager()->getChatUserInterface()->playerLeftGlobalChat(playerNick);
}


void ClientGame::connectionToServerRejected()
{
   getUIManager()->getMainMenuUserInterface()->activate();

   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();
   ui->reset();
   ui->setTitle("Connection Terminated");
   ui->setMessage(2, "Lost connection with the server.");
   ui->setMessage(3, "Unable to join game.  Please try again.");
   ui->activate();

   endGame();
}


void ClientGame::setMOTD(const char *motd)
{
   getUIManager()->getMainMenuUserInterface()->setMOTD(motd); 
}


void ClientGame::setNeedToUpgrade(bool needToUpgrade)
{
  getUIManager()->getMainMenuUserInterface()->setNeedToUpgrade(needToUpgrade);
}



void ClientGame::displayMessage(const Color &msgColor, const char *format, ...)
{
   va_list args;
   char message[MAX_CHAT_MSG_LENGTH]; 

   va_start(args, format);
   vsnprintf(message, sizeof(message), format, args); 
   va_end(args);
    
   getUserInterface()->displayMessage(msgColor, message);
}


extern Color gCmdChatColor;

void ClientGame::gotAdminPermissionsReply(bool granted)
{
   static const char *adminPassSuccessMsg = "You've been granted permission to manage players and change levels";
   static const char *adminPassFailureMsg = "Incorrect password: Admin access denied";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(UserInterface::current->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = granted ? adminPassSuccessMsg : adminPassFailureMsg;
   else
      displayMessage(gCmdChatColor, granted ? adminPassSuccessMsg : adminPassFailureMsg);
}


void ClientGame::gotLevelChangePermissionsReply(bool granted)
{
   static const char *levelPassSuccessMsg = "You've been granted permission to change levels";
   static const char *levelPassFailureMsg = "Incorrect password: Level changing permissions denied";

   // Either display the message in the menu subtitle (if the menu is active), or in the message area if not
   if(UserInterface::current->getMenuID() == GameMenuUI)
      getUIManager()->getGameMenuUserInterface()->mMenuSubTitle = granted ? levelPassSuccessMsg : levelPassFailureMsg;
   else
      displayMessage(gCmdChatColor, granted ? levelPassSuccessMsg : levelPassFailureMsg);
}


void ClientGame::displayMessageBox(const StringTableEntry &title, const StringTableEntry &instr, const Vector<StringTableEntry> &message)
{
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();
   ui->setTitle(title.getString());
   ui->setInstr(instr.getString());

   for(S32 i = 0; i < message.size(); i++)
      ui->setMessage(i+1, message[i].getString());      // UIErrorMsgInterface ==> first line = 1

   ui->activate();
}


// Established connection is terminated.  Compare to onConnectTerminate() below.
void ClientGame::onConnectionTerminated(const Address &serverAddress, NetConnection::TerminationReason reason, const char *reasonStr)
{
   if(getUIManager()->cameFrom(EditorUI))
     getUIManager()->reactivateMenu(getUIManager()->getEditorUserInterface());
   else
     getUIManager()->reactivateMenu(getUIManager()->getMainMenuUserInterface());

   unsuspendGame();

   // Display a context-appropriate error message
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   ui->reset();
   ui->setTitle("Connection Terminated");

   switch(reason)
   {
      case NetConnection::ReasonTimedOut:
         ui->setMessage(2, "Your connection timed out.  Please try again later.");
         ui->activate();
         break;

      case NetConnection::ReasonPuzzle:
         ui->setMessage(2, "Unable to connect to the server.  Recieved message:");
         ui->setMessage(3, "Invalid puzzle solution");
         ui->setMessage(5, "Please try a different game server, or try again later.");
         ui->activate();
         break;

      case NetConnection::ReasonKickedByAdmin:
         ui->setMessage(2, "You were kicked off the server by an admin,");
         ui->setMessage(3, "and have been temporarily banned.");
         ui->setMessage(5, "You can try another server, host your own,");
         ui->setMessage(6, "or try the server that kicked you again later.");
         getUIManager()->getNameEntryUserInterface()->activate();
         ui->activate();

         // Add this server to our list of servers not to display for a spell...
         getUIManager()->getQueryServersUserInterface()->addHiddenServer(serverAddress, Platform::getRealMilliseconds() + GameConnection::BanDuration);
         break;

      case NetConnection::ReasonFloodControl:
         ui->setMessage(2, "Your connection was rejected by the server");
         ui->setMessage(3, "because you sent too many connection requests.");
         ui->setMessage(5, "Please try a different game server, or try again later.");
         getUIManager()->getNameEntryUserInterface()->activate();
         ui->activate();
         break;

      case NetConnection::ReasonShutdown:
         ui->setMessage(2, "Remote server shut down.");
         ui->setMessage(4, "Please try a different server,");
         ui->setMessage(5, "or host a game of your own!");
         ui->activate();
         break;

      case NetConnection::ReasonSelfDisconnect:
            // We get this when we terminate our own connection.  Since this is intentional behavior,
            // we don't want to display any message to the user.
         break;

      default:
         ui->setMessage(1, "Unable to connect to the server for reasons unknown.");
         ui->setMessage(3, "Please try a different game server, or try again later.");
         ui->activate();
   }
}


extern ClientInfo gClientInfo;

void ClientGame::onConnectionToMasterTerminated(NetConnection::TerminationReason reason, const char *reasonStr)
{
   ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

   switch(reason)
   {
      case NetConnection::ReasonDuplicateId:
         ui->setMessage(2, "Your connection was rejected by the server");
         ui->setMessage(3, "because you sent a duplicate player id. Player ids are");
         ui->setMessage(4, "generated randomly, and collisions are extremely rare.");
         ui->setMessage(5, "Please restart Bitfighter and try again.  Statistically");
         ui->setMessage(6, "speaking, you should never see this message again!");
         ui->activate();

         if(getConnectionToServer())
            setReadyToConnectToMaster(false);  // New ID might cause Authentication (underline name) problems if connected to game server...
         else
            gClientInfo.id.getRandom();        // Get another ID, if not connected to game server
         break;

      case NetConnection::ReasonBadLogin:
         ui->setMessage(2, "Unable to log you in with the username/password you");
         ui->setMessage(3, "provided. If you have an account, please verify your");
         ui->setMessage(4, "password. Otherwise, you chose a reserved name; please");
         ui->setMessage(5, "try another.");
         ui->setMessage(7, "Please check your credentials and try again.");

         getUIManager()->getNameEntryUserInterface()->activate();
         ui->activate();
         break;

      case NetConnection::ReasonInvalidUsername:
         ui->setMessage(2, "Your connection was rejected by the server because");
         ui->setMessage(3, "you sent an username that contained illegal characters.");
         ui->setMessage(5, "Please try a different name.");

         getUIManager()->getNameEntryUserInterface()->activate();
         ui->activate();
         break;

      case NetConnection::ReasonError:
         ui->setMessage(2, "Unable to connect to the server.  Recieved message:");
         ui->setMessage(3, reasonStr);
         ui->setMessage(5, "Please try a different game server, or try again later.");
         ui->activate();
         break;
   }
}

extern CIniFile gINI;

// This function only gets called while the player is trying to connect to a server.  Connection has not yet been established.
// Compare to onConnectIONTerminated()
void ClientGame::onConnectTerminated(const Address &serverAddress, NetConnection::TerminationReason reason)
{
   if(reason == NetConnection::ReasonNeedServerPassword)
   {
      // We have the wrong password, let's make sure it's not saved
      string serverName = getUIManager()->getQueryServersUserInterface()->getLastSelectedServerName();
      gINI.deleteKey("SavedServerPasswords", serverName);

      ServerPasswordEntryUserInterface *ui = getUIManager()->getServerPasswordEntryUserInterface();
      ui->setConnectServer(serverAddress);
      ui->activate();
   }
   else if(reason == NetConnection::ReasonServerFull)
   {
      getUIManager()->reactivateMenu(getUIManager()->getMainMenuUserInterface());

      // Display a context-appropriate error message
      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();
      ui->reset();
      ui->setTitle("Connection Terminated");

      getUIManager()->getMainMenuUserInterface()->activate();

      ui->setMessage(2, "Could not connect to server");
      ui->setMessage(3, "because server is full.");
      ui->setMessage(5, "Please try a different server, or try again later.");
      ui->activate();
   }
   else if(reason == NetConnection::ReasonKickedByAdmin)
   {
      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

      ui->reset();
      ui->setTitle("Connection Terminated");

      ui->setMessage(2, "You were kicked off the server by an admin,");
      ui->setMessage(3, "and have been temporarily banned.");
      ui->setMessage(5, "You can try another server, host your own,");
      ui->setMessage(6, "or try the server that kicked you again later.");

      getUIManager()->getMainMenuUserInterface()->activate();
      ui->activate();
   }
   else  // Looks like the connection failed for some unknown reason.  Server died?
   {
      getUIManager()->reactivateMenu(getUIManager()->getMainMenuUserInterface());

      ErrorMessageUserInterface *ui = getUIManager()->getErrorMsgUserInterface();

      // Display a context-appropriate error message
      ui->reset();
      ui->setTitle("Connection Terminated");

      getUIManager()->getMainMenuUserInterface()->activate();

      ui->setMessage(2, "Lost connection with the server.");
      ui->setMessage(3, "Unable to join game.  Please try again.");
      ui->activate();
   }
}


string ClientGame::getRequestedServerName()
{
   return getUIManager()->getQueryServersUserInterface()->getLastSelectedServerName();
}


string ClientGame::getServerPassword()
{
   getUIManager()->getServerPasswordEntryUserInterface()->getText();
}


string ClientGame::getHashedServerPassword()
{
   getUIManager()->getServerPasswordEntryUserInterface()->getSaltedHashText();
}


// Fire keyboard event to suppress screen saver
void ClientGame::supressScreensaver()
{

#if defined(TNL_OS_WIN32) && (_WIN32_WINNT > 0x0400)     // Windows only for now, sadly...
   // _WIN32_WINNT is needed in case of compiling for old windows 98 (this code won't work for windows 98)

   // Code from Tom Revell's Caffeine screen saver suppression product

   // Build keypress
   tagKEYBDINPUT keyup;
   keyup.wVk = VK_MENU;     // Some key they GLUT doesn't recognize
   keyup.wScan = NULL;
   keyup.dwFlags = KEYEVENTF_KEYUP;
   keyup.time = NULL;
   keyup.dwExtraInfo = NULL;

   tagINPUT array[1];
   array[0].type = INPUT_KEYBOARD;
   array[0].ki = keyup;
   SendInput(1, array, sizeof(INPUT));
#endif
}


void ClientGame::zoomCommanderMap()
{
   mInCommanderMap = !mInCommanderMap;
   if(mInCommanderMap)
      SoundSystem::playSoundEffect(SFXUICommUp);
   else
      SoundSystem::playSoundEffect(SFXUICommDown);


   GameConnection *conn = getConnectionToServer();

   if(conn)
   {
      if(mInCommanderMap)
         conn->c2sRequestCommanderMap();
      else
         conn->c2sReleaseCommanderMap();
   }
}


// Unused
U32 ClientGame::getPlayerAndRobotCount() 
{ 
   return mGameType ? mGameType->getClientCount() : (U32)PLAYER_COUNT_UNAVAILABLE; 
}


U32 ClientGame::getPlayerCount()
{
   if(!mGameType)
      return (U32)PLAYER_COUNT_UNAVAILABLE;

   U32 players = 0;

   for(S32 i = 0; i < mGameType->getClientCount(); i++)
      if(!mGameType->getClient(i)->isRobot)
         players++;

   return players;
}


const Color *ClientGame::getTeamColor(S32 teamId) const
{
   // In editor: 
   // return Game::getBasicTeamColor(this, teamIndex);
   GameType *gameType = getGameType();   

   if(!gameType)
      return Parent::getTeamColor(teamId);   // Returns white

   return gameType->getTeamColor(teamId);    // return Game::getBasicTeamColor(mGame, teamIndex); by default, overridden by certain gametypes...
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

// Generic color function, works in most cases (static method)
const Color *Game::getBasicTeamColor(const Game *game, S32 teamId)
{
   //TNLAssert(teamId < game->getTeamCount() || teamId < Item::TEAM_HOSTILE, "Invalid team id!");

   if(teamId == Item::TEAM_NEUTRAL)
      return &gNeutralTeamColor;
   else if(teamId == Item::TEAM_HOSTILE)
      return &gHostileTeamColor;
   else if((U32)teamId < (U32)game->getTeamCount())
      return game->getTeam(teamId)->getColor();
   else
      return &Colors::magenta;  // a level can make team number out of range, throw in some rarely used color to let user know an object is out of range team number
}


void ClientGame::drawStars(F32 alphaFrac, Point cameraPos, Point visibleExtent)
{
   const F32 starChunkSize = 1024;        // Smaller numbers = more dense stars
   const F32 starDist = 3500;             // Bigger value = slower moving stars

   Point upperLeft = cameraPos - visibleExtent * 0.5f;   // UL corner of screen in "world" coords
   Point lowerRight = cameraPos + visibleExtent * 0.5f;  // LR corner of screen in "world" coords

   // When zooming out to commander's view, visibleExtent will grow larger and larger.  At some point, drawing all the stars
   // needed to fill the zoomed out screen becomes overwhelming, and bogs the computer down.  So we really need to set some
   // rational limit on where we stop showing stars during the zoom process (recall that stars are hidden when we are done zooming,
   // so this effect should be transparent to the user except at the most extreme of scales, and then, the alternative is slowing 
   // the computer greatly).  Note that 10000 is probably irrationally high.
   if(visibleExtent.x > 10000 || visibleExtent.y > 10000) 
      return;

   upperLeft  *= 1 / starChunkSize;
   lowerRight *= 1 / starChunkSize;

   upperLeft.x = floor(upperLeft.x);      // Round to ints, slightly enlarging the corners to ensure
   upperLeft.y = floor(upperLeft.y);      // the entire screen is "covered" by the bounding box

   lowerRight.x = floor(lowerRight.x) + 0.5f;
   lowerRight.y = floor(lowerRight.y) + 0.5f;

   // Render some stars
   glPointSize( gLineWidth1 );
   glColor3f(0.8f * alphaFrac, 0.8f * alphaFrac, alphaFrac);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, sizeof(Point), &mStars[0]);    // Each star is a pair of floats between 0 and 1

   S32 fx1 = gIniSettings.starsInDistance ? -1 - ((S32) (cameraPos.x / starDist)) : 0;
   S32 fx2 = gIniSettings.starsInDistance ?  1 - ((S32) (cameraPos.x / starDist)) : 0;
   S32 fy1 = gIniSettings.starsInDistance ? -1 - ((S32) (cameraPos.y / starDist)) : 0;
   S32 fy2 = gIniSettings.starsInDistance ?  1 - ((S32) (cameraPos.y / starDist)) : 0;

   glDisableBlendfromLineSmooth;


   for(F32 xPage = upperLeft.x + fx1; xPage < lowerRight.x + fx2; xPage++)
      for(F32 yPage = upperLeft.y + fy1; yPage < lowerRight.y + fy2; yPage++)
      {
         glPushMatrix();
         glScale(starChunkSize);   // Creates points with coords btwn 0 and starChunkSize

         if(gIniSettings.starsInDistance)
            glTranslatef(xPage + (cameraPos.x / starDist), yPage + (cameraPos.y / starDist), 0);
         else
            glTranslatef(xPage, yPage, 0);

         glDrawArrays(GL_POINTS, 0, NumStars);
         
         //glColor3f(.1,.1,.1);
         // for(S32 i = 0; i < 50; i++)
         //   glDrawArrays(GL_LINE_LOOP, i * 6, 6);

         glPopMatrix();
      }

   glEnableBlendfromLineSmooth;

   glDisableClientState(GL_VERTEX_ARRAY);
}


// Should only be called from Editor
boost::shared_ptr<AbstractTeam> ClientGame::getNewTeam() 
{ 
   return boost::shared_ptr<AbstractTeam>(new TeamEditor());
}


S32 QSORT_CALLBACK renderSortCompare(GameObject **a, GameObject **b)
{
   return (*a)->getRenderSortValue() - (*b)->getRenderSortValue();
}


// Called from renderObjectiveArrow() & ship's onMouseMoved() when in commander's map
Point ClientGame::worldToScreenPoint(const Point *point) const
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   GameObject *controlObject = mConnectionToServer->getControlObject();

   Ship *ship = dynamic_cast<Ship *>(controlObject);
   if(!ship)
      return Point(0,0);

   Point position = ship->getRenderPos();    // Ship's location (which will be coords of screen's center)
   
   if(mCommanderZoomDelta)    // In commander's map, or zooming in/out
   {
      F32 zoomFrac = getCommanderZoomFraction();
      Point worldExtents = mWorldExtents.getExtents();
      worldExtents.x *= canvasWidth / F32(canvasWidth - (UserInterface::horizMargin * 2));
      worldExtents.y *= canvasHeight / F32(canvasHeight - (UserInterface::vertMargin * 2));

      F32 aspectRatio = worldExtents.x / worldExtents.y;
      F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);
      if(aspectRatio > screenAspectRatio)
         worldExtents.y *= aspectRatio / screenAspectRatio;
      else
         worldExtents.x *= screenAspectRatio / aspectRatio;

      Point offset = (mWorldExtents.getCenter() - position) * zoomFrac + position;
      Point visSize = computePlayerVisArea(ship) * 2;
      Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

      Point visScale(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y );

      Point ret = (*point - offset) * visScale + Point((gScreenInfo.getGameCanvasWidth() / 2), (gScreenInfo.getGameCanvasHeight() / 2));
      return ret;
   }
   else                       // Normal map view
   {
      Point visExt = computePlayerVisArea(ship);
      Point scaleFactor((gScreenInfo.getGameCanvasWidth() / 2) / visExt.x, (gScreenInfo.getGameCanvasHeight() / 2) / visExt.y);

      Point ret = (*point - position) * scaleFactor + Point((gScreenInfo.getGameCanvasWidth() / 2), (gScreenInfo.getGameCanvasHeight() / 2));
      return ret;
   }
}


void ClientGame::renderSuspended()
{
   glColor3f(1,1,0);
   S32 textHeight = 20;
   S32 textGap = 5;
   S32 ypos = gScreenInfo.getGameCanvasHeight() / 2 - 3 * (textHeight + textGap);

   UserInterface::drawCenteredString(ypos, textHeight, "==> Game is currently suspended, waiting for other players <==");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When another player joins, the game will start automatically.");
   ypos += textHeight + textGap;
   UserInterface::drawCenteredString(ypos, textHeight, "When the game restarts, the level will be reset.");
   ypos += 2 * (textHeight + textGap);
   UserInterface::drawCenteredString(ypos, textHeight, "Press <SPACE> to resume playing now");
}


static Vector<DatabaseObject *> rawRenderObjects;
static Vector<GameObject *> renderObjects;

void ClientGame::renderCommander()
{
   F32 zoomFrac = getCommanderZoomFraction();
   
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   Point worldExtents = (mGameUserInterface->mShowProgressBar ? getGameType()->mViewBoundsWhileLoading : mWorldExtents).getExtents();
   worldExtents.x *= canvasWidth / F32(canvasWidth - (UserInterface::horizMargin * 2));
   worldExtents.y *= canvasHeight / F32(canvasHeight - (UserInterface::vertMargin * 2));

   F32 aspectRatio = worldExtents.x / worldExtents.y;
   F32 screenAspectRatio = F32(canvasWidth) / F32(canvasHeight);
   if(aspectRatio > screenAspectRatio)
      worldExtents.y *= aspectRatio / screenAspectRatio;
   else
      worldExtents.x *= screenAspectRatio / aspectRatio;

   glPushMatrix();

   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship
   
   Point position = u ? u->getRenderPos() : Point(0,0);

   Point visSize = u ? computePlayerVisArea(u) * 2 : worldExtents;
   Point modVisSize = (worldExtents - visSize) * zoomFrac + visSize;

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() / 2.f, gScreenInfo.getGameCanvasHeight() / 2.f, 0);    

   glScalef(canvasWidth / modVisSize.x, canvasHeight / modVisSize.y, 1);

   Point offset = (mWorldExtents.getCenter() - position) * zoomFrac + position;
   glTranslatef(-offset.x, -offset.y, 0);

   if(zoomFrac < 0.95)
      drawStars(1 - zoomFrac, offset, modVisSize);
 

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects.  Note that this no longer captures
   // walls -- those will be rendered separately.
   rawRenderObjects.clear();
   mGameObjDatabase->findObjects(CommandMapVisType, rawRenderObjects);

   // If we're drawing bot zones, add them to our list of render objects
   if(gServerGame && mGameUserInterface->isShowingDebugMeshZones())
      gServerGame->getBotZoneDatabase()->findObjects(0, rawRenderObjects, BotNavMeshZoneTypeNumber);

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));

   if(gServerGame && showDebugBots)
      for(S32 i = 0; i < Robot::robots.size(); i++)
         renderObjects.push_back(Robot::robots[i]);

   if(u)
   {
      // Get info about the current player
      GameType *gt = getGameType();
      S32 playerTeam = -1;

      if(gt)
      {
         playerTeam = u->getTeam();
         Color teamColor = *gt->getTeamColor(playerTeam);

         for(S32 i = 0; i < renderObjects.size(); i++)
         {
            // Render ship visibility range, and that of our teammates
            if(renderObjects[i]->getObjectTypeMask() & (ShipType | RobotType))
            {
               Ship *ship = dynamic_cast<Ship *>(renderObjects[i]);

               // Get team of this object
               S32 ourTeam = ship->getTeam();
               if((ourTeam == playerTeam && getGameType()->isTeamGame()) || ship == u)  // On our team (in team game) || the ship is us
               {
                  Point p = ship->getRenderPos();
                  Point visExt = computePlayerVisArea(ship);

                  glColor(teamColor * zoomFrac * 0.35f);

                  glBegin(GL_POLYGON);
                     glVertex2f(p.x - visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y - visExt.y);
                     glVertex2f(p.x + visExt.x, p.y + visExt.y);
                     glVertex2f(p.x - visExt.x, p.y + visExt.y);
                  glEnd();
               }
            }
         }

         fillVector.clear();
         mGameObjDatabase->findObjects(SpyBugType, fillVector);

         // Render spy bug visibility range second, so ranges appear above ship scanner range
         for(S32 i = 0; i < fillVector.size(); i++)
         {
            SpyBug *sb = dynamic_cast<SpyBug *>(fillVector[i]);

            if(sb->isVisibleToPlayer(playerTeam, getGameType()->mLocalClient ? getGameType()->mLocalClient->name : 
                                                                                 StringTableEntry(""), getGameType()->isTeamGame()))
            {
               const Point &p = sb->getRenderPos();
               Point visExt(gSpyBugRange, gSpyBugRange);
               glColor(teamColor * zoomFrac * 0.45f);     // Slightly different color than that used for ships

               glBegin(GL_POLYGON);
                  glVertex2f(p.x - visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y - visExt.y);
                  glVertex2f(p.x + visExt.x, p.y + visExt.y);
                  glVertex2f(p.x - visExt.x, p.y + visExt.y);
               glEnd();

               glColor(teamColor * 0.8f);     // Draw a marker in the middle
               drawCircle(u->getRenderPos(), 2);
            }
         }
      }
   }

   // Now render the objects themselves
   renderObjects.sort(renderSortCompare);

   // First pass
   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->render(0);

   // Second pass
   Barrier::renderEdges(1);    // Render wall edges

   for(S32 i = 0; i < renderObjects.size(); i++)
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->render(1);

   glPopMatrix();
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// This is a test of a partial map overlay to assist in navigation
// still needs work, and early indications are that it is not
// a beneficial addition to the game.

void ClientGame::renderOverlayMap()
{
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);

   Point position = u->getRenderPos();

   S32 mapWidth = canvasWidth / 4;
   S32 mapHeight = canvasHeight / 4;
   S32 mapX = UserInterface::horizMargin;        // This may need to the the UL corner, rather than the LL one
   S32 mapY = canvasHeight - UserInterface::vertMargin - mapHeight;
   F32 mapScale = 0.1f;

   glBegin(GL_LINE_LOOP);
      glVertex2i(mapX, mapY);
      glVertex2i(mapX, mapY + mapHeight);
      glVertex2i(mapX + mapWidth, mapY + mapHeight);
      glVertex2i(mapX + mapWidth, mapY);
   glEnd();

   glEnable(GL_SCISSOR_BOX);                    // Crop to overlay map display area
   glScissor(mapX, mapY + mapHeight, mapWidth, mapHeight);  // Set cropping window

   glPushMatrix();   // Set scaling and positioning of the overlay

   glTranslatef(mapX + mapWidth / 2.f, mapY + mapHeight / 2.f, 0);          // Move map off to the corner
   glScalef(mapScale, mapScale, 1);                                     // Scale map
   glTranslatef(-position.x, -position.y, 0);                           // Put ship at the center of our overlay map area

   // Render the objects.  Start by putting all command-map-visible objects into renderObjects
   Rect mapBounds(position, position);
   mapBounds.expand(Point(mapWidth * 2, mapHeight * 2));      //TODO: Fix

   rawRenderObjects.clear();
   mGameObjDatabase->findObjects(CommandMapVisType, rawRenderObjects, mapBounds);

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));


   renderObjects.sort(renderSortCompare);

   for(S32 i = 0; i < renderObjects.size(); i++)
      renderObjects[i]->render(0);

   for(S32 i = 0; i < renderObjects.size(); i++)
      // Keep our spy bugs from showing up on enemy commander maps, even if they're known
 //     if(!(renderObjects[i]->getObjectTypeMask() & SpyBugType && playerTeam != renderObjects[i]->getTeam()))
         renderObjects[i]->render(1);

   glPopMatrix();
   glDisable(GL_SCISSOR_BOX);     // Stop cropping
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

// ==> should move render to UIGame?

static Point screenSize, position;


void ClientGame::renderNormal()
{
   if(!hasValidControlObject())
      return;

   GameObject *controlObject = mConnectionToServer->getControlObject();
   Ship *u = dynamic_cast<Ship *>(controlObject);      // This is the local player's ship
   if(!u)
      return;

   position.set(u->getRenderPos());

   glPushMatrix();

   // Put (0,0) at the center of the screen
   glTranslatef(gScreenInfo.getGameCanvasWidth() / 2.f, gScreenInfo.getGameCanvasHeight() / 2.f, 0);       

   Point visExt = computePlayerVisArea(dynamic_cast<Ship *>(u));
   glScalef((gScreenInfo.getGameCanvasWidth()  / 2) / visExt.x, 
            (gScreenInfo.getGameCanvasHeight() / 2) / visExt.y, 1);

   glTranslatef(-position.x, -position.y, 0);

   drawStars(1.0, position, visExt * 2);

   // Render all the objects the player can see
   screenSize.set(visExt);
   Rect extentRect(position - screenSize, position + screenSize);

   rawRenderObjects.clear();
   mGameObjDatabase->findObjects(AllObjectTypes, rawRenderObjects, extentRect);    // Use extent rects to quickly find objects in visual range

   // Normally a big no-no, we'll access the server's bot zones directly if we are running locally so we can visualize them without bogging
   // the game down with the normal process of transmitting zones from server to client.  The result is that we can only see zones on our local
   // server.
   if(gServerGame && mGameUserInterface->isShowingDebugMeshZones())
       gServerGame->getBotZoneDatabase()->findObjects(0, rawRenderObjects, extentRect, BotNavMeshZoneTypeNumber);

   renderObjects.clear();
   for(S32 i = 0; i < rawRenderObjects.size(); i++)
      renderObjects.push_back(dynamic_cast<GameObject *>(rawRenderObjects[i]));

   if(gServerGame && showDebugBots)
      for(S32 i = 0; i < Robot::robots.size(); i++)
         renderObjects.push_back(Robot::robots[i]);


   renderObjects.sort(renderSortCompare);

   // Render in three passes, to ensure some objects are drawn above others
   for(S32 j = -1; j < 2; j++)
   {
      Barrier::renderEdges(j);    // Render wall edges
      for(S32 i = 0; i < renderObjects.size(); i++)
         renderObjects[i]->render(j);
      FXManager::render(j);
   }

   FXTrail::renderTrails();

   Ship *ship = NULL;
   if(mConnectionToServer.isValid())
      ship = dynamic_cast<Ship *>(mConnectionToServer->getControlObject());

   if(ship)
      mGameUserInterface->renderEngineeredItemDeploymentMarker(ship);

   glPopMatrix();

   // Render current ship's energy
   if(ship)
   {
      renderEnergyGuage(ship->mEnergy, Ship::EnergyMax, Ship::EnergyCooldownThreshold);
   }


   //renderOverlayMap();     // Draw a floating overlay map
}


void ClientGame::render()
{
   bool renderObjectsWhileLoading = false;

   if(!renderObjectsWhileLoading && !hasValidControlObject())
      return;

   if(mGameUserInterface->mShowProgressBar)
      renderCommander();
   else if(mGameSuspended)
      renderCommander();
   else if(mCommanderZoomDelta > 0)
      renderCommander();
   else
      renderNormal();
}


////////////////////////////////////////
////////////////////////////////////////

//const Color *EditorGame::getTeamColor(S32 teamIndex) const
//{
//   return Game::getBasicTeamColor(this, teamIndex); 
//}



bool ClientGame::processPseudoItem(S32 argc, const char **argv, const string &levelFileName)
{
   if(!stricmp(argv[0], "Spawn") || !stricmp(argv[0], "FlagSpawn") || !stricmp(argv[0], "AsteroidSpawn"))
   {
      EditorObject *newObject;

      if(!stricmp(argv[0], "Spawn"))
         newObject = new Spawn();
      else if(!stricmp(argv[0], "FlagSpawn"))
         newObject = new FlagSpawn();
      else /* if(!stricmp(argv[0], "AsteroidSpawn")) */
         newObject = new AsteroidSpawn();


      bool validArgs = newObject->processArguments(argc - 1, argv + 1, this);

      if(validArgs)
         newObject->addToEditor(this);
      else
      {
         logprintf(LogConsumer::LogWarning, "Invalid arguments in object \"%s\" in level \"%s\"", argv[0], levelFileName.c_str());
         delete newObject;
      }
   }
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      if(argc >= 2)
      {
         WallItem *wallObject = new WallItem();  
         
         wallObject->setWidth(atoi(argv[1]));      // setWidth handles bounds checking

         wallObject->setDockItem(false);     // TODO: Needed?
         wallObject->initializeEditor();     // Only runs unselectVerts

         Point p;
         for(S32 i = 2; i < argc; i+=2)
         {
            p.set(atof(argv[i]), atof(argv[i+1]));
            p *= getGridSize();
            wallObject->addVert(p);
         }
         
         if(wallObject->getVertCount() >= 2)
         {
            wallObject->addToGame(this, this->getEditorDatabase());
            wallObject->processEndPoints();
            //wallObject->onGeomChanged(); 
         }
         else
            delete wallObject;
      }
   }
   // TODO: Integrate code above with code above!!  EASY!!
   else if(!stricmp(argv[0], "BarrierMakerS") || !stricmp(argv[0], "PolyWall"))
   {
      //if(width)      // BarrierMakerS still width, though we ignore it
      //   barrier.width = F32(atof(argv[1]));
      //else           // PolyWall does not have width specified
      //   barrier.width = 1;

      if(argc >= 2)
      {
         EditorObject *newObject = new PolyWall();  
         
         S32 skipArgs = 0;
         if(!stricmp(argv[0], "BarrierMakerS"))
         {
            logprintf(LogConsumer::LogWarning, "BarrierMakerS has been deprecated.  Please use PolyWall instead.");

            skipArgs = 1;
         }

         newObject->setDockItem(false);     // TODO: Needed?
         newObject->initializeEditor();     // Only runs unselectVerts

         newObject->processArguments(argc - 1 - skipArgs, argv + 1 + skipArgs, this);
         
         if(newObject->getVertCount() >= 2)
         {
            newObject->addToEditor(this);
            newObject->onGeomChanged(); 
         }
         else
            delete newObject;
      }
   }
   else 
      return false;

   return true;
}


};


