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

#include "gameType.h"
#include "EngineeredItem.h"
#include "gameObjectRender.h"
#include "SoundEffect.h"
#include "config.h"
#include "projectile.h"       // For s2cClientJoinedTeam()
#include "playerInfo.h"       // For LuaPlayerInfo constructor  
#include "stringUtils.h"      // For itos
#include "gameStats.h"        // For VersionedGameStats def
#include "version.h"
#include "BanList.h"
#include "IniFile.h"          // For CIniFile


#ifndef ZAP_DEDICATED
#   include "ClientGame.h"
#   include "loadoutHelper.h"
#   include "UIGame.h"
#   include "UIMenus.h"
#   include "UIErrorMessage.h"   
#   include "SDL/SDL_opengl.h"
#endif


#include "../master/database.h"

#include "statistics.h"
#include "masterConnection.h"     // For s2mSendPlayerStatistics, s2mSendGameStatistics


#include "tnlThread.h"
#include <math.h>

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{

#ifdef TNL_OS_MAC_OSX
const S32 GameType::MAX_TEAMS;
#endif

//static Timer mTestTimer(10 * 1000);
//static bool on = true;

// List of valid game types -- these are the "official" names, not the more user-friendly names provided by getGameTypeString
// All names are of the form xxxGameType, and have a corresponding class xxxGame
const char *gGameTypeNames[] = {
   "GameType",                // Generic game type --> Bitmatch
   "CTFGameType",
   "HTFGameType",
   "NexusGameType",
   "RabbitGameType",
   "RetrieveGameType",
   "SoccerGameType",
   "ZoneControlGameType",
   NULL  // Last item must be NULL
};


S32 gDefaultGameTypeIndex = 0;  // What we'll default to if the name provided is invalid or missing... i.e. GameType ==> Bitmatch


////////////////////////////////////////      __              ___           
////////////////////////////////////////     /__  _. ._ _   _  |    ._   _  
////////////////////////////////////////     \_| (_| | | | (/_ | \/ |_) (/_ 
////////////////////////////////////////                         /  |       

TNL_IMPLEMENT_NETOBJECT(GameType);

// Constructor
GameType::GameType(S32 winningScore) : mScoreboardUpdateTimer(1000) , mGameTimer(DefaultGameTime) , mGameTimeUpdateTimer(30000)
{
   mNetFlags.set(Ghostable);
   mBetweenLevels = true;
   mGameOver = false;
   mWinningScore = winningScore;
   mLeadingTeam = -1;
   mLeadingTeamScore = 0;
   mLeadingPlayer = -1;
   mLeadingPlayerScore = 0;
   mSecondLeadingPlayer = -1;
   mSecondLeadingPlayerScore = 0;
   mDigitsNeededToDisplayScore = 1;
   mCanSwitchTeams = true;       // Players can switch right away
   mZoneGlowTimer.setPeriod(mZoneGlowTime);
   mGlowingZoneTeam = -1;        // By default, all zones glow
   mLevelHasLoadoutZone = false;
   mShowAllBots = false;
   mTotalGamePlay = 0;
   mBotZoneCreationFailed = false;

   mMinRecPlayers = 0;
   mMaxRecPlayers = 0;

   mEngineerEnabled = false;
   mBotsAllowed = true;

#ifndef ZAP_DEDICATED
   // I *think* this is only here to provide a default for the editor
   mLevelCredits = gClientGame ? gClientGame->getClientInfo()->getName() : "";     
#endif

   mGame = NULL;
}


// Destructor
GameType::~GameType()
{
   // Do nothing
}


bool GameType::processArguments(S32 argc, const char **argv, Game *game)
{
   if(argc > 0)      // First arg is game length, in minutes
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));

   if(argc > 1)      // Second arg is winning score
      mWinningScore = atoi(argv[1]);

   return true;
}


string GameType::toString() const
{
   return string(getClassName()) + " " + ftos(F32(getTotalGameTime()) / 60, 3) + " " + itos(mWinningScore);
}


// GameType object is the first to be added when a new game starts... therefore, this is a reasonable signifier that a new game is starting up.  I think.
void GameType::addToGame(Game *game, GridDatabase *database)
{
   mGame = game;
   game->setGameType(this);
}


bool GameType::onGhostAdd(GhostConnection *theConnection)
{

   //TNLAssert(!mGame->isServer(), "Should only be client here!");
   // mGame appears to return null here sometimes... why??

#ifndef ZAP_DEDICATED
   addToGame(gClientGame, gClientGame->getGameObjDatabase());
#endif
   return true;
}


#ifndef ZAP_DEDICATED
// Menu items we want to show
const char **GameType::getGameParameterMenuKeys()
{
    static const char *items[] = {
      "Level Name",
      "Level Descr",
      "Level Credits",
      "Levelgen Script",
      "Game Time",
      "Win Score",
      "Grid Size",
      "Min Players",
      "Max Players",
      "Allow Engr",
      "Allow Robots",
      "" };

      return items;
}


// Definitions for those items
boost::shared_ptr<MenuItem> GameType::getMenuItem(const char *key)
{
   if(!strcmp(key, "Level Name"))
   {
      MenuItem *item = new TextEntryMenuItem("Level Name:", mLevelName.getString(), "", "The level's name -- pick a good one!", MAX_GAME_NAME_LEN);   
      item->setFilter(LineEditor::allAsciiFilter);

      return boost::shared_ptr<MenuItem>(item);
   }
   else if(!strcmp(key, "Level Descr"))
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Level Descr:", 
                                                              mLevelDescription.getString(), 
                                                              "", 
                                                              "A brief description of the level",                     
                                                              MAX_GAME_DESCR_LEN));
   else if(!strcmp(key, "Level Credits"))
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Level By:",       
                                                              mLevelCredits.getString(), 
                                                              "", 
                                                              "Who created this level",                                  
                                                              MAX_GAME_DESCR_LEN));
   else if(!strcmp(key, "Levelgen Script"))
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Levelgen Script:", 
                                                              getScriptLine(), 
                                                              "<None>", 
                                                              "Levelgen script & args to be run when level is loaded",  
                                                              255));
   else if(!strcmp(key, "Game Time"))
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItem("Game Time:", getTotalGameTime(), 99*60, "Unlimited", "Time game will last"));
   else if(!strcmp(key, "Win Score"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Score to Win:", getWinningScore(), 1, 1, 99, "points", "", "Game ends when one team gets this score"));
   else if(!strcmp(key, "Grid Size"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Grid Size:",       
                                                             (S32)getGame()->getGridSize(),
                                                             Game::MIN_GRID_SIZE,      // increment
                                                             Game::MIN_GRID_SIZE,      // min val
                                                             Game::MAX_GRID_SIZE,      // max val
                                                             "pixels",                 // units
                                                             "", 
                                                             "\"Magnification factor.\" Larger values lead to larger levels.  Default is 255."));
   else if(!strcmp(key, "Min Players"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Min Players:",       
                                                             mMinRecPlayers,     // value
                                                             1,                  // increment
                                                             0,                  // min val
                                                             MAX_PLAYERS,        // max val
                                                             "players",          // units
                                                             "N/A", 
                                                             "Min. players you would recommend for this level (to help server select the next level)"));
   else if(!strcmp(key, "Max Players"))
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Max Players:",       
                                                             mMaxRecPlayers,     // value
                                                             1,                  // increment
                                                             0,                  // min val
                                                             MAX_PLAYERS,        // max val
                                                             "players",          // units
                                                             "N/A",
                                                             "Max. players you would recommend for this level (to help server select the next level)"));
   else if(!strcmp(key, "Allow Engr"))
      return boost::shared_ptr<MenuItem>(new YesNoMenuItem("Allow Engineer Module:",       
                                                           mEngineerEnabled,
                                                           "Allow players to use the Engineer module?"));
   else if(!strcmp(key, "Allow Robots"))
         return boost::shared_ptr<MenuItem>(new YesNoMenuItem("Allow Robots:",
                                                              mBotsAllowed,
                                                              "Allow players to add robots?"));
   else
      return boost::shared_ptr<MenuItem>();     // NULLish pointer
}


bool GameType::saveMenuItem(const MenuItem *menuItem, const char *key)
{
   if(!strcmp(key, "Level Name"))
      setLevelName(menuItem->getValue());
   else if(!strcmp(key, "Level Descr"))
      setLevelDescription(menuItem->getValue());
   else if(!strcmp(key, "Level Credits"))
      setLevelCredits(menuItem->getValue());
   else if(!strcmp(key, "Levelgen Script"))
      setScript(parseString(menuItem->getValue()));
   else if(!strcmp(key, "Game Time"))
      setGameTime((F32)menuItem->getIntValue());
   else if(!strcmp(key, "Win Score"))
      setWinningScore(menuItem->getIntValue());
   else if(!strcmp(key, "Grid Size"))
      mGame->setGridSize((F32)menuItem->getIntValue());
   else if(!strcmp(key, "Min Players"))
       setMinRecPlayers(menuItem->getIntValue());
   else if(!strcmp(key, "Max Players"))
      setMaxRecPlayers(menuItem->getIntValue());
   else if(!strcmp(key, "Allow Engr"))
      setEngineerEnabled(menuItem->getIntValue());
   else
      return false;

   return true;
}
#endif

bool GameType::processSpecialsParam(const char *param)
{
   if(!stricmp(param, "Engineer"))
      setEngineerEnabled(true);
   else if(!stricmp(param, "NoBots"))
      setBotsAllowed(false);
   else
      return false;

   return true;
}


string GameType::getSpecialsLine()
{
   string specialsLine = "Specials";

   if(isEngineerEnabled())
      specialsLine += " Engineer";

   if(!areBotsAllowed())
      specialsLine += " NoBots";

   return specialsLine;
}


string GameType::getScriptLine() const
{
   string str;

   if(mScriptName == "")
      return "";

   str += mScriptName;
   
   if(mScriptArgs.size() > 0)
      str += " " + concatenate(mScriptArgs);

   return str;
}


void GameType::setScript(const Vector<string> &args)
{
   mScriptName = args.size() > 0 ? args[0] : "";

   mScriptArgs.clear();       // Clear out any args from a previous Script line
   for(S32 i = 1; i < args.size(); i++)
      mScriptArgs.push_back(args[i]);
}


void GameType::printRules()
{
   NetClassRep::initialize();    // We need this to instantiate objects to interrogate them below

   printf("\n\n");
   printf("Bitfighter rules\n");
   printf("================\n\n");
   printf("Projectiles:\n\n");
   for(S32 i = 0; i < WeaponCount; i++)
   {
      printf("Name: %s \n", GameWeapon::weaponInfo[i].name.getString());
      printf("\tEnergy Drain: %d\n", GameWeapon::weaponInfo[i].drainEnergy);
      printf("\tVelocity: %d\n", GameWeapon::weaponInfo[i].projVelocity);
      printf("\tLifespan (ms): %d\n", GameWeapon::weaponInfo[i].projLiveTime);
      printf("\tDamage: %2.2f\n", GameWeapon::weaponInfo[i].damageAmount);
      printf("\tDamage To Self Multiplier: %2.2f\n", GameWeapon::weaponInfo[i].damageSelfMultiplier);
      printf("\tCan Damage Teammate: %s\n", GameWeapon::weaponInfo[i].canDamageTeammate ? "Yes" : "No");
   }

   printf("\n\n");
   printf("Game Types:\n\n");
   for(S32 i = 0; ; i++)     // second arg intentionally blank!
   {
      if(gGameTypeNames[i] == NULL)
         break;

      TNL::Object *theObject = TNL::Object::create(gGameTypeNames[i]);  // Instantiate a gameType object
      GameType *gameType = dynamic_cast<GameType*>(theObject);          // and cast it

      string indTeam;

      if(gameType->canBeIndividualGame() && gameType->canBeTeamGame())
         indTeam = "Individual or Teams";
      else if (gameType->canBeIndividualGame())
         indTeam = "Individual only";
      else if (gameType->canBeTeamGame())
         indTeam = "Team only";
      else
         indTeam = "Configuration Error!";


      printf("Game type: %s [%s]\n", gameType->getGameTypeString(), indTeam.c_str());
      printf("Configure ship: %s", gameType->isSpawnWithLoadoutGame() ? "By respawning (no need for loadout zones)" : "By entering loadout zone");
      printf("\nEvent: Individual Score / Team Score\n");
      printf(  "====================================\n");
      for(S32 j = 0; j < ScoringEventsCount; j++)
      {
         S32 teamScore = gameType->getEventScore(GameType::TeamScore, (ScoringEvent) j, 0);
         S32 indScore = gameType->getEventScore(GameType::IndividualScore, (ScoringEvent) j, 0);

         if(teamScore == naScore && indScore == naScore)    // Skip non-scoring events
            continue;

         string teamScoreStr = (teamScore == naScore) ? "N/A" : itos(teamScore);
         string indScoreStr =  (indScore == naScore)  ? "N/A" : itos(indScore);

         printf("%s: %s / %s\n", getScoringEventDescr((ScoringEvent) j).c_str(), indScoreStr.c_str(), teamScoreStr.c_str() );
      }

      printf("\n\n");
   }
}


// For external access -- delete me if you can!
void printRules()
{
   GameType::printRules();
}


// These are really only used for displaying scoring with the -rules option
string GameType::getScoringEventDescr(ScoringEvent event)
{
   switch(event)
   {
      // General scoring events:
      case KillEnemy:
         return "Kill enemy player";
      case KillSelf:
         return "Kill self";
      case KillTeammate:
         return "Kill teammate";
      case KillEnemyTurret:
         return "Kill enemy turret";
      case KillOwnTurret:
         return "Kill own turret";
      case KilledByAsteroid:
         return "Killed by asteroid";
      case KilledByTurret:
         return "Killed by turret";

      // CTF specific:
      case CaptureFlag:
         return "Touch enemy flag to your flag";
      case ReturnTeamFlag:
         return "Return own flag to goal";

      // ZC specific:
     case  CaptureZone:
         return "Capture zone";
      case UncaptureZone:
         return "Lose captured zone to other team";

      // HTF specific:
      case HoldFlagInZone:
         return "Hold flag in zone for time";
      case RemoveFlagFromEnemyZone:
         return "Remove flag from enemy zone";

      // Rabbit specific:
      case RabbitHoldsFlag:
         return "Hold flag, per second";
      case RabbitKilled:
         return "Kill the rabbit";
      case RabbitKills:
         return "Kill other player if you are rabbit";

      // Nexus specific:
      case ReturnFlagsToNexus:
         return "Return flags to Nexus";

      // Retrieve specific:
      case ReturnFlagToZone:
         return "Return flags to own zone";
      case LostFlag:
         return "Lose captured flag to other team";

      // Soccer specific:
      case ScoreGoalEnemyTeam:
         return "Score a goal against other team";
      case ScoreGoalHostileTeam:
         return "Score a goal against Hostile team";
      case ScoreGoalOwnTeam:
         return "Score a goal against own team";

      // Other:
      default:
         return "Unknown event!";
   }
}


// Will return a valid GameType string -- either what's passed in, or the default if something bogus was specified  (static)
const char *GameType::validateGameType(const char *gtype)
{
   for(S32 i = 0; gGameTypeNames[i]; i++)    // Repeat until we hit NULL
      if(!strcmp(gGameTypeNames[i], gtype))
         return gGameTypeNames[i];

   // If we get to here, no valid game type was specified, so we'll return the default
   return gGameTypeNames[gDefaultGameTypeIndex];
}


void GameType::idle(GameObject::IdleCallPath path, U32 deltaT)
{
   mTotalGamePlay += deltaT;

   if(path != GameObject::ServerIdleMainLoop)    
      idle_client(deltaT);
   else
      idle_server(deltaT);
}


void GameType::idle_server(U32 deltaT)
{
   queryItemsOfInterest();

   bool needsScoreboardUpdate = mScoreboardUpdateTimer.update(deltaT);

   if(needsScoreboardUpdate)
      mScoreboardUpdateTimer.reset();

   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      if(needsScoreboardUpdate)
      {
         if(conn->isEstablished())    // robots don't have connection
         {
            clientInfo->setPing((U32) conn->getRoundTripTime());

            if(clientInfo->getPing() > MaxPing || conn->lostContact())
               clientInfo->setPing(MaxPing);
         }

         // Send scores/pings to client if game is over, or client has requested them
         if(mGameOver || conn->wantsScoreboardUpdates())
            updateClientScoreboard(clientInfo);
      }


      // Respawn dead players
      if(conn->respawnTimer.update(deltaT))           // Need to respawn?
         spawnShip(clientInfo);                       
                                                         
      if(conn->mSwitchTimer.getCurrent())             // Are we still counting down until the player can switch?
         if(conn->mSwitchTimer.update(deltaT))        // Has the time run out?
         {                                            
            NetObject::setRPCDestConnection(conn);    // Limit who gets this message
            s2cCanSwitchTeams(true);                  // If so, let the client know they can switch again
            NetObject::setRPCDestConnection(NULL);
         }

      if(!clientInfo->isRobot())
         conn->addTimeSinceLastMove(deltaT);             // Increment timer       
   }


   // Periodically send time-remaining updates to the clients unless the game timer is at zero
   if(mGameTimeUpdateTimer.update(deltaT) && mGameTimer.getCurrent() != 0)
   {
      mGameTimeUpdateTimer.reset();
      s2cSetTimeRemaining(mGameTimer.getCurrent());
   }


   // Spawn items (that are not ships)
   for(S32 i = 0; i < mItemSpawnPoints.size(); i++)
      if(mItemSpawnPoints[i]->updateTimer(deltaT))
      {
         mItemSpawnPoints[i]->spawn(mGame, mItemSpawnPoints[i]->getPos());    // Spawn item
         mItemSpawnPoints[i]->resetTimer();                                   // Reset the spawn timer
      }

   //if(mTestTimer.update(deltaT))
   //{
   //   Worm *worm = new Worm();
   //   F32 ang = TNL::Random::readF() * Float2Pi;
   //   worm->setPosAng(Point(0,0), ang);
   //   worm->addToGame(gServerGame);
   //   mTestTimer.reset(10000);
   //}
   //{
   //   on = !on;

   //   if(!on)
   //   {
   //      Vector<F32> v;
   //      s2cAddBarriers(v, 0, false);
   //   }
   //   else
   //   {
   //      for(S32 i = 0; i < mBarriers.size(); i++)
   //         s2cAddBarriers(mBarriers[i].verts, mBarriers[i].width, mBarriers[i].solid);
   //   }

   //   mTestTimer.reset();
   //}
  

   // Process any pending Robot events
   Robot::getEventManager().update();

   // If game time has expired... game is over, man, it's over
   if(mGameTimer.update(deltaT))
      gameOverManGameOver();
}


void GameType::idle_client(U32 deltaT)
{
   mGameTimer.update(deltaT);
   mZoneGlowTimer.update(deltaT);
}


//// Sorts teams by player counts, high to low
//S32 QSORT_CALLBACK teamSizeSort(Team *a, Team *b)
//{
//   return (b->numPlayers + b->numBots) - (a->numPlayers + a->numBots);
//}


#ifndef ZAP_DEDICATED
void GameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   dynamic_cast<ClientGame *>(mGame)->getUIManager()->getGameUserInterface()->renderBasicInterfaceOverlay(this, scoreboardVisible);
}


void GameType::renderObjectiveArrow(const GameObject *target, const Color *c, F32 alphaMod) const
{
   if(!target)
      return;

   GameConnection *gc = dynamic_cast<ClientGame *>(mGame)->getConnectionToServer();
   GameObject *ship = NULL;

   if(gc)
      ship = gc->getControlObject();
   if(!ship)
      return;

   Rect r = target->getBounds(MoveObject::RenderState);
   Point nearestPoint = ship->getRenderPos();

   if(r.max.x < nearestPoint.x)
      nearestPoint.x = r.max.x;
   if(r.min.x > nearestPoint.x)
      nearestPoint.x = r.min.x;
   if(r.max.y < nearestPoint.y)
      nearestPoint.y = r.max.y;
   if(r.min.y > nearestPoint.y)
      nearestPoint.y = r.min.y;

   renderObjectiveArrow(&nearestPoint, c, alphaMod);
}


void GameType::renderObjectiveArrow(const Point *nearestPoint, const Color *outlineColor, F32 alphaMod) const
{
   ClientGame *game = dynamic_cast<ClientGame *>(mGame);

   GameConnection *gc = game->getConnectionToServer();

   GameObject *co = NULL;
   if(gc)
      co = gc->getControlObject();
   if(!co)
      return;

   Point rp = game->worldToScreenPoint(nearestPoint);
   Point center(400, 300);
   Point arrowDir = rp - center;

   F32 er = arrowDir.x * arrowDir.x / (350 * 350) + arrowDir.y * arrowDir.y / (250 * 250);
   if(er < 1)
      return;
   Point np = rp;

   er = sqrt(er);
   rp.x = arrowDir.x / er;
   rp.y = arrowDir.y / er;
   rp += center;

   F32 dist = (np - rp).len();

   arrowDir.normalize();
   Point crossVec(arrowDir.y, -arrowDir.x);

   // Fade the arrows as we transition to/from commander's map
   F32 alpha = (1 - game->getCommanderZoomFraction()) * 0.6f * alphaMod;
   if(!alpha)
      return;

   // Make indicator fade as we approach the target
   if(dist < 50)
      alpha *= dist * 0.02f;

   // Scale arrow accorging to distance from objective --> doesn't look very nice
   //F32 scale = max(1 - (min(max(dist,100),1000) - 100) / 900, .5);
   F32 scale = 1.0;

   Point p2 = rp - arrowDir * 23 * scale + crossVec * 8 * scale;
   Point p3 = rp - arrowDir * 23 * scale - crossVec * 8 * scale;

   Color fillColor = *outlineColor;    // Create local copy
   fillColor *= .7f;

   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   // This loops twice: once to render the objective arrow, once to render the outline
   for(S32 i = 0; i < 2; i++)
   {
      glColor(i == 1 ? &fillColor : outlineColor, alpha);
      glBegin(i == 1 ? GL_POLYGON : GL_LINE_LOOP);
         glVertex(rp);
         glVertex(p2);
         glVertex(p3);
      glEnd();
   }

//   Point cen = rp - arrowDir * 12;

   // Try labelling the objective arrows... kind of lame.
   //UserInterface::drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}
#endif


// Server only
void GameType::gameOverManGameOver()
{
   if(mGameOver)     // Only do this once
      return;

   mBetweenLevels = true;
   mGameOver = true;                     // Show scores at end of game
   s2cSetGameOver(true);                 // Alerts clients that the game is over
   ((ServerGame *)mGame)->gameEnded();   // Sets level-switch timer, which gives us a short delay before switching games

   onGameOver();                         // Call game-specific end-of-game code

   saveGameStats();
}


// Server only
VersionedGameStats GameType::getGameStats()
{
   VersionedGameStats stats;
   GameStats *gameStats = &stats.gameStats;

   gameStats->serverName = mGame->getSettings()->getHostName();     // Not sent anywhere, used locally for logging stats

   gameStats->isOfficial = false;
   gameStats->isTesting = mGame->isTestServer();
   gameStats->playerCount = 0; //mClientList.size(); ... will count number of players.
   gameStats->duration = mTotalGamePlay / 1000;
   gameStats->isTeamGame = isTeamGame();
   gameStats->levelName = mLevelName.getString();
   gameStats->gameType = getGameTypeString();
   gameStats->cs_protocol_version = CS_PROTOCOL_VERSION;
   gameStats->build_version = BUILD_VERSION;

   gameStats->teamStats.resize(mGame->getTeamCount());


   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      TeamStats *teamStats = &gameStats->teamStats[i];

      const Color *color = mGame->getTeamColor(i);
      teamStats->intColor = color->toU32();
      teamStats->hexColor = color->toHexString();

      teamStats->name = mGame->getTeam(i)->getName().getString();
      teamStats->score = ((Team *)(mGame->getTeam(i)))->getScore();
      teamStats->gameResult = 0;  // will be filled in later

      for(S32 j = 0; j < mGame->getClientCount(); j++)
      {
         ClientInfo *clientInfo = mGame->getClientInfo(j);
         GameConnection *conn = clientInfo->getConnection();

         // Only looking for players on the current team
         if(clientInfo->getTeamIndex() != i)  // this is not sorted... 
            continue;

         teamStats->playerStats.push_back(PlayerStats());
         PlayerStats *playerStats = &teamStats->playerStats.last();

         Statistics *statistics = &conn->mStatistics;
            
         playerStats->name           = clientInfo->getName().getString();    // TODO: What if this is a bot??  What should go here??
         playerStats->nonce          = *clientInfo->getId();
         playerStats->isRobot        = clientInfo->isRobot();
         playerStats->points         = clientInfo->getScore();

         playerStats->kills          = statistics->getKills();
         playerStats->deaths         = statistics->getDeaths();
         playerStats->suicides       = statistics->getSuicides();
         playerStats->fratricides    = statistics->getFratricides();

         playerStats->switchedTeamCount = conn->switchedTeamCount;
         playerStats->isAdmin           = clientInfo->isAdmin();
         playerStats->isLevelChanger    = clientInfo->isLevelChanger();
         playerStats->isAuthenticated   = clientInfo->isAuthenticated();
         playerStats->isHosting         = conn->isLocalConnection();

         playerStats->flagPickup          = statistics->mFlagPickup;
         playerStats->flagDrop            = statistics->mFlagDrop;
         playerStats->flagReturn          = statistics->mFlagReturn;
         playerStats->flagScore           = statistics->mFlagScore;
         playerStats->crashedIntoAsteroid = statistics->mCrashedIntoAsteroid;
         playerStats->changedLoadout      = statistics->mChangedLoadout;
         playerStats->teleport            = statistics->mTeleport;
         playerStats->playTime            = statistics->mPlayTime / 1000;

         playerStats->gameResult     = 0; // will fill in later

         Vector<U32> shots = statistics->getShotsVector();
         Vector<U32> hits = statistics->getHitsVector();

         for(S32 k = 0; k < shots.size(); k++)
         {
            WeaponStats weaponStats;
            weaponStats.weaponType = WeaponType(k);
            weaponStats.shots = shots[k];
            weaponStats.hits = hits[k];
            weaponStats.hitBy = statistics->getHitBy(WeaponType(k));
            if(weaponStats.shots != 0 || weaponStats.hits != 0 || weaponStats.hitBy != 0)
               playerStats->weaponStats.push_back(weaponStats);
         }

         for(S32 k = 0; k < ModuleCount; k++)
         {
            ModuleStats moduleStats;
            moduleStats.shipModule = ShipModule(k);
            moduleStats.seconds = statistics->getModuleUsed(ShipModule(k)) / 1000;
            if(moduleStats.seconds != 0)
               playerStats->moduleStats.push_back(moduleStats);
         }

         gameStats->playerCount++;
      }
   }
   return stats;
}


// Sorts players by score, high to low
S32 QSORT_CALLBACK playerScoreSort(ClientInfo **a, ClientInfo **b)
{
   return (*b)->getScore() - (*a)->getScore();
}


// Client only
void GameType::getSortedPlayerScores(S32 teamIndex, Vector<ClientInfo *> &playerScores) const
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *info = mGame->getClientInfo(i);

      if(!isTeamGame() || info->getTeamIndex() == teamIndex)
         playerScores.push_back(info);
   }

   playerScores.sort(playerScoreSort);
}


////////////////////////////////////////
////////////////////////////////////////

class InsertStatsToDatabaseThread : public TNL::Thread
{
private:
   GameSettings *mSettings;
   VersionedGameStats mStats;

public:
   InsertStatsToDatabaseThread(GameSettings *settings, const VersionedGameStats &stats) { mSettings = settings; mStats = stats; }
   virtual ~InsertStatsToDatabaseThread() { }

   U32 run()
   {
#ifdef BF_WRITE_TO_MYSQL
      if(mSettings->getIniSettings()->mySqlStatsDatabaseServer != "")
      {
         DatabaseWriter databaseWriter(mSettings->getIniSettings()->mySqlStatsDatabaseServer.c_str(), 
                                       mSettings->getIniSettings()->mySqlStatsDatabaseName.c_str(),
                                       mSettings->getIniSettings()->mySqlStatsDatabaseUser.c_str(),   
                                       mSettings->getIniSettings()->mySqlStatsDatabasePassword.c_str() );
         databaseWriter.insertStats(mStats.gameStats);
      }
      else
#endif
      logGameStats(&mStats);      // Log to sqlite db


      delete this; // will this work?
      return 0;
   }
};


// Transmit statistics to the master server, LogStats to game server
// Only runs on server
void GameType::saveGameStats()
{
   MasterServerConnection *masterConn = mGame->getConnectionToMaster();

   VersionedGameStats stats = getGameStats();

#ifdef TNL_ENABLE_ASSERTS
   {
      // This is to find any errors with write/read
      BitStream s;
      VersionedGameStats stats2;
      Types::write(s, stats);
      U32 size = s.getBitPosition();
      s.setBitPosition(0);
      Types::read(s, &stats2);

      TNLAssert(s.isValid(), "Stats not valid, problem with gameStats.cpp read/write");
      TNLAssert(size == s.getBitPosition(), "Stats not equal size, problem with gameStats.cpp read/write");
   }
#endif
 
   if(masterConn)
      masterConn->s2mSendStatistics(stats);

   if(gServerGame->getSettings()->getIniSettings()->logStats)
   {
      processStatsResults(&stats.gameStats);

      InsertStatsToDatabaseThread *statsthread = new InsertStatsToDatabaseThread(gServerGame->getSettings(), stats);
      statsthread->start();
   }
}


// Handle the end-of-game...  handles all games... not in any subclasses
// Can be overridden for any game-specific game over stuff
// Server only
void GameType::onGameOver()
{
   static StringTableEntry teamString("Team ");
   static StringTableEntry emptyString;

   bool tied = false;
   Vector<StringTableEntry> e;

   if(isTeamGame())   // Team game -> find top team
   {
      S32 teamWinner = 0;
      S32 winningScore = ((Team *)(mGame->getTeam(0)))->getScore();
      for(S32 i = 1; i < mGame->getTeamCount(); i++)
      {
         if(((Team *)(mGame->getTeam(i)))->getScore() == winningScore)
            tied = true;
         else if(((Team *)(mGame->getTeam(i)))->getScore() > winningScore)
         {
            teamWinner = i;
            winningScore = ((Team *)(mGame->getTeam(i)))->getScore();
            tied = false;
         }
      }
      if(!tied)
      {
         e.push_back(teamString);
         e.push_back(mGame->getTeam(teamWinner)->getName());
      }
   }
   else                    // Individual game -> find player with highest score
   {
      S32 clientCount = mGame->getClientCount();

      if(clientCount)
      {
         ClientInfo *winningClient = mGame->getClientInfo(0);

         for(S32 i = 1; i < clientCount; i++)
         {
            ClientInfo *clientInfo = mGame->getClientInfo(i);

            tied = (clientInfo->getScore() == winningClient->getScore());     // TODO: I think this logic is wrong -- what if scores are in the order 4 5 5 4 will still be tied?

            if(!tied && clientInfo->getScore() > winningClient->getScore())
               winningClient = clientInfo;
         }

         if(!tied)
         {
            e.push_back(emptyString);
            e.push_back(winningClient->getName());
         }
      }
   }

   static StringTableEntry tieMessage("The game ended in a tie.");
   static StringTableEntry winMessage("%e0%e1 wins the game!");

   if(tied)
      broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, tieMessage);
   else
      broadcastMessage(GameConnection::ColorNuclearGreen, SFXFlagCapture, winMessage, e);
}


// Tells the client that the game is now officially over... scoreboard should be displayed, no further scoring will be allowed
TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cSetGameOver, (bool gameOver), (gameOver),
                            NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   mBetweenLevels = gameOver;
   mGameOver = gameOver;

   // Exit shuffle mode
   if(gameOver)
   {
      ClientGame *clientGame = static_cast<ClientGame *>(mGame);
      if(clientGame->getUIMode() == TeamShuffleMode) 
         clientGame->enterMode(PlayMode);
   }
#endif
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cCanSwitchTeams, (bool allowed), (allowed),
                            NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mCanSwitchTeams = allowed;
}


// Need to bump the priority of the gameType up really high, to ensure it gets ghosted first, before any game-specific objects like nexuses and
// other things that need to get registered with the gameType.  This will fix (I hope) the random crash-at-level start issues that have
// been annoying everyone so much.
F32 GameType::getUpdatePriority(NetObject *scopeObject, U32 updateMask, S32 updateSkips)
{
   return F32_MAX;      // High priority!!
}


bool GameType::isTeamGame() const 
{
   return mGame->getTeamCount() > 1;
}


// Find all spubugs in the game, and store them for future reference
// server only
void GameType::catalogSpybugs()
{
   Vector<DatabaseObject *> spyBugs;
   mSpyBugs.clear();

   // Find all spybugs in the game, load them into mSpyBugs
   mGame->getGameObjDatabase()->findObjects(SpyBugTypeNumber, spyBugs);

   mSpyBugs.resize(spyBugs.size());
   for(S32 i = 0; i < spyBugs.size(); i++)
      mSpyBugs[i] = dynamic_cast<Object *>(spyBugs[i]); // convert to SafePtr
}


void GameType::addSpyBug(SpyBug *spybug)
{
   mSpyBugs.push_back(dynamic_cast<Object *>(spybug)); // convert to SafePtr 
}


// Only runs on server
void GameType::addWall(WallRec wall, Game *game)
{
   mWalls.push_back(wall);
   wall.constructWalls(game);
}


// Runs on server, after level has been loaded from a file.  Can be overridden, but isn't.
void GameType::onLevelLoaded()
{
   catalogSpybugs();

   // Figure out if this level has any loadout zones
   fillVector.clear();
   mGame->getGameObjDatabase()->findObjects(LoadoutZoneTypeNumber, fillVector);

   mLevelHasLoadoutZone = (fillVector.size() > 0);

   Robot::startBots();           // Cycle through all our bots and start them up
}


// Gets run in editor and game
void GameType::onAddedToGame(Game *game)
{
   //game->setGameType(this);    // also set in GameType::addToGame(), which I think is a better place

   if(mGame->isServer())
      mShowAllBots = mGame->isTestServer();  // Default to true to show all bots if on testing mode
}


// Server only! (overridden in NexusGame)
void GameType::spawnShip(ClientInfo *clientInfo)
{
   GameConnection *conn = clientInfo->getConnection();

   static const U32 INACTIVITY_THRESHOLD = 20000;    // 20 secs, in ms

   // Check if player is "on hold" due to inactivity; if so, delay spawn and alert client.  Never display bots.
   if((conn->getTimeSinceLastMove() > INACTIVITY_THRESHOLD) && !clientInfo->isRobot())
   {
      s2cPlayerSpawnDelayed();
      return;
   }


   U32 teamIndex = clientInfo->getTeamIndex();

   Point spawnPoint = getSpawnPoint(teamIndex);

   conn->respawnTimer.clear(); // Prevent spawning a second copy of the same player ship

   if(clientInfo->isRobot())
   {
      Robot *robot = (Robot *) conn->getControlObject();
      robot->setOwner(conn);
      robot->setTeam(teamIndex);
      spawnRobot(robot);
   }
   else
   {
      // Player's name, team, and spawn location
      Ship *newShip = new Ship(clientInfo->getName(), clientInfo->isAuthenticated(), teamIndex, spawnPoint);
      clientInfo->getConnection()->setControlObject(newShip);
      newShip->setOwner(clientInfo->getConnection());
      newShip->addToGame(mGame, mGame->getGameObjDatabase());
   }

   if(!levelHasLoadoutZone())
      setClientShipLoadout(conn, conn->getLoadout());          // Set loadout if this is a SpawnWithLoadout type of game, or there is no loadout zone
   else
      setClientShipLoadout(conn, conn->mOldLoadout, true);     // Still using old loadout because we haven't entered a loadout zone yet...
   conn->mOldLoadout.clear();
}


// Note that we need to have spawn method here so we can override it for different game types
void GameType::spawnRobot(Robot *robot)
{
   SafePtr<Robot> robotPtr = robot;

   Point spawnPoint = getSpawnPoint(robotPtr->getTeam());

   if(!robot->initialize(spawnPoint))
   {
      if(robotPtr.isValid())
         robotPtr->deleteObject();
      return;
   }

   // Should probably do this, but... not now.
   //if(isSpawnWithLoadoutGame())
   //   setClientShipLoadout(cl, theClient->getLoadout());                  // Set loadout if this is a SpawnWithLoadout type of game
}


Point GameType::getSpawnPoint(S32 teamIndex)
{
   // Invalid team id
   if(teamIndex < 0 || teamIndex >= mGame->getTeamCount())
      return Point(0,0);

   Team *team = (Team *)mGame->getTeam(teamIndex);

   // If team has no spawn points, we'll just have them spawn at 0,0
   if(team->getSpawnPointCount() == 0)
      return Point(0,0);

   S32 spawnIndex = TNL::Random::readI() % team->getSpawnPointCount();    // Pick random spawn point
   return team->getSpawnPoint(spawnIndex);
}


// This gets run when the ship hits a loadout zone -- server only
void GameType::SRV_updateShipLoadout(GameObject *shipObject)
{
   GameConnection *gc = shipObject->getControllingClient();

   if(gc)
      setClientShipLoadout(gc, gc->getLoadout());
}


// Return error message if loadout is invalid, return "" if it looks ok
// Runs on client and server
string GameType::validateLoadout(const Vector<U32> &loadout)
{
   bool spyBugAllowed = false;

   if(loadout.size() != ShipModuleCount + ShipWeaponCount)     // Reject improperly sized loadouts.  Currently 2 + 3
      return "Invalid loadout size";

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      if(loadout[i] >= U32(ModuleCount))   // Invalid number.  Might crash server if trying to continue...
         return "Invalid module in loadout";

      if(!mEngineerEnabled && (loadout[i] == ModuleEngineer)) // Reject engineer if not enabled
         return "Engineer module not allowed here";

      if((loadout[i] == ModuleSensor))    // Allow spyBug when using Sensor
         spyBugAllowed = true;
   }

   for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
   {
      if(loadout[i] >= U32(WeaponCount))  // Invalid number
         return "Invalid weapon in loadout";


      if(loadout[i] == WeaponSpyBug && !spyBugAllowed)      // Reject spybug when not using ModuleSensor
         return "Spybug not allowed when sensor not selected; loadout not set";


      if(loadout[i] == WeaponTurret)      // Reject WeaponTurret
         return "Illegal weapon in loadout";

      if(loadout[i] == WeaponHeatSeeker)  // Reject HeatSeeker, not supported yet
         return "Illegal weapon in loadout";
   }

   return "";     // Passed validation
}


// Set the "on-deck" loadout for a ship, and make it effective immediately if we're in a loadout zone
// Server only, called in direct response to request from client via c2sRequestLoadout()
void GameType::SRV_clientRequestLoadout(GameConnection *conn, const Vector<U32> &loadout)
{
   Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());

   if(ship)
   {
      GameObject *object = ship->isInZone(LoadoutZoneTypeNumber);

      if(object)
         if(object->getTeam() == ship->getTeam() || object->getTeam() == -1)
            setClientShipLoadout(conn, loadout, false);
   }
}


// Called from above
// Server only -- to trigger this on client, use GameConnection::c2sRequestLoadout()
void GameType::setClientShipLoadout(GameConnection *conn, const Vector<U32> &loadout, bool silent)
{
   if(validateLoadout(loadout) != "")
      return;

   Ship *theShip = dynamic_cast<Ship *>(conn->getControlObject());

   if(theShip)
      theShip->setLoadout(loadout, silent);
}


// Runs only on server, I think
void GameType::performScopeQuery(GhostConnection *connection)
{
   GameConnection *conn = (GameConnection *) connection;
   ClientInfo *clientInfo = conn->getClientInfo();
   GameObject *co = conn->getControlObject();

   //TNLAssert(gc, "Invalid GameConnection in gameType.cpp!");
   //TNLAssert(co, "Invalid ControlObject in gameType.cpp!");

   const Vector<SafePtr<GameObject> > &scopeAlwaysList = mGame->getScopeAlwaysList();

   conn->objectInScope(this);   // Put GameType in scope, always

   // Make sure the "always-in-scope" objects are actually in scope
   for(S32 i = 0; i < scopeAlwaysList.size(); i++)
      if(!scopeAlwaysList[i].isNull())
         conn->objectInScope(scopeAlwaysList[i]);

   // readyForRegularGhosts is set once all the RPCs from the GameType
   // have been received and acknowledged by the client
   if(conn->isReadyForRegularGhosts() && co)
   {
      performProxyScopeQuery(co, clientInfo);
      conn->objectInScope(co);            // Put controlObject in scope ==> This is where the update mask gets set to 0xFFFFFFFF
   }

   // What does the spy bug see?
   for(S32 i = mSpyBugs.size()-1; i >= 0; i--)
   {
      SpyBug *sb = dynamic_cast<SpyBug *>(mSpyBugs[i].getPointer());
      if(!sb)  // SpyBug is destroyed?
         mSpyBugs.erase_fast(i);
      else
      {
         if(!sb->isVisibleToPlayer(clientInfo->getTeamIndex(), clientInfo->getName(), isTeamGame()))
            break;

         Point pos = sb->getActualPos();
         Point scopeRange(gSpyBugRange, gSpyBugRange);
         Rect queryRect(pos, pos);
         queryRect.expand(scopeRange);

         fillVector.clear();
         mGame->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, fillVector, queryRect);

         for(S32 j = 0; j < fillVector.size(); j++)
            connection->objectInScope(dynamic_cast<GameObject *>(fillVector[j]));
      }
   }
}



// Here is where we determine which objects are visible from player's ships.  Only runs on server.
void GameType::performProxyScopeQuery(GameObject *scopeObject, ClientInfo *clientInfo)
{
   // If this block proves unnecessary, then we can remove the whole itemsOfInterest thing, I think...
   //if(isTeamGame())
   //{
   //   // Start by scanning over all items located in queryItemsOfInterest()
   //   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
   //   {
   //      if(mItemsOfInterest[i].teamVisMask & (1 << scopeObject->getTeam()))    // Item is visible to scopeObject's team
   //      {
   //         Item *theItem = mItemsOfInterest[i].theItem;
   //         connection->objectInScope(theItem);

   //         if(theItem->isMounted())                                 // If item is mounted...
   //            connection->objectInScope(theItem->getMount());       // ...then the mount is visible too
   //      }
   //   }
   //}

   // If we're in commander's map mode, then we can see what our teammates can see.  
   // This will also scope what we can see.

   GameConnection *connection = clientInfo->getConnection();
   TNLAssert(connection, "NULL gameConnection!");

   if(isTeamGame() && connection->isInCommanderMap())
   {
      S32 teamId = clientInfo->getTeamIndex();
      fillVector.clear();

      for(S32 i = 0; i < mGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mGame->getClientInfo(i);

         if(clientInfo->getTeamIndex() != teamId)      // Wrong team
            continue;

         Ship *ship = dynamic_cast<Ship *>(clientInfo->getConnection()->getControlObject());
         if(!ship)       // Can happen!
            continue;

         Rect queryRect(ship->getActualPos(), ship->getActualPos());
         queryRect.expand(mGame->getScopeRange(ship->hasModule(ModuleSensor)));

         if (scopeObject == ship)
            mGame->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, fillVector, queryRect);
         else
            if (ship && ship->hasModule(ModuleSensor))
               mGame->getGameObjDatabase()->findObjects((TestFunc)isVisibleOnCmdrsMapWithSensorType, fillVector, queryRect);
            else
               mGame->getGameObjDatabase()->findObjects((TestFunc)isVisibleOnCmdrsMapType, fillVector, queryRect);
      }
   }
   else     // Do a simple query of the objects within scope range of the ship
   {
      // Note that if we make mine visibility controlled by server, here's where we'd put the code
      Point pos = scopeObject->getActualPos();
      Ship *co = dynamic_cast<Ship *>(scopeObject);
      TNLAssert(co, "Null control object!");

      Rect queryRect(pos, pos);
      queryRect.expand( mGame->getScopeRange(co->hasModule(ModuleSensor)) );

      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, fillVector, queryRect);
   }

   // Set object-in-scope for all objects found above
   for(S32 i = 0; i < fillVector.size(); i++)
      connection->objectInScope(dynamic_cast<GameObject *>(fillVector[i]));
   
   if(mShowAllBots && connection->isInCommanderMap())
   {
      for(S32 i = 0; i < Robot::robots.size(); i++)
         connection->objectInScope(Robot::robots[i]);
   }
}


// Server only
void GameType::addItemOfInterest(MoveItem *item)
{
#ifdef TNL_DEBUG
   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
      TNLAssert(mItemsOfInterest[i].theItem.getPointer() != item, "Item already exists in ItemOfInterest!");
#endif

   ItemOfInterest i;
   i.theItem = item;
   i.teamVisMask = 0;
   mItemsOfInterest.push_back(i);
}


// Here we'll cycle through all itemsOfInterest, then find any ships within scope range of each.  We'll then mark the object as being visible
// to those teams with ships close enough to see it, if any.  Called from idle()
void GameType::queryItemsOfInterest()
{
   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
   {
      ItemOfInterest &ioi = mItemsOfInterest[i];
      if(ioi.theItem.isNull())
      {
         // This can happen when dropping NexusFlagItem in ZoneControlGameType
         TNLAssert(false,"item in ItemOfInterest is NULL. This can happen when an item got deleted.");
         mItemsOfInterest.erase(i);    // When not in debug mode, the TNLAssert is not fired.  Delete the problem object and carry on.
         break;
      }

      ioi.teamVisMask = 0;                         // Reset mask, object becomes invisible to all teams
      
      Point pos = ioi.theItem->getActualPos();
      Point scopeRange(Game::PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL, Game::PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL);
      Rect queryRect(pos, pos);

      queryRect.expand(scopeRange);
      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isShipType, fillVector, queryRect);

      for(S32 j = 0; j < fillVector.size(); j++)
      {
         Ship *theShip = dynamic_cast<Ship *>(fillVector[j]);     // Safe because we only looked for ships and robots
         Point delta = theShip->getActualPos() - pos;
         delta.x = fabs(delta.x);
         delta.y = fabs(delta.y);

         if( (theShip->hasModule(ModuleSensor) && delta.x < Game::PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL && delta.y < Game::PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL) ||
               (delta.x < Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL && delta.y < Game::PLAYER_VISUAL_DISTANCE_VERTICAL) )
            ioi.teamVisMask |= (1 << theShip->getTeam());      // Mark object as visible to theShip's team
      }
   }
}


// Team in range?    Currently not used.
// Could use it for processArguments, but out of range will be UNKNOWN name and should not cause any errors.
bool GameType::checkTeamRange(S32 team)
{
   return (team < mGame->getTeamCount() && team >= -2);
}


// Zero teams will crash, returns true if we had to add a default team
bool GameType::makeSureTeamCountIsNotZero()
{
   if(mGame->getTeamCount() == 0) 
   {
      Team *team = new Team;           // Will be deleted by TeamManager
      team->setName("Missing Team");
      team->setColor(Colors::blue);
      mGame->addTeam(team);

      return true;
   }

   return false;
}


const Color *GameType::getTeamColor(GameObject *theObject)
{
   return getTeamColor(theObject->getTeam());
}


// This method can be overridden by other game types that handle colors differently
const Color *GameType::getTeamColor(S32 teamIndex) const
{
   return mGame->getTeamColor(teamIndex);
}


// TODO: can be replaced by getTeamColor?
const Color *GameType::getShipColor(Ship *s)
{
   return getTeamColor(s->getTeam());
}


// Runs on the server.
// Adds a new client to the game when a player joins, or when a level cycles.
// Note that when a new game starts, players will be added in order from
// strongest to weakest.  Bots will be added to their predefined teams, or if that is invalid, to the lowest ranked team.
void GameType::serverAddClient(ClientInfo *clientInfo)
{
   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn, "Attempting to add a client with a NULL connection!");

   conn->setScopeObject(this);
   conn->reset();

   getGame()->countTeamPlayers();     // Also calcs team ratings

   // Figure out how many players the team with the fewest players has
   Team *team = (Team *)mGame->getTeam(0);
   S32 minPlayers = team->getPlayerCount() + team->getBotCount();

   for(S32 i = 1; i < mGame->getTeamCount(); i++)
   {
      team = (Team *)mGame->getTeam(i);

      if(team->getPlayerBotCount() < minPlayers)
         minPlayers = team->getPlayerBotCount();
   }

   // Of the teams with minPlayers, find the one with the lowest total rating...
   S32 minTeamIndex = 0;
   F32 minRating = F32_MAX;

   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      team = (Team *)mGame->getTeam(i);
      if(team->getPlayerBotCount() == minPlayers && team->getRating() < minRating)
      {
         minTeamIndex = i;
         minRating = team->getRating();
      }
   }

   if(clientInfo->isRobot())                              // Robots use their own team number, if it is valid
   {
      Ship *ship = dynamic_cast<Ship *>(conn->getControlObject());

      if(ship)
      {
         if(ship->getTeam() >= 0 && ship->getTeam() < mGame->getTeamCount())        // No neutral or hostile bots -- why not?
            minTeamIndex = ship->getTeam();

         ship->setMaskBits(Ship::ChangeTeamMask);        // This is needed to avoid gray robot ships when using /addbot
      }
   }
   
   clientInfo->setTeamIndex(minTeamIndex);     // Add new player to that team

   // Tell other clients about the new guy, who is never us...
   s2cAddClient(clientInfo->getName(), false, clientInfo->isAdmin(), clientInfo->isRobot(), true);    

   if(clientInfo->getTeamIndex() >= 0) 
      s2cClientJoinedTeam(clientInfo->getName(), clientInfo->getTeamIndex());

   spawnShip(clientInfo);
}


// Determines who can damage what.  Can be overridden by individual games.  Currently only overridden by Rabbit.
bool GameType::objectCanDamageObject(GameObject *damager, GameObject *victim)
{
   if(!damager)            // Anonomyous projectiles are deadly to all!
      return true;

   //if(!strcmp(damager->getClassName(), "Mine"))        // Mines can damage anyone!  (there's got to be a better way to tell if damager is a mine... but getObjectMask doesn't seem to do the trick...)
   //   return true;

   GameConnection *damagerOwner = damager->getOwner();
   GameConnection *victimOwner = victim->getOwner();

   if(!victimOwner)     // Perhaps the victim is dead?!?
      return true;

   // Asteroids do damage
   if( dynamic_cast<Asteroid *>(damager) )
      return true;

   WeaponType weaponType;

   if( Projectile *proj = dynamic_cast<Projectile *>(damager) )
      weaponType = proj->mWeaponType;
   else if( GrenadeProjectile *grenproj = dynamic_cast<GrenadeProjectile*>(damager) )
      weaponType = grenproj->mWeaponType;
   else
      return false;

   // Check for self-inflicted damage
   if(damagerOwner == victimOwner)
      return GameWeapon::weaponInfo[weaponType].damageSelfMultiplier != 0;

   // Check for friendly fire
   else if(damager->getTeam() == victim->getTeam())
      return !isTeamGame() || GameWeapon::weaponInfo[weaponType].canDamageTeammate;

   return true;
}


// Handle scoring when ship is killed
void GameType::controlObjectForClientKilled(ClientInfo *victim, GameObject *clientObject, GameObject *killerObject)
{
   ClientInfo *killer = killerObject && killerObject->getOwner() ? killerObject->getOwner()->getClientInfo() : NULL;

   victim->getConnection()->addDeath();

   StringTableEntry killerDescr = killerObject->getKillString();

   if(killer)     // Known killer
   {
      if(killer == victim)    // We killed ourselves -- should have gone easy with the bouncers!
      {
         killer->getConnection()->addSuicide();
         updateScore(killer, KillSelf);
      }

      // Should do nothing with friendly fire disabled
      else if(isTeamGame() && killer->getTeamIndex() == victim->getTeamIndex())   // Same team in a team game
      {
         killer->getConnection()->addFratricide();
         updateScore(killer, KillTeammate);
      }

      else                                                                        // Different team, or not a team game
      {
         killer->getConnection()->addKill();
         updateScore(killer, KillEnemy);
      }

      s2cKillMessage(victim->getName(), killer->getName(), killerObject->getKillString());
   }
   else              // Unknown killer... not a scorable event.  Unless killer was an asteroid!
   {
      if( dynamic_cast<Asteroid *>(killerObject) )       // Asteroid
         updateScore(victim, KilledByAsteroid, 0);
      else                                               // Check for turret shot
      {
         Projectile *projectile = dynamic_cast<Projectile *>(killerObject);

         if( projectile && projectile->mShooter.isValid() && dynamic_cast<Turret *>(projectile->mShooter.getPointer()) )
            updateScore(victim, KilledByTurret, 0);
      }


      s2cKillMessage(victim->getName(), NULL, killerDescr);
   }

   victim->getConnection()->respawnTimer.reset(RespawnDelay);
}


// Handle score for ship and robot
// Runs on server only?
void GameType::updateScore(Ship *ship, ScoringEvent scoringEvent, S32 data)
{
   TNLAssert(ship, "Ship is null in updateScore!!");

   ClientInfo *clientInfo = NULL;

   if(ship->getControllingClient())
      clientInfo = ship->getControllingClient()->getClientInfo();  // Get client reference for ships...

   updateScore(clientInfo, ship->getTeam(), scoringEvent, data);
}


// Handle both individual scores and team scores
// Runs on server only
void GameType::updateScore(ClientInfo *player, S32 teamIndex, ScoringEvent scoringEvent, S32 data)
{
   if(mGameOver)     // Score shouldn't change once game is complete
      return;

   S32 newScore = S32_MIN;

   if(player != NULL)
   {
      // Individual scores
      S32 points = getEventScore(IndividualScore, scoringEvent, data);

      if(points != 0)
      {
         player->addScore(points);
         newScore = player->getScore();

         // Broadcast player scores for rendering on the client
         for(S32 i = 0; i < mGame->getClientCount(); i++)
            if(!isTeamGame())
               s2cSetPlayerScore(i, mGame->getClientInfo(i)->getScore());

         updateLeadingPlayerAndScore();
      }
   }

   if(isTeamGame())
   {
      // Just in case...  completely superfluous, gratuitous check
      if(teamIndex < 0 || teamIndex >= mGame->getTeamCount())
         return;

      S32 points = getEventScore(TeamScore, scoringEvent, data);
      if(points == 0)
         return;

      Team *team = (Team *)mGame->getTeam(teamIndex);
      team->addScore(points);

      // This is kind of a hack to emulate adding a point to every team *except* the scoring team.  The scoring team has its score
      // deducted, then the same amount is added to every team.  Assumes that points < 0.
      if(scoringEvent == ScoreGoalOwnTeam)
      {
         for(S32 i = 0; i < mGame->getTeamCount(); i++)
         {
            ((Team *)mGame->getTeam(i))->addScore(-points);            // Add magnitiude of negative score to all teams
            s2cSetTeamScore(i, ((Team *)(mGame->getTeam(i)))->getScore());     // Broadcast result
         }
      }
      else  // All other scoring events
         s2cSetTeamScore(teamIndex, team->getScore());     // Broadcast new team score

      updateLeadingTeamAndScore();
      newScore = mLeadingTeamScore;
   }

   checkForWinningScore(newScore);              // Check if score is high enough to trigger end-of-game
}


// Sets mLeadingTeamScore and mLeadingTeam; runs on client and server
void GameType::updateLeadingTeamAndScore()
{
   mLeadingTeamScore = S32_MIN;
   mDigitsNeededToDisplayScore = -1;

   // Find the leading team...
   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      S32 score = ((Team *)(mGame->getTeam(i)))->getScore();
      S32 digits = score == 0 ? 1 : (S32(log10(F32(abs(score)))) + (score < 0 ? 2 : 1));

      mDigitsNeededToDisplayScore = max(digits, mDigitsNeededToDisplayScore);

      if(score > mLeadingTeamScore)
      {
         mLeadingTeamScore = ((Team *)(mGame->getTeam(i)))->getScore();
         mLeadingTeam = i;
      }
   }
}


// Sets mLeadingTeamScore and mLeadingTeam; runs on client and server
void GameType::updateLeadingPlayerAndScore()
{
   mLeadingPlayerScore = 0;
   mLeadingPlayer = -1;
   mSecondLeadingPlayerScore = 0;
   mSecondLeadingPlayer = -1;

   // Find the leading player
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      S32 score = mGame->getClientInfo(i)->getScore();

      if(score > mLeadingPlayerScore)
      {
         mLeadingPlayerScore = score;
         mLeadingPlayer = i;
         continue;
      }

      if(score > mSecondLeadingPlayerScore)
      {
         mSecondLeadingPlayerScore = score;
         mSecondLeadingPlayer = i;
      }
   }
}


// Different signature for more common usage
void GameType::updateScore(ClientInfo *clientInfo, ScoringEvent event, S32 data)
{
   if(clientInfo)
      updateScore(clientInfo, clientInfo->getTeamIndex(), event, data);
   // Else, no one scores... sometimes client really is null
}


// Signature for team-only scoring event
void GameType::updateScore(S32 team, ScoringEvent event, S32 data)
{
   updateScore(NULL, team, event, data);
}


// At game end, we need to update everyone's game-normalized ratings
void GameType::updateRatings()
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      GameConnection *conn = mGame->getClientInfo(i)->getConnection();
      conn->endOfGameScoringHandler();
   }
}


void GameType::checkForWinningScore(S32 newScore)
{
   if(newScore >= mWinningScore)        // End game if max score has been reached
      gameOverManGameOver();
}


// What does a particular scoring event score?
S32 GameType::getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data)
{
   if(scoreGroup == TeamScore)
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 1;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -1;           // was zero in 015a
         case KillTeammate:
            return -1;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         default:
            return naScore;
      }
   }
   else  // scoreGroup == IndividualScore
   {
      switch(scoreEvent)
      {
         case KillEnemy:
            return 1;
         case KilledByAsteroid:  // Fall through OK
         case KilledByTurret:    // Fall through OK
         case KillSelf:
            return -1;
         case KillTeammate:
            return -1;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         default:
            return naScore;
      }
   }
}


#ifndef ZAP_DEDICATED

static void switchTeamsCallback(ClientGame *game, U32 unused)
{
   GameType *gt = game->getGameType();
   if(!gt)
      return;

   // If there are only two teams, just switch teams and skip the rigamarole
   if(game->getTeamCount() == 2)
   {
      Ship *ship = dynamic_cast<Ship *>(game->getConnectionToServer()->getControlObject());  // Returns player's ship...
      if(!ship)
         return;

      gt->c2sChangeTeams(1 - ship->getTeam());                                            // If two teams, team will either be 0 or 1, so "1 - " will toggle
      game->getUIManager()->reactivateMenu(game->getUIManager()->getGameUserInterface()); // Jump back into the game (this option takes place immediately)
   }
   else
   {
      TeamMenuUserInterface *ui = game->getUIManager()->getTeamMenuUserInterface();
      ui->activate();     // Show menu to let player select a new team
      ui->nameToChange = game->getClientInfo()->getName();
   }
 }


// Add any additional game-specific menu items, processed below
void GameType::addClientGameMenuOptions(ClientGame *game, MenuUserInterface *menu)
{
   if(isTeamGame() && mGame->getTeamCount() > 1 && !mBetweenLevels)
   {
      GameConnection *gc = game->getConnectionToServer();
      if(!gc)
         return;

      ClientInfo *clientInfo = gc->getClientInfo();

      if(mCanSwitchTeams || clientInfo->isAdmin())
         menu->addMenuItem(new MenuItem("SWITCH TEAMS", switchTeamsCallback, "", KEY_S, KEY_T));
      else
      {
         menu->addMenuItem(new MessageMenuItem("WAITING FOR SERVER TO ALLOW", Colors::red));
         menu->addMenuItem(new MessageMenuItem("YOU TO SWITCH TEAMS AGAIN", Colors::red));
      }
   }
}


static void switchPlayersTeamCallback(ClientGame *game, U32 unused)
{
   PlayerMenuUserInterface *ui = game->getUIManager()->getPlayerMenuUserInterface();

   ui->action = PlayerMenuUserInterface::ChangeTeam;
   ui->activate();
}


// Add any additional game-specific admin menu items, processed below
void GameType::addAdminGameMenuOptions(MenuUserInterface *menu)
{
   ClientGame *game = dynamic_cast<ClientGame *>(mGame);

   if(isTeamGame() && game->getTeamCount() > 1)
      menu->addMenuItem(new MenuItem("CHANGE A PLAYER'S TEAM", switchPlayersTeamCallback, "", KEY_C));
}
#endif


// Broadcast info about the current level... code gets run on client, obviously
// Note that if we add another arg to this, we need to further expand FunctorDecl methods in tnlMethodDispatch.h
// Also serves to tell the client we're on a new level.
GAMETYPE_RPC_S2C(GameType, s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, 
                                                StringTableEntry levelCreds, S32 objectCount, F32 lx, F32 ly, F32 ux, F32 uy, 
                                                bool levelHasLoadoutZone, bool engineerEnabled),
                                            (levelName, levelDesc, teamScoreLimit, 
                                                levelCreds, objectCount, lx, ly, ux, uy, 
                                                levelHasLoadoutZone, engineerEnabled))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   mLevelName = levelName;
   mLevelDescription = levelDesc;
   mLevelCredits = levelCreds;

   mWinningScore = teamScoreLimit;
   mObjectsExpected = objectCount;

   mEngineerEnabled = engineerEnabled;

   mViewBoundsWhileLoading = Rect(lx, ly, ux, uy);

   // Need to send this to the client because we won't know for sure when the loadout zones will be sent, so searching for them is difficult
   mLevelHasLoadoutZone = levelHasLoadoutZone;        

   clientGame->mObjectsLoaded = 0;              // Reset item counter

   GameUserInterface *gameUI = clientGame->getUIManager()->getGameUserInterface();
   gameUI->mShowProgressBar = true;             // Show progress bar

   //clientGame->setInCommanderMap(true);       // If we change here, need to tell the server we are in this mode
   //clientGame->resetZoomDelta();

   gameUI->resetLevelInfoDisplayTimer();        // Start displaying the level info, now that we have it

   gameUI->getLoadoutHelper(clientGame)->initialize(engineerEnabled);    // Now we know all we need to initialize our loadout options
#endif
}


GAMETYPE_RPC_C2S(GameType, c2sAddTime, (U32 time), (time))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())                // Level changers and above
      return;

   // Use voting when no level change password and more then 1 players
   if(!clientInfo->isAdmin() && gServerGame->getSettings()->getLevelChangePassword() == "" && 
            gServerGame->getPlayerCount() > 1 && gServerGame->voteStart(clientInfo, 1, time))
      return;

   mGameTimer.extend(time);                         // Increase "official time"
   s2cSetTimeRemaining(mGameTimer.getCurrent());    // Broadcast time to clients

   static StringTableEntry EXTEND_MESSAGE("%e0 has extended the game");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, EXTEND_MESSAGE, e);
}


GAMETYPE_RPC_C2S(GameType, c2sChangeTeams, (S32 team), (team))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isAdmin() && source->mSwitchTimer.getCurrent())    // If we're not admin and we're waiting for our switch-expiration to reset,
      return;                                                     // return without processing the change team request

   // Vote to change team might have different problems than the old way...
   if( (!clientInfo->isLevelChanger() || gServerGame->getSettings()->getLevelChangePassword() == "") && 
        gServerGame->getPlayerCount() > 1 )
   {
      if(gServerGame->voteStart(clientInfo, 4, team))
         return;
   }

   changeClientTeam(clientInfo, team);

   if(!clientInfo->isAdmin() && gServerGame->getPlayerCount() > 1)
   {
      NetObject::setRPCDestConnection(NetObject::getRPCSourceConnection());   // Send c2s to the changing player only
      s2cCanSwitchTeams(false);                                               // Let the client know they can't switch until they hear back from us
      NetObject::setRPCDestConnection(NULL);

      source->mSwitchTimer.reset(SwitchTeamsDelay);
   }
}


// Add some more time to the game (exercized by a user with admin privs)
void GameType::addTime(U32 time)
{
   c2sAddTime(time);
}


// Change client's team.  If team == -1, then pick next team
void GameType::changeClientTeam(ClientInfo *client, S32 team)
{
   if(mGame->getTeamCount() <= 1)         // Can't change if there's only one team...
      return;

   if(team >= mGame->getTeamCount())      // Make sure team is in range; negative values are allowed
      return;

   if(client->getTeamIndex() == team)     // Don't explode if not switching team
      return;

   Ship *ship = dynamic_cast<Ship *>(client->getConnection()->getControlObject());    // Get the ship that's switching

   if(ship)
   {
      // Find all spybugs and mines that this player owned, and reset ownership
      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isGrenadeType, fillVector);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         GameObject *obj = dynamic_cast<GameObject *>(fillVector[i]);

         if((obj->getOwner()) == ship->getOwner())
            obj->setOwner(NULL);
      }

      if(ship->isRobot())              // Players get a new ship object, robots reuse the same ship object
         ship->setMaskBits(Ship::ChangeTeamMask);

      ship->kill();                    // Destroy the old ship

      client->getConnection()->respawnTimer.clear();    // If we've just died, this will keep a second copy of ourselves from appearing
   }

   if(team < 0)                                                                     // If no team provided...
      client->setTeamIndex((client->getTeamIndex() + 1) % mGame->getTeamCount());   // ...find the next one...
   else                                                                             // ...otherwise...
      client->setTeamIndex(team);                                                   // ...use the one provided

   if(client->getTeamIndex() >= 0)                                                  // But if we know the team...
      s2cClientJoinedTeam(client->getName(), client->getTeamIndex());               // ...announce the change

   spawnShip(client);                                                               // Create a new ship
   client->getConnection()->switchedTeamCount++;                                    // Track number of times the player switched teams
}


// A player (either us or a remote player) has joined the game.  This will be called for all players (including us) when changing levels.
// This suggests that RemoteClientInfos are not retained from game to game, but are generated anew.
GAMETYPE_RPC_S2C(GameType, s2cAddClient, 
                (StringTableEntry name, bool isLocalClient, bool isAdmin, bool isRobot, bool playAlert), 
                (name, isLocalClient, isAdmin, isRobot, playAlert))
{
#ifndef ZAP_DEDICATED

   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   TNLAssert(clientGame, "Invalid client game!");
   if(!clientGame) 
      return;
      
   boost::shared_ptr<ClientInfo> clientInfo = boost::shared_ptr<ClientInfo>(new RemoteClientInfo(name, isRobot, isAdmin));  

   clientGame->onPlayerJoined(clientInfo, isLocalClient, playAlert);

#endif
}


// Player appears to be away, spawn is on hold until he returns
GAMETYPE_RPC_S2C(GameType, s2cPlayerSpawnDelayed, (), ())
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "Invalid client game!");

   if(!clientGame) 
      return;

   clientGame->setSpawnDelayed(true);

   // If the player is busy in some other UI, there is nothing to do here -- spawn will be undelayed when
   // they return to the game.  If the player is in game, however, we'll show them a message.
   // Spawn will be automatically undelayed when user reactivates gameUI by pressing any key.
   UIManager *uiManager = clientGame->getUIManager();

   // Check if we're in the gameUI, and make sure a helper menu isn't open and we're not chatting
   if(uiManager->getCurrentUI()->getMenuID() == GameUI && !uiManager->getGameUserInterface()->isHelperActive() && 
                                                          !uiManager->getGameUserInterface()->isChatting())
   {
      ErrorMessageUserInterface *errUI = uiManager->getErrorMsgUserInterface();

      errUI->reset();
      errUI->setPresentation(1);
      errUI->setTitle("PRESS ANY KEY TO SPAWN");
      errUI->setMessage(2, "You were killed; press any key to continue playing.");
      errUI->activate();
   }
#endif
}


GAMETYPE_RPC_C2S(GameType, c2sPlayerSpawnUndelayed, (), ())
{
   GameConnection *conn = (GameConnection *) getRPCSourceConnection();
   conn->resetTimeSinceLastMove();
   spawnShip(conn->getClientInfo());
}


// Remove a client from the game
// Server only
void GameType::serverRemoveClient(ClientInfo *clientInfo)
{
   if(clientInfo->getConnection())
   {
      // Blow up the ship...
      GameObject *theControlObject = clientInfo->getConnection()->getControlObject();

      if(theControlObject)
      {
         Ship *ship = dynamic_cast<Ship *>(theControlObject);
         if(ship)
            ship->kill();
      }
   }

   s2cRemoveClient(clientInfo->getName());            // Tell other clients that this one has departed

   getGame()->removeFromClientList(clientInfo);   

   updateLeadingPlayerAndScore();
   // Note that we do not need to delete clientConnection... TNL handles that, and the destructor gets runs shortly after we get here
}


// Server notifies clients that a player has changed name
GAMETYPE_RPC_S2C(GameType, s2cRenameClient, (StringTableEntry oldName, StringTableEntry newName), (oldName, newName))
{
#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      if(mGame->getClientInfo(i)->getName() == oldName)
      {
         mGame->getClientInfo(i)->setName(newName);
         break;
      }
   }

   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "%s changed to %s", oldName.getString(), newName.getString());
#endif
}


// Server has notified us that a player has left the game
GAMETYPE_RPC_S2C(GameType, s2cRemoveClient, (StringTableEntry name), (name))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   clientGame->onPlayerQuit(name);

   updateLeadingPlayerAndScore();
#endif
}


GAMETYPE_RPC_S2C(GameType, s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b, U32 score, bool firstTeam), 
                                       (teamName, r, g, b, score, firstTeam))
{
   TNLAssert(mGame, "NULL mGame!");

   if(firstTeam)
      mGame->clearTeams();

   Team *team = new Team;           // Will be deleted by TeamManager

   team->setName(teamName);
   team->setColor(r,g,b);
   team->setScore(score);

   mGame->addTeam(team);
}


GAMETYPE_RPC_S2C(GameType, s2cSetTeamScore, (RangedU32<0, GameType::MAX_TEAMS> teamIndex, U32 score), (teamIndex, score))
{
   TNLAssert(teamIndex < U32(mGame->getTeamCount()), "teamIndex out of range");

   if(teamIndex >= U32(mGame->getTeamCount()))
      return;
   
   ((Team *)mGame->getTeam(teamIndex))->setScore(score);
   updateLeadingTeamAndScore();    
}

GAMETYPE_RPC_S2C(GameType, s2cSetPlayerScore, (U16 index, S32 score), (index, score))
{
   TNLAssert(index < U32(mGame->getClientCount()), "player index out of range");

   if(index < U32(mGame->getClientCount()))
      mGame->getClientInfo(index)->setScore(score);

   updateLeadingPlayerAndScore();
}


// Server has sent us (the client) a message telling us how much longer we have in the current game
GAMETYPE_RPC_S2C(GameType, s2cSetTimeRemaining, (U32 timeLeft), (timeLeft))
{
   mGameTimer.reset(timeLeft);
}


// Server has sent us (the client) a message telling us the winning score has changed, and who changed it
GAMETYPE_RPC_S2C(GameType, s2cChangeScoreToWin, (U32 winningScore, StringTableEntry changer), (winningScore, changer))
{
#ifndef ZAP_DEDICATED
   mWinningScore = winningScore;

   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   clientGame->displayMessage(Color(0.6f, 1, 0.8f) /*Nuclear green */, 
               "%s changed the winning score to %d.", changer.getString(), mWinningScore);
#endif
}


// Announce a new player has joined the team
GAMETYPE_RPC_S2C(GameType, s2cClientJoinedTeam, 
                (StringTableEntry name, RangedU32<0, GameType::MAX_TEAMS> teamIndex), 
                (name, teamIndex))
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mGame->findClientInfo(name);      // Will be us, if we changed teams
   if(!clientInfo)
      return;

   clientInfo->setTeamIndex((S32) teamIndex);

   // The following works as long as everyone runs with a unique name.  Fails if two players have names that collide and have
   // been corrected by the server.
   // TODO: Better place to get current player's name?  This may fail if users have same name, and system has changed it
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   GameConnection *localClient = clientGame->getConnectionToServer();

   if(localClient && localClient->getClientInfo()->getName() == name)      
      clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "You have joined team %s.", mGame->getTeamName(teamIndex).getString());
   else
      clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined team %s.", name.getString(), mGame->getTeamName(teamIndex).getString());

   // Make this client forget about any mines or spybugs he knows about... it's a bit of a kludge to do this here,
   // but this RPC only runs when a player joins the game or changes teams, so this will never hurt, and we can
   // save the overhead of sending a separate message which, while theoretically cleaner, will never be needed practically.
   fillVector.clear();
   clientGame->getGameObjDatabase()->findObjects((TestFunc)isGrenadeType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GrenadeProjectile *gp = dynamic_cast<GrenadeProjectile *>(fillVector[i]);
      if(gp->mSetBy == name)
      {
         gp->mSetBy = "";                                    // No longer set-by-self
      }
   }
#endif
}


// Announce a new player has become an admin
GAMETYPE_RPC_S2C(GameType, s2cClientBecameAdmin, (StringTableEntry name), (name))
{
#ifndef ZAP_DEDICATED
   // Get a RemoteClientInfo representing the client that just became an admin
   ClientInfo *clientInfo = mGame->findClientInfo(name);      
   if(!clientInfo)
      return;

   // Record that fact in our local copy of info about them
   clientInfo->setIsAdmin(true);

   // Now display a message to the local client, unless they were the ones who were granted the privs, in which case they already
   // saw a different message.
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   if(clientGame->getClientInfo()->getName() != name)    // Don't show message to self
      clientGame->displayMessage(Colors::cyan, "%s has been granted administrator access.", name.getString());
#endif
}


// Announce a new player has permission to change levels
GAMETYPE_RPC_S2C(GameType, s2cClientBecameLevelChanger, (StringTableEntry name), (name))
{
#ifndef ZAP_DEDICATED
   // Get a RemoteClientInfo representing the client that just became a level changer
   ClientInfo *clientInfo = mGame->findClientInfo(name);      
   if(!clientInfo)
      return;

   // Record that fact in our local copy of info about them
   clientInfo->setIsLevelChanger(true);


   // Now display a message to the local client, unless they were the ones who were granted the privs, in which case they already
   // saw a different message.
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   if(clientGame->getClientInfo()->getName() != name)    // Don't show message to self
      clientGame->displayMessage(Colors::cyan, "%s can now change levels.", name.getString());
#endif
}

// Runs after the server knows that the client is available and addressable via the getGhostIndex()
// Server only, obviously
void GameType::onGhostAvailable(GhostConnection *theConnection)
{
   NetObject::setRPCDestConnection(theConnection);    // Focus all RPCs on client only

   Rect barrierExtents = mGame->computeBarrierExtents();

   s2cSetLevelInfo(mLevelName, mLevelDescription, mWinningScore, mLevelCredits, mGame->mObjectsLoaded, 
                   barrierExtents.min.x, barrierExtents.min.y, barrierExtents.max.x, barrierExtents.max.y, 
                   mLevelHasLoadoutZone, isEngineerEnabled());

   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      Team *team = (Team *)mGame->getTeam(i);
      const Color *color = team->getColor();

      s2cAddTeam(team->getName(), color->r, color->g, color->b, team->getScore(), i == 0);
   }

   // Add all the client and team information
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      bool isLocalClient = (conn == theConnection);

      s2cAddClient(clientInfo->getName(), isLocalClient, clientInfo->isAdmin(), clientInfo->isRobot(), false);

      S32 team = clientInfo->getTeamIndex();

      if(team >= 0) 
         s2cClientJoinedTeam(clientInfo->getName(), team);
   }

   //for(S32 i = 0; i < Robot::robots.size(); i++)  //Robot is part of mClientList
   //{
   //   s2cAddClient(Robot::robots[i]->getName(), false, false, true, false);
   //   s2cClientJoinedTeam(Robot::robots[i]->getName(), Robot::robots[i]->getTeam());
   //}

   // Sending an empty list clears the barriers
   Vector<F32> v;
   s2cAddWalls(v, 0, false);

   for(S32 i = 0; i < mWalls.size(); i++)
      s2cAddWalls(mWalls[i].verts, mWalls[i].width, mWalls[i].solid);

   s2cSetTimeRemaining(mGameTimer.getCurrent());      // Tell client how much time left in current game
   s2cSetGameOver(mGameOver);
   s2cSyncMessagesComplete(theConnection->getGhostingSequence());

   NetObject::setRPCDestConnection(NULL);             // Set RPCs to go to all players
}


GAMETYPE_RPC_S2C(GameType, s2cSyncMessagesComplete, (U32 sequence), (sequence))
{
#ifndef ZAP_DEDICATED
   // Now we know the game is ready to begin...
   mBetweenLevels = false;
   c2sSyncMessagesComplete(sequence);     // Tells server we're ready to go!

   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   clientGame->computeWorldObjectExtents();          // Make sure our world extents reflect all the objects we've loaded
   Barrier::prepareRenderingGeometry(clientGame);    // Get walls ready to render

   clientGame->getUIManager()->getGameUserInterface()->mShowProgressBar = false;
   //clientGame->setInCommanderMap(false);          // Start game in regular mode, If we change here, need to tell the server we are in this mode. Map can change while in commander map.
   //clientGame->clearZoomDelta();                  // No in zoom effect
   
  clientGame->getUIManager()->getGameUserInterface()->mProgressBarFadeTimer.reset(1000);
#endif
}


// Client acknowledges that it has recieved s2cSyncMessagesComplete, and is ready to go
GAMETYPE_RPC_C2S(GameType, c2sSyncMessagesComplete, (U32 sequence), (sequence))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   if(sequence != source->getGhostingSequence())
      return;

   source->setReadyForRegularGhosts(true);
}


// Gets called multiple times as barriers are added
GAMETYPE_RPC_S2C(GameType, s2cAddWalls, (Vector<F32> verts, F32 width, bool solid), (verts, width, solid))
{
   if(!verts.size())
      mGame->deleteObjects((TestFunc)isWallType);
   else
   {
      WallRec wall;
      wall.verts = verts;
      wall.width = width;
      wall.solid = solid;

      wall.constructWalls(mGame);
   }
}


extern void writeServerBanList(CIniFile *ini, BanList *banList);

// Runs the server side commands, which the client may or may not know about

// This is server side commands, For client side commands, use UIGame.cpp, GameUserInterface::processCommand.
// When adding new commands, please update the giant CommandInfo chatCmds[] array in UIGame.cpp)
void GameType::processServerCommand(ClientInfo *clientInfo, const char *cmd, Vector<StringPtr> args)
{
   ServerGame *serverGame = dynamic_cast<ServerGame *>(mGame);
   if(!mGame)
      return;

   GameConnection *conn = clientInfo->getConnection();

   if(!stricmp(cmd, "yes"))
      serverGame->voteClient(clientInfo, true);
   else if(!stricmp(cmd, "no"))
      serverGame->voteClient(clientInfo, false);
   else
   {
      // Command not found, tell the client
      conn->s2cDisplayErrorMessage("!!! Invalid Command");
   }
}


void GameType::addBot(Vector<StringTableEntry> args)
{
   // TODO
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   GameConnection *conn = clientInfo->getConnection();

   if(mBotZoneCreationFailed)
      conn->s2cDisplayErrorMessage("!!! Zone creation failed for this level -- bots disabled");

   // Bots not allowed flag is set, unless admin
   else if(!areBotsAllowed() && !clientInfo->isAdmin())
      conn->s2cDisplayErrorMessage("!!! This level does not allow robots");

   // No default robot set
   else if(!clientInfo->isAdmin() && settings->getIniSettings()->defaultRobotScript == "" && args.size() < 2)
      conn->s2cDisplayErrorMessage("!!! This server doesn't have default robots configured");

   else if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   else if((Robot::robots.size() >= settings->getIniSettings()->maxBots && !clientInfo->isAdmin()) ||
         Robot::robots.size() >= 256)
      conn->s2cDisplayErrorMessage("!!! Can't add more bots -- this server is full");

   else if(args.size() >= 2 && !safeFilename(args[1].getString()))
      conn->s2cDisplayErrorMessage("!!! Invalid filename");

   else
   {
      Robot *robot = new Robot();

      S32 args_count = 0;
      const char *args_char[LevelLoader::MAX_LEVEL_LINE_ARGS];  // Convert to a format processArgs will allow

      // The first arg = team number, the second arg = robot script filename, the rest of args get passed as script arguments
      for(S32 i = 0; i < args.size() && i < LevelLoader::MAX_LEVEL_LINE_ARGS; i++)
      {
         args_char[i] = args[i].getString();
         args_count++;
      }

      if(!robot->processArguments(args_count, args_char, mGame))
      {
         delete robot;
         conn->s2cDisplayErrorMessage("!!! Could not start robot; please see server logs");
         return;
      }

      robot->addToGame(mGame, mGame->getGameObjDatabase());

      if(!robot->start())
      {
         delete robot;
         conn->s2cDisplayErrorMessage("!!! Could not start robot; please see server logs");
         return;
      }

      serverAddClient(robot->getClientInfo().get());

      StringTableEntry msg = StringTableEntry("Robot added by %e0");
      Vector<StringTableEntry> e;
      e.push_back(clientInfo->getName());

      broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
   }
}


GAMETYPE_RPC_C2S(GameType, c2sAddBot,
      (Vector<StringTableEntry> args),
      (args))
{
   addBot(args);
}


GAMETYPE_RPC_C2S(GameType, c2sAddBots,
      (U32 count, Vector<StringTableEntry> args),
      (count, args))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   // Invalid number of bots
   //if(count <= 0)  // this doesn't matter here, "while" loops zero times so nothing happens without this check    -sam
   //   return;  // Error message handled client-side

   S32 prevRobotSize = -1;

   while(count > 0 && prevRobotSize != Robot::robots.size()) // loop may end when cannot add anymore bots
   {
      count--;
      prevRobotSize = Robot::robots.size();
      addBot(args);
   }
}


GAMETYPE_RPC_C2S(GameType, c2sSetTime, (U32 time), (time))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   if(!clientInfo->isLevelChanger())  // Extra check in case of hacked client
      return;

   // Use voting when there is no level change password, and there is more then 1 player
   if(!clientInfo->isAdmin() && settings->getLevelChangePassword() == "" && gServerGame->getPlayerCount() > 1)
   {
      if(gServerGame->voteStart(clientInfo, 2, time))
         return;
   }

   // We want to preserve the actual, overall time of the game in mGameTimer's period
   mGameTimer.extend(time - mGameTimer.getCurrent());

   s2cSetTimeRemaining(mGameTimer.getCurrent());    // Broadcast time to clients

   static StringTableEntry msg("%e0 has changed the amount of time left in the game");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sSetWinningScore, (U32 score), (score))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   // Level changers and above
   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side


   if(score <= 0)    // i.e. score is invalid
      return;  // Error message handled client-side

   ServerGame *serverGame = dynamic_cast<ServerGame *>(mGame);

   // Use voting when there is no level change password, and there is more then 1 player
   if(!clientInfo->isAdmin() && settings->getLevelChangePassword() == "" && serverGame->getPlayerCount() > 1)
      if(serverGame->voteStart(clientInfo, 3, score))
         return;

   mWinningScore = score;
   s2cChangeScoreToWin(mWinningScore, clientInfo->getName());
}


GAMETYPE_RPC_C2S(GameType, c2sResetScore, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   // Level changers and above
   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   ServerGame *serverGame = dynamic_cast<ServerGame *>(mGame);

   // Reset player scores
   for(S32 i = 0; i < serverGame->getClientCount(); i++)
      mGame->getClientInfo(i)->setScore(0);

   // Reset team scores
   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      // Set the score internally...
      ((Team*)mGame->getTeam(i))->setScore(0);

      // ...and broadcast it to the clients
      s2cSetTeamScore(i, 0);

      StringTableEntry msg("%e0 has reset the score of the game");
      Vector<StringTableEntry> e;
      e.push_back(clientInfo->getName());

      broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
   }
}


GAMETYPE_RPC_C2S(GameType, c2sKickBot, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   if(Robot::robots.size() == 0)
   {
      conn->s2cDisplayErrorMessage("!!! There are no robots to kick");
      return;
   }

   // Only delete one robot - the most recently added
   delete Robot::robots[Robot::robots.size() - 1];

   StringTableEntry msg = StringTableEntry("Robot kicked by %e0");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sKickBots, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   if(Robot::robots.size() == 0)
   {
      conn->s2cDisplayErrorMessage("!!! There are no robots to kick");
      return;
   }

   // Delete all bots
   for(S32 i = Robot::robots.size() - 1; i >= 0; i--)
      delete Robot::robots[i];

   StringTableEntry msg = StringTableEntry("All robots kicked by %e0");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sShowBots, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   // Show all robots affects all players
   mShowAllBots = !mShowAllBots;  // Toggle

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   if(Robot::robots.size() == 0)
      conn->s2cDisplayErrorMessage("!!! There are no robots to show");
   else
   {
      StringTableEntry msg = mShowAllBots ? StringTableEntry("Show all robots option enabled by %e0") : StringTableEntry("Show all robots option disabled by %e0");
      Vector<StringTableEntry> e;
      e.push_back(clientInfo->getName());

      broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
   }
}


GAMETYPE_RPC_C2S(GameType, c2sSetMaxBots, (S32 count), (count))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   // Invalid number of bots
   if(count <= 0)
      return;  // Error message handled client-side

   settings->getIniSettings()->maxBots = count;

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   Vector<StringTableEntry> e;
   e.push_back(itos(count));
   conn->s2cDisplayMessageE(GameConnection::ColorRed, SFXNone, "Maximum bots was changed to %e0", e);
}


GAMETYPE_RPC_C2S(GameType, c2sBanPlayer, (StringTableEntry playerName, U32 duration), (playerName, duration))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   // Conn is the connection of the player doing the banning
   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   ClientInfo *bannedClientInfo = mGame->findClientInfo(playerName);

   // Player not found
   if(!bannedClientInfo)
      return;  // Error message handled client-side

   if(bannedClientInfo->isAdmin())
      return;  // Error message handled client-side

   // Cannot ban robot
   if(!bannedClientInfo->getConnection()->isEstablished())
      return;  // Error message handled client-side

   Address ipAddress = bannedClientInfo->getConnection()->getNetAddressString();

   S32 banDuration = duration == 0 ? settings->getBanList()->getDefaultBanDuration() : duration;

   // Add the ban
   settings->getBanList()->addToBanList(ipAddress, banDuration);
   logprintf(LogConsumer::ServerFilter, "%s was banned for %d minutes", ipAddress.toString(), banDuration);

   // Save BanList in memory
   writeServerBanList(&gINI, settings->getBanList());

   // Save new INI settings to disk
   gINI.WriteFile();

   GameConnection *conn = clientInfo->getConnection();

   // Disconnect player
   bannedClientInfo->getConnection()->disconnect(NetConnection::ReasonBanned, "");
   conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Player was banned");
}


GAMETYPE_RPC_C2S(GameType, c2sBanIp, (StringTableEntry ipAddressString, U32 duration), (ipAddressString, duration))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = gServerGame->getSettings();

   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   Address ipAddress(ipAddressString.getString());

   if(!ipAddress.isValid())
      return;  // Error message handled client-side

   S32 banDuration = duration == 0 ? settings->getBanList()->getDefaultBanDuration() : duration;

   // banip should always add to the ban list, even if the IP isn't connected
   settings->getBanList()->addToBanList(ipAddress, banDuration);
   logprintf(LogConsumer::ServerFilter, "%s - banned for %d minutes", ipAddress.toString(), banDuration);

   // Save BanList in memory
   writeServerBanList(&gINI, settings->getBanList());

   // Save new INI settings to disk
   gINI.WriteFile();

   // Now check to see if the client is connected and disconnect them if they are
   GameConnection *connToDisconnect = NULL;

   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      GameConnection *baneeConn = mGame->getClientInfo(i)->getConnection();

      if(baneeConn->getNetAddress().isEqualAddress(ipAddress))
      {
         connToDisconnect = baneeConn;
         break;
      }
   }

   GameConnection *conn = clientInfo->getConnection();

   if(!connToDisconnect)
      conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Client has been banned but is no longer connected");
   else
   {
      connToDisconnect->disconnect(NetConnection::ReasonBanned, "");
      conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Client was banned and kicked");
   }
}


GAMETYPE_RPC_C2S(GameType, c2sRenamePlayer, (StringTableEntry playerName, StringTableEntry newName), (playerName, newName))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   ClientInfo *renamedClientInfo = mGame->findClientInfo(playerName);

   // Player not found
   if(!renamedClientInfo)
      return;  // Error message handled client-side

   if(renamedClientInfo->isAuthenticated())
      return;  // Error message handled client-side

   StringTableEntry oldName = renamedClientInfo->getName();
   renamedClientInfo->setName("");                        // Avoid unique self
   StringTableEntry uniqueName = GameConnection::makeUnique(newName.getString()).c_str();  // New name
   renamedClientInfo->setName(oldName);                   // Restore name to properly get it updated to clients
   renamedClientInfo->setAuthenticated(false);            // Don't underline anymore because of rename
   updateClientChangedName(renamedClientInfo, uniqueName);

   GameConnection *conn = clientInfo->getConnection();
   conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, StringTableEntry("Player has been renamed"));
}


GAMETYPE_RPC_C2S(GameType, c2sGlobalMutePlayer, (StringTableEntry playerName), (playerName))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   GameConnection *gc = mGame->findClientInfo(playerName)->getConnection();

   if(!gc)
      return;  // Error message handled client-side

   GameConnection *conn = clientInfo->getConnection();

   // Toggle
   gc->mChatMute = !gc->mChatMute;

   conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, gc->mChatMute ? "Player is muted" : "Player is not muted");
}


GAMETYPE_RPC_C2S(GameType, c2sTriggerTeamChange, (StringTableEntry playerName, S32 teamIndex), (playerName, teamIndex))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();

   if(!sourceClientInfo->isAdmin())
      return;  // Error message handled client-side

   ClientInfo *playerClientInfo = mGame->findClientInfo(playerName);

   // Player disappeared
   if(!playerClientInfo)
      return;

   changeClientTeam(playerClientInfo, teamIndex);

   playerClientInfo->getConnection()->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "An admin has shuffled you to a different team");
}



GAMETYPE_RPC_C2S(GameType, c2sKickPlayer, (StringTableEntry playerName), (playerName))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();

   if(!sourceClientInfo->isAdmin())
      return;

   ClientInfo *playerClientInfo = mGame->findClientInfo(playerName);

   if(!playerClientInfo)    // Hmmm... couldn't find the dude.  Maybe he disconnected?
      return;

   if(playerClientInfo->isAdmin())
   {
      source->s2cDisplayErrorMessage("Can't kick an administrator!");
      return;
   }

   if(playerClientInfo->getConnection()->isEstablished())     // Robots don't have established connections
   {
      ConnectionParameters &p = playerClientInfo->getConnection()->getConnectionParameters();

      if(p.mIsArranged)
         gServerGame->getSettings()->getBanList()->kickHost(p.mPossibleAddresses[0]);      // Banned for 30 seconds

      gServerGame->getSettings()->getBanList()->kickHost(playerClientInfo->getConnection()->getNetAddress());      // Banned for 30 seconds
      playerClientInfo->getConnection()->disconnect(NetConnection::ReasonKickedByAdmin, "");
   }

   // Get rid of robots tied to the player
   for(S32 i = 0; i < Robot::robots.size(); i++)
      if(Robot::robots[i]->getName() == playerName)
         delete Robot::robots[i];

   Vector<StringTableEntry> e;
   e.push_back(playerName);  // --> Name of player being administered
   e.push_back(sourceClientInfo->getName());  // --> Name of player doing the administering

   broadcastMessage(GameConnection::ColorAqua, SFXIncomingMessage, "%e0 was kicked from the game by %e1.", e);
}


GAMETYPE_RPC_C2S(GameType, c2sSendCommand, (StringTableEntry cmd, Vector<StringPtr> args), (cmd, args))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   processServerCommand(source->getClientInfo(), cmd.getString(), args);     // was conn instead of source...
}


// Send a private message
GAMETYPE_RPC_C2S(GameType, c2sSendChatPM, (StringTableEntry toName, StringPtr message), (toName, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();

   bool found = false;

   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);

      if(clientInfo->getName() == toName)     // Do we want a case insensitive search?
      {
         if(!source->checkMessage(message.getString(), 2))
            return;

         RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayChatPM, (sourceClientInfo->getName(), toName, message));
         source->postNetEvent(theEvent);

         if(source != clientInfo->getConnection())          // No sending messages to self
            clientInfo->getConnection()->postNetEvent(theEvent);

         found = true;
         break;
      }
   }

   if(!found)
      source->s2cDisplayErrorMessage("!!! Player not found");
}


// Client sends chat message to/via game server
GAMETYPE_RPC_C2S(GameType, c2sSendChat, (bool global, StringPtr message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();

   if(!source->checkMessage(message.getString(), global ? 0 : 1))
      return;

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayChatMessage, (global, sourceClientInfo->getName(), message));
   sendChatDisplayEvent(sourceClientInfo, global, message.getString(), theEvent);
}


// Sends a quick-chat message (which, due to its repeated nature can be encapsulated in a StringTableEntry item)
GAMETYPE_RPC_C2S(GameType, c2sSendChatSTE, (bool global, StringTableEntry message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   if(!source->checkMessage(message.getString(), global ? 0 : 1))
      return;

   ClientInfo *sourceClientInfo = source->getClientInfo();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayChatMessageSTE, (global, sourceClientInfo->getName(), message));
   sendChatDisplayEvent(sourceClientInfo, global, message.getString(), theEvent);
}


// Send a chat message that will be displayed in-game
// If not global, send message only to other players on team
void GameType::sendChatDisplayEvent(ClientInfo *sender, bool global, const char *message, NetEvent *theEvent)
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      S32 senderTeamIndex = sender->getTeamIndex();

      if(global || clientInfo->getTeamIndex() == senderTeamIndex)
         clientInfo->getConnection()->postNetEvent(theEvent);
   }

   // And fire an event handler...
   Robot::getEventManager().fireEvent(NULL, EventManager::MsgReceivedEvent, message, sender->getConnection()->getPlayerInfo(), global);
}


extern Color gGlobalChatColor;
extern Color gTeamChatColor;

// Server sends message to the client for display using StringPtr
GAMETYPE_RPC_S2C(GameType, s2cDisplayChatPM, (StringTableEntry fromName, StringTableEntry toName, StringPtr message), (fromName, toName, message))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   ClientInfo *localClientInfo = clientGame->getClientInfo();
   GameUserInterface *gameUI = clientGame->getUIManager()->getGameUserInterface();

   Color theColor = Colors::yellow;

   if(localClientInfo->getName() == toName && toName == fromName)      // Message sent to self
      gameUI->displayChatMessage(theColor, "%s: %s", toName.getString(), message.getString());

   else if(localClientInfo->getName() == toName)                       // To this player
      gameUI->displayChatMessage(theColor, "from %s: %s", fromName.getString(), message.getString());

   else if(localClientInfo->getName() == fromName)                     // From this player
      gameUI->displayChatMessage(theColor, "to %s: %s", toName.getString(), message.getString());

   else  
      TNLAssert(false, "Should never get here... shouldn't be able to see PM that is not from or not to you"); 
#endif
}


GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message), (global, clientName, message))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");

   if(!clientGame || clientGame->isOnMuteList(clientName.getString()))
      return;

   Color theColor = global ? gGlobalChatColor : gTeamChatColor;
   clientGame->getUIManager()->getGameUserInterface()->displayChatMessage(theColor, "%s: %s", clientName.getString(), message.getString());
#endif
}


// Server sends message to the client for display using StringTableEntry
GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessageSTE, (bool global, StringTableEntry clientName, StringTableEntry message), (global, clientName, message))
{
#ifndef ZAP_DEDICATED
   Color theColor = global ? gGlobalChatColor : gTeamChatColor;
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);

   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) 
      return;

   clientGame->getUIManager()->getGameUserInterface()->displayChatMessage(theColor, "%s: %s", clientName.getString(), message.getString());
#endif
}


// Client requests start/stop of streaming pings and scores from server to client
GAMETYPE_RPC_C2S(GameType, c2sRequestScoreboardUpdates, (bool updates), (updates))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   //GameConnection *conn = source->getClientRef()->getConnection();          // I think these were the same... I think?

   source->setWantsScoreboardUpdates(updates);

   if(updates)
      updateClientScoreboard(source->getClientInfo());
}


// Client tells server that they chose the next weapon
GAMETYPE_RPC_C2S(GameType, c2sAdvanceWeapon, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   Ship *ship = dynamic_cast<Ship *>(source->getControlObject());
   if(ship)
      ship->selectWeapon();
}


// Client tells server that they dropped flag or other item
GAMETYPE_RPC_C2S(GameType, c2sDropItem, (), ())
{
   //logprintf("%s GameType->c2sDropItem", isGhost()? "Client:" : "Server:");
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   Ship *ship = dynamic_cast<Ship *>(source->getControlObject());
   if(!ship)
      return;

   S32 count = ship->mMountedItems.size();
   for(S32 i = count - 1; i >= 0; i--)
      ship->mMountedItems[i]->onItemDropped();
}


//GAMETYPE_RPC_C2S(GameType, c2sResendItemStatus, (U16 itemId), (itemId))  // no need to use RPCGuaranteedOrdered
TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sResendItemStatus, (U16 itemId), (itemId), NetClassGroupGameMask, RPCGuaranteed, RPCToGhostParent, 0)
{
   //GameConnection *source = (GameConnection *) getRPCSourceConnection();  // not used

   if(mCacheResendItem.size() == 0)
      mCacheResendItem.resize(1024);

   for(S32 i=0; i < 1024; i += 256)
   {
      MoveItem *item = mCacheResendItem[S32(itemId & 255) | i];
      if(item && item->getItemId() == itemId)
      {
         item->setPositionMask();
         return;
      }
   }

   fillVector.clear();
   mGame->getGameObjDatabase()->findObjects(fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      MoveItem *item = dynamic_cast<MoveItem *>(fillVector[i]);
      if(item && item->getItemId() == itemId)
      {
         item->setPositionMask();
         for(S32 j=0; j < 1024; j += 256)
         {
            if(mCacheResendItem[S32(itemId & 255) | j].isNull())
            {
               mCacheResendItem[S32(itemId & 255) | j].set(item);
            }
            return;
         }
         mCacheResendItem[S32(itemId & 255) | (TNL::Random::readI(0,3) * 256)].set(item);
         break;
      }
   }
}


// Client tells server that they chose the specified weapon
GAMETYPE_RPC_C2S(GameType, c2sSelectWeapon, (RangedU32<0, ShipWeaponCount> indx), (indx))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   Ship *ship = dynamic_cast<Ship *>(source->getControlObject());
   if(ship)
      ship->selectWeapon(indx);
}


Vector<RangedU32<0, GameType::MaxPing> > GameType::mPingTimes; ///< Static vector used for constructing update RPCs
Vector<SignedInt<24> > GameType::mScores;
Vector<SignedFloat<8> > GameType::mRatings;  // 8 bits for 255 gradations between -1 and 1 ~ about 1 value per .01


void GameType::updateClientScoreboard(ClientInfo *requestor)
{
   mPingTimes.clear();
   mScores.clear();
   mRatings.clear();

   // First, list the players
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *info = mGame->getClientInfo(i);

      if(info->getPing() < MaxPing)
         mPingTimes.push_back(info->getPing());
      else
         mPingTimes.push_back(MaxPing);

      mScores.push_back(info->getScore());

      // Players rating = cumulative score / total score played while this player was playing, ranks from 0 to 1
      mRatings.push_back(info->getConnection()->getCalculatedRating());
   }

   // Next come the robots ... Robots is part of mClientList
   //for(S32 i = 0; i < Robot::robots.size(); i++)
   //{
   //   mPingTimes.push_back(0);
   //   mScores.push_back(Robot::robots[i]->getScore());
   //   mRatings.push_back(max(min((U32)(Robot::robots[i]->getRating() * 100.0) + 100, maxRating), minRating));
   //}

   NetObject::setRPCDestConnection(requestor->getConnection());
   s2cScoreboardUpdate(mPingTimes, mScores, mRatings);
   NetObject::setRPCDestConnection(NULL);
}


GAMETYPE_RPC_S2C(GameType, s2cScoreboardUpdate,
                 (Vector<RangedU32<0, GameType::MaxPing> > pingTimes, Vector<SignedInt<24> > scores, Vector<SignedFloat<8> > ratings),
                 (pingTimes, scores, ratings))
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      if(i >= pingTimes.size())
         break;

      ClientInfo *client = mGame->getClientInfo(i);

      client->setPing(pingTimes[i]);
      client->setScore(scores[i]);
      client->setRating(ratings[i]);
   }
}


GAMETYPE_RPC_S2C(GameType, s2cKillMessage, (StringTableEntry victim, StringTableEntry killer, StringTableEntry killerDescr), (victim, killer, killerDescr))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = dynamic_cast<ClientGame *>(mGame);
   TNLAssert(clientGame, "clientGame is NULL");
   if(!clientGame) return;

   if(killer)  // Known killer, was self, robot, or another player
   {
      if(killer == victim)
         if(killerDescr == "mine")
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by own mine", victim.getString());
         else
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s zapped self", victim.getString());
      else
         if(killerDescr == "mine")
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by mine put down by %s", victim.getString(), killer.getString());
         else
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s zapped %s", killer.getString(), victim.getString());
   }
   else if(killerDescr == "mine")   // Killer was some object with its own kill description string
      clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s got blown up by a mine", victim.getString());
   else if(killerDescr != "")
      clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s %s", victim.getString(), killerDescr.getString());
   else         // Killer unknown
      clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s got zapped", victim.getString());
#endif
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sVoiceChat, (bool echo, ByteBufferPtr voiceBuffer), (echo, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhostParent, 0)
{
   // Broadcast this to all clients on the same team; only send back to the source if echo is true

   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();


   if(source)
   {
      RefPtr<NetEvent> event = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cVoiceChat, (sourceClientInfo->getName(), voiceBuffer));

      for(S32 i = 0; i < mGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mGame->getClientInfo(i);
         GameConnection *dest = clientInfo->getConnection();

         if(clientInfo && clientInfo->getTeamIndex() == sourceClientInfo->getTeamIndex() && (dest != source || echo))
            dest->postNetEvent(event);
      }
   }
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cVoiceChat, (StringTableEntry clientName, ByteBufferPtr voiceBuffer), (clientName, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = mGame->findClientInfo(clientName);
   if(!clientInfo)
      return;

   ByteBufferPtr playBuffer = clientInfo->getVoiceDecoder()->decompressBuffer(*(voiceBuffer.getPointer()));
   SoundSystem::queueVoiceChatBuffer(clientInfo->getVoiceSFX(), playBuffer);

#endif
}


Game *GameType::getGame() const
{
   return mGame;
}


// static
StringTableEntry GameType::getGameTypeName(GameTypes gameType)
{
   switch(gameType)
   {
      case BitmatchGame:
         return "Bitmatch";
      case CTFGame:
         return "Capture the Flag";
      case HTFGame:
         return "Hold the Flag";
      case NexusGame:
         return "Nexus";
      case RabbitGame:
         return "Rabbit";
      case RetrieveGame:
         return "Retrieve";
      case SoccerGame:
         return "Soccer";
      case ZoneControlGame:
         return "Zone Control";
      default:
         TNLAssert(false, "Bad GameType value");
         return "";
   }
}


GameTypes GameType::getGameType() const
{
   return BitmatchGame;
}


const char *GameType::getGameTypeString() const
{
   return getGameTypeName(getGameType()).getString();
}


const char *GameType::getShortName() const
{
   return "BM";
}


const char *GameType::getInstructionString() const
{
   return "Blast as many ships as you can!";
}


bool GameType::canBeTeamGame() const
{
   return true;
}


bool GameType::canBeIndividualGame() const
{
   return true;
}


bool GameType::teamHasFlag(S32 teamId) const
{
   return false;
}


S32 GameType::getWinningScore() const
{
   return mWinningScore;
}


void GameType::setWinningScore(S32 score)
{
   mWinningScore = score;
}


void GameType::setGameTime(F32 timeInSeconds)
{
   mGameTimer.reset(U32(timeInSeconds) * 1000);
}


U32 GameType::getTotalGameTime() const
{
   return (mGameTimer.getPeriod() / 1000);
}


S32 GameType::getRemainingGameTime() const
{
   return (mGameTimer.getCurrent() / 1000);
}


S32 GameType::getRemainingGameTimeInMs() const
{
   return (mGameTimer.getCurrent());
}


void GameType::extendGameTime(S32 timeInMs)
{
   mGameTimer.extend(timeInMs);
}


S32 GameType::getLeadingScore() const
{
   return mLeadingTeamScore;
}


S32 GameType::getLeadingTeam() const
{
   return mLeadingTeam;
}


S32 GameType::getLeadingPlayerScore() const
{
   TNLAssert(mLeadingPlayer < mGame->getClientCount(), "mLeadingPlayer out of range");
   return mLeadingPlayerScore;
}


S32 GameType::getLeadingPlayer() const
{
   return mLeadingPlayer;
}


S32 GameType::getSecondLeadingPlayerScore() const
{
   return mSecondLeadingPlayerScore;
}


S32 GameType::getSecondLeadingPlayer() const
{
   return mSecondLeadingPlayer;
}


bool GameType::isFlagGame()
{
   return false;
}


bool GameType::isTeamFlagGame()
{
   return true;
}


S32 GameType::getFlagCount()
{
   return mFlags.size();
}


bool GameType::isCarryingItems(Ship *ship)
{
   return ship->mMountedItems.size() > 0;
}


bool GameType::onFire(Ship *ship)
{
   return true;
}


bool GameType::okToUseModules(Ship *ship)
{
   return true;
}


bool GameType::isSpawnWithLoadoutGame()
{
   return false;
}


bool GameType::levelHasLoadoutZone()
{
   return mLevelHasLoadoutZone;
}


U32 GameType::getLowerRightCornerScoreboardOffsetFromBottom() const
{
   return 60;
}


const Vector<WallRec> *GameType::getBarrierList()
{
   return &mWalls;
}



S32 GameType::getFlagSpawnCount() const
{
   return mFlagSpawnPoints.size();
}


const FlagSpawn *GameType::getFlagSpawn(S32 index) const
{
   return &mFlagSpawnPoints[index];
}


const Vector<FlagSpawn> *GameType::getFlagSpawns() const
{
   return &mFlagSpawnPoints;
}


void GameType::addFlagSpawn(FlagSpawn flagSpawn)
{
   mFlagSpawnPoints.push_back(flagSpawn);
}


void GameType::addItemSpawn(ItemSpawn *spawn)
{
   mItemSpawnPoints.push_back(boost::shared_ptr<ItemSpawn>(spawn));
   logprintf("spawn time: %d", mItemSpawnPoints.last()->getPeriod());
}


S32 GameType::getDigitsNeededToDisplayScore() const
{
   return mDigitsNeededToDisplayScore;
}


// Send a message to all clients
void GameType::broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, const StringTableEntry &message)
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
      mGame->getClientInfo(i)->getConnection()->s2cDisplayMessage(color, sfx, message);
}


// Send a message to all clients
void GameType::broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, 
                                const StringTableEntry &formatString, const Vector<StringTableEntry> &e)
{
   for(S32 i = 0; i < mGame->getClientCount(); i++)
      mGame->getClientInfo(i)->getConnection()->s2cDisplayMessageE(color, sfx, formatString, e);
}


bool GameType::isGameOver() const
{
   return mGameOver;
}

const StringTableEntry *GameType::getLevelName() const
{
   return &mLevelName;
}


void GameType::setLevelName(const StringTableEntry &levelName)
{
   mLevelName = levelName;
}


const StringTableEntry *GameType::getLevelDescription() const
{
   return &mLevelDescription;
}


void GameType::setLevelDescription(const StringTableEntry &levelDescription)
{
   mLevelDescription = levelDescription;
}


const StringTableEntry *GameType::getLevelCredits() const
{
   return &mLevelCredits;
}


void GameType::setLevelCredits(const StringTableEntry &levelCredits)
{
   mLevelCredits = levelCredits;
}


S32 GameType::getMinRecPlayers()
{
   return mMinRecPlayers;
}


void GameType::setMinRecPlayers(S32 minPlayers)
{
   mMinRecPlayers = minPlayers;
}


S32 GameType::getMaxRecPlayers()
{
   return mMaxRecPlayers;
}


void GameType::setMaxRecPlayers(S32 maxPlayers)
{
   mMaxRecPlayers = maxPlayers;
}


bool GameType::isEngineerEnabled()
{
   return mEngineerEnabled;
}


void GameType::setEngineerEnabled(bool enabled)
{
   mEngineerEnabled = enabled;
}


bool GameType::areBotsAllowed()
{
   return mBotsAllowed;
}


void GameType::setBotsAllowed(bool allowed)
{
   mBotsAllowed = allowed;
}


string GameType::getScriptName() const
{
   return mScriptName;
}


const Vector<string> *GameType::getScriptArgs()
{
   return &mScriptArgs;
}


bool GameType::isDatabasable()
{
   return false;
}


void GameType::addFlag(FlagItem *flag)
{
   mFlags.push_back(flag);
}


void GameType::itemDropped(Ship *ship, MoveItem *item)
{
   /* Do nothing */
}


void GameType::shipTouchFlag(Ship *ship, FlagItem *flag)
{
   /* Do nothing */
}


void GameType::addZone(GoalZone *zone)
{
   /* Do nothing */
}


void GameType::shipTouchZone(Ship *ship, GoalZone *zone)
{
   /* Do nothing */
}


void GameType::majorScoringEventOcurred(S32 team)
{
   /* empty */
}


};

