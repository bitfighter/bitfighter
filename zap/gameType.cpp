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
#include "ClientInfo.h"
#include "ServerGame.h"
#include "robot.h"
#include "loadoutZone.h"      // For LoadoutZone


#ifndef ZAP_DEDICATED
#   include "ClientGame.h"
#   include "loadoutHelper.h"
#   include "UIGame.h"
#   include "UIMenus.h"
#   include "UIErrorMessage.h"   
#   include "OpenglUtils.h"
#endif


#include "../master/database.h"

#include "statistics.h"
#include "masterConnection.h"    


#include "tnlThread.h"
#include <math.h>

namespace Zap
{

#ifdef TNL_OS_MAC_OSX
const S32 GameType::MAX_TEAMS;
#endif


// List of valid game types -- these are the "official" names, not the more user-friendly names provided by getGameTypeName
// All names are of the form xxxGameType, and have a corresponding class xxxGame
static const char *gameTypeClassNames[] = {
#  define GAME_TYPE_ITEM(a, type, c, d) type,
       GAME_TYPE_TABLE
#  undef GAME_TYPE_ITEM

       NULL  // Last item must be NULL
};


////////////////////////////////////////               
////////////////////////////////////////


// Pass 0 for unlimited game
void GameTimer::reset(U32 timeInMs)
{
   if(timeInMs == 0)
   {
      mTimer.reset(GameType::MAX_GAME_TIME);
      mIsUnlimited = true;
   }
   else
   {
      mTimer.reset(timeInMs);
      mIsUnlimited = false;
   }

   mRenderingOffset = 0;
   mGameOver = false;
}


void GameTimer::sync(U32 deltaT)
{
   mTimer.reset(deltaT);
}


// Return true if timer has expired, false otherwise -- unlimited games never expire
bool GameTimer::update(U32 deltaT)
{
   bool status = mTimer.update(deltaT);

   if(mGameOver)
      mRenderingOffset -= deltaT;

   if(mIsUnlimited)
      return false;

   return status;
}


U32 GameTimer::getCurrent() const
{
    return mTimer.getCurrent();
}


void GameTimer::extend(S32 deltaT)
{
   if(mTimer.getPeriod() + (U32)deltaT > (U32)GameType::MAX_GAME_TIME)
      deltaT = GameType::MAX_GAME_TIME - mTimer.getPeriod();
   mTimer.extend(deltaT);
}


void GameTimer::setIsUnlimited(bool isUnlimited)
{
   mIsUnlimited = isUnlimited;
}


bool GameTimer::isUnlimited() const
{
   return mIsUnlimited;
}


U32 GameTimer::getTotalGameTime() const      // in ms
{
   return mTimer.getPeriod();
}


string GameTimer::toString_minutes() const
{
   F32 gameTime = mIsUnlimited ? 0 : F32(mTimer.getPeriod());

   return ftos(gameTime / (60 * 1000), 3);
}


S32 GameTimer::getRenderingOffset() const
{
   return mRenderingOffset;
}


void GameTimer::setRenderingOffset(S32 offset)
{
   mRenderingOffset = offset;
}


void GameTimer::setGameIsOver()
{
   mGameOver = true;
}



////////////////////////////////////////      __              ___           
////////////////////////////////////////     /__  _. ._ _   _  |    ._   _  
////////////////////////////////////////     \_| (_| | | | (/_ | \/ |_) (/_ 
////////////////////////////////////////                         /  |       

TNL_IMPLEMENT_NETOBJECT(GameType);

// Constructor
GameType::GameType(S32 winningScore) : mScoreboardUpdateTimer(1000), mGameTimeUpdateTimer(30000), mBotBalanceAnalysisTimer(10000)
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
   mEngineerUnrestrictedEnabled = false;
   mBotsAllowed = true;
   mBotBalancingDisabled = false;

   mGameTimer.reset(DefaultGameTime);

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
      setGameTime(F32(atof(argv[0]) * 60));

   if(argc > 1)      // Second arg is winning score
      mWinningScore = atoi(argv[1]);

   return true;
}


string GameType::toLevelCode() const
{
   return string(getClassName()) + " " + mGameTimer.toString_minutes() + " " + itos(mWinningScore);
}


// GameType object is the first to be added when a new game starts... therefore, this is a reasonable signifier that a new game is starting up.  I think.
void GameType::addToGame(Game *game, GridDatabase *database)
{
   mGame = game;
   game->setGameType(this);
}


// Client only
bool GameType::onGhostAdd(GhostConnection *theConnection)
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = ((GameConnection *) theConnection)->getClientGame();
   TNLAssert(clientGame, "Should only be client here!");

   addToGame(clientGame, clientGame->getGameObjDatabase());
   return true;
#else
   TNLAssert(false, "Should only be client here!");
   return false;
#endif
}

void GameType::onGhostRemove()
{
#ifndef ZAP_DEDICATED
   TNLAssert(dynamic_cast<ClientGame *>(getGame()), "Should only be clientGame here!");
   ClientGame *clientGame = static_cast<ClientGame *>(getGame());

   // Quit EngineerHelper when level changes, or when current GameType get removed
   clientGame->getUIManager()->getGameUserInterface()->quitEngineerHelper();
#endif
}


#ifndef ZAP_DEDICATED
// Menu items we want to show
Vector<string> GameType::getGameParameterMenuKeys()
{
   static const string vals[] = {
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
      "Allow Robots"
   };

   return Vector<string> (vals, ARRAYSIZE(vals));
}



// Definitions for those items
boost::shared_ptr<MenuItem> GameType::getMenuItem(const string &key)
{
   if(key == "Level Name")
   {
      MenuItem *item = new TextEntryMenuItem("Level Name:", mLevelName.getString(), "", "The level's name -- pick a good one!", MAX_GAME_NAME_LEN);   
      item->setFilter(LineEditor::allAsciiFilter);

      return boost::shared_ptr<MenuItem>(item);
   }
   else if(key == "Level Descr")
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Level Descr:", 
                                                              mLevelDescription.getString(), 
                                                              "", 
                                                              "A brief description of the level",                     
                                                              MAX_GAME_DESCR_LEN));
   else if(key == "Level Credits")
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Level By:",       
                                                              mLevelCredits.getString(), 
                                                              "", 
                                                              "Who created this level?",                                  
                                                              MAX_GAME_DESCR_LEN));
   else if(key == "Levelgen Script")
      return boost::shared_ptr<MenuItem>(new TextEntryMenuItem("Levelgen Script:", 
                                                              getScriptLine(), 
                                                              "<None>", 
                                                              "Levelgen script & args to be run when level is loaded",  
                                                              255));
   else if(key == "Game Time")
   {
      S32 gameTime = mGameTimer.isUnlimited() ? 0 : mGameTimer.getTotalGameTime() / 1000;
      return boost::shared_ptr<MenuItem>(new TimeCounterMenuItem("Game Time:", gameTime, 99*60, "Unlimited", "Time game will last"));
   }
   else if(key == "Win Score")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Score to Win:", getWinningScore(), 1, 1, 99, "points", "", "Game ends when one team gets this score"));
   else if(key == "Grid Size")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Grid Size:",       
                                                             (S32)getGame()->getGridSize(),
                                                             Game::MIN_GRID_SIZE,      // increment
                                                             Game::MIN_GRID_SIZE,      // min val
                                                             Game::MAX_GRID_SIZE,      // max val
                                                             "pixels",                 // units
                                                             "", 
                                                             "\"Magnification factor.\" Larger values lead to larger levels.  Default is 255."));
   else if(key == "Min Players")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Min Players:",       
                                                             mMinRecPlayers,     // value
                                                             1,                  // increment
                                                             0,                  // min val
                                                             MAX_PLAYERS,        // max val
                                                             "players",          // units
                                                             "N/A", 
                                                             "Min. players you would recommend for this level (to help server select the next level)"));
   else if(key == "Max Players")
      return boost::shared_ptr<MenuItem>(new CounterMenuItem("Max Players:",       
                                                             mMaxRecPlayers,     // value
                                                             1,                  // increment
                                                             0,                  // min val
                                                             MAX_PLAYERS,        // max val
                                                             "players",          // units
                                                             "N/A",
                                                             "Max. players you would recommend for this level (to help server select the next level)"));
   else if(key == "Allow Engr")
      return boost::shared_ptr<MenuItem>(new YesNoMenuItem("Allow Engineer Module:",       
                                                           mEngineerEnabled,
                                                           "Allow players to use the Engineer module?"));
   else if(key == "Allow Robots")
         return boost::shared_ptr<MenuItem>(new YesNoMenuItem("Allow Robots:",
                                                              mBotsAllowed,
                                                              "Allow players to add robots?"));
   else
   {
      TNLAssert(false, "Invalid menu key!");
      return boost::shared_ptr<MenuItem>();     // NULLish pointer
   }
}


bool GameType::saveMenuItem(const MenuItem *menuItem, const string &key)
{
   if(key == "Level Name")
      setLevelName(menuItem->getValue());
   else if(key == "Level Descr")
      setLevelDescription(menuItem->getValue());
   else if(key == "Level Credits")
      setLevelCredits(menuItem->getValue());
   else if(key == "Levelgen Script")
      setScript(parseString(menuItem->getValue()));
   else if(key == "Game Time")
      setGameTime((F32)menuItem->getIntValue());
   else if(key == "Win Score")
      setWinningScore(menuItem->getIntValue());
   else if(key == "Grid Size")
      mGame->setGridSize((F32)menuItem->getIntValue());
   else if(key == "Min Players")
       setMinRecPlayers(menuItem->getIntValue());
   else if(key == "Max Players")
      setMaxRecPlayers(menuItem->getIntValue());
   else if(key == "Allow Engr")
      setEngineerEnabled(menuItem->getIntValue());
   else if(key == "Allow Robots")
      setBotsAllowed(menuItem->getIntValue());
   else
      return false;

   return true;
}
#endif

bool GameType::processSpecialsParam(const char *param)
{
   if(!stricmp(param, "Engineer"))
      setEngineerEnabled(true);
   else if(!stricmp(param, "EngineerUnrestricted"))
   {
      setEngineerEnabled(true);
      setEngineerUnrestrictedEnabled(true);
   }
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
   {
      if(isEngineerUnrestrictedEnabled())
         specialsLine += " EngineerUnrestricted";
      else
         specialsLine += " Engineer";
   }

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
      printf("Name: %s \n",                          GameWeapon::weaponInfo[i].name.getString());
      printf("\tEnergy Drain: %d\n",                 GameWeapon::weaponInfo[i].drainEnergy);
      printf("\tVelocity: %d\n",                     GameWeapon::weaponInfo[i].projVelocity);
      printf("\tLifespan (ms): %d\n",                GameWeapon::weaponInfo[i].projLiveTime);
      printf("\tDamage: %2.2f\n",                    GameWeapon::weaponInfo[i].damageAmount);
      printf("\tDamage To Self Multiplier: %2.2f\n", GameWeapon::weaponInfo[i].damageSelfMultiplier);
      printf("\tCan Damage Teammate: %s\n",          GameWeapon::weaponInfo[i].canDamageTeammate ? "Yes" : "No");
   }

   printf("\n\n");
   printf("Game Types:\n\n");

   for(S32 i = 0; gameTypeClassNames[i]; i++)
   {
      TNL::Object *theObject = TNL::Object::create(gameTypeClassNames[i]);  // Instantiate a gameType object
      GameType *gameType = dynamic_cast<GameType*>(theObject);              // and cast it

      string indTeam;

      if(gameType->canBeIndividualGame() && gameType->canBeTeamGame())
         indTeam = "Individual or Teams";
      else if (gameType->canBeIndividualGame())
         indTeam = "Individual only";
      else if (gameType->canBeTeamGame())
         indTeam = "Team only";
      else
         indTeam = "Configuration Error!";


      printf("Game type: %s [%s]\n", gameType->getGameTypeName(), indTeam.c_str());
      printf("Configure ship: %s",   gameType->isSpawnWithLoadoutGame() ? "By respawning (no need for loadout zones)" : "By entering loadout zone");
      printf("\nEvent: Individual Score / Team Score\n");
      printf(  "====================================\n");
      for(S32 j = 0; j < ScoringEventsCount; j++)
      {
         S32 teamScore = gameType->getEventScore(GameType::TeamScore,       (ScoringEvent) j, 0);
         S32 indScore =  gameType->getEventScore(GameType::IndividualScore, (ScoringEvent) j, 0);

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

      // Core specifig:
      case EnemyCoreDestroyed:
         return "Destroyed a Core on enemy team";
      case OwnCoreDestroyed:
         return "Destroyed a Core on own team";

      // Other:
      default:
         return "Unknown event!";
   }
}


// Will return a valid GameType string -- either what's passed in, or the default if something bogus was specified  (static)
const char *GameType::validateGameType(const char *gameTypeName)
{
   if(!stricmp(gameTypeName, "HuntersGameType"))
       return "NexusGameType";

   for(S32 i = 0; gameTypeClassNames[i]; i++)    // Repeat until we hit NULL
      if(stricmp(gameTypeClassNames[i], gameTypeName) == 0)
         return gameTypeClassNames[i];

   // If we get to here, no valid game type was specified, so we'll return the default (Bitmatch)
   return gameTypeClassNames[0];
}


void GameType::idle(BfObject::IdleCallPath path, U32 deltaT)
{
   mTotalGamePlay += deltaT;

   if(path != BfObject::ServerIdleMainLoop)    
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
      // Respawn dead players
      if(clientInfo->respawnTimer.update(deltaT))           // Need to respawn?
         spawnShip(clientInfo);                       

      if(!clientInfo->isRobot())
      {
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

         if(conn->mSwitchTimer.getCurrent())             // Are we still counting down until the player can switch?
            if(conn->mSwitchTimer.update(deltaT))        // Has the time run out?
            {                                            
               NetObject::setRPCDestConnection(conn);    // Limit who gets this message
               s2cCanSwitchTeams(true);                  // If so, let the client know they can switch again
               NetObject::setRPCDestConnection(NULL);
            }

         conn->addTimeSinceLastMove(deltaT);             // Increment timer       
      }
   }


   // Periodically send time-remaining updates to the clients to keep everyone in sync
   if(mGameTimeUpdateTimer.update(deltaT))
   {
      broadcastTimeSyncSignal();
      mGameTimeUpdateTimer.reset();
   }

   // Analyze if we need to re-balance teams with bots
   if(!mBotBalancingDisabled &&
         getGame()->getSettings()->getIniSettings()->botsBalanceTeams &&
         mBotBalanceAnalysisTimer.update(deltaT))
   {
      balanceTeams();
      mBotBalanceAnalysisTimer.reset();
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
   EventManager::get()->update();

   // If game time has expired... game is over, man, it's over
   if(advanceGameClock(deltaT))
      gameOverManGameOver();
}


void GameType::idle_client(U32 deltaT)
{
   advanceGameClock(deltaT);
   mZoneGlowTimer.update(deltaT);
}


bool GameType::advanceGameClock(U32 deltaT)
{
   return mGameTimer.update(deltaT);
}


//// Sorts teams by player counts, high to low
//S32 QSORT_CALLBACK teamSizeSort(Team *a, Team *b)
//{
//   return (b->numPlayers + b->numBots) - (a->numPlayers + a->numBots);
//}


#ifndef ZAP_DEDICATED
void GameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   static_cast<ClientGame *>(mGame)->getUIManager()->getGameUserInterface()->renderBasicInterfaceOverlay(this, scoreboardVisible);
}


// Client only
void GameType::renderObjectiveArrow(const BfObject *target) const
{
   renderObjectiveArrow(target, target->getColor(), 1.0f);
}


void GameType::renderObjectiveArrow(const BfObject *target, const Color *c, F32 alphaMod) const
{
   if(!target)
      return;

   GameConnection *gc = static_cast<ClientGame *>(mGame)->getConnectionToServer();
   BfObject *ship = NULL;

   if(gc)
      ship = gc->getControlObject();

   if(!ship)
      return;

   Point targetPoint;

   // Handle different geometry types a bit differently.
   // Note that for simpleLine objects, we will point at the origin point; this will usually be correct, but is somewhat moot as
   // we don't currently draw objective arrows towards any simpleLine objects.  If we did, this might fail for TextItems.
   GeomType geomType = target->getGeomType();
   if(geomType == geomPoint || geomType == geomSimpleLine) 
   {
      targetPoint = target->getRenderPos();
   }
   else if(geomType == geomPolyLine || geomType == geomPolygon)
   {
      Rect r = target->getExtent();
      targetPoint = ship->getPos();

      if(r.max.x < targetPoint.x)
         targetPoint.x = r.max.x;
      if(r.min.x > targetPoint.x)
         targetPoint.x = r.min.x;
      if(r.max.y < targetPoint.y)
         targetPoint.y = r.max.y;
      if(r.min.y > targetPoint.y)
         targetPoint.y = r.min.y;
   }

   renderObjectiveArrow(targetPoint, c, alphaMod);
}


void GameType::renderObjectiveArrow(const Point &nearestPoint, const Color *outlineColor, F32 alphaMod) const
{
   ClientGame *game = static_cast<ClientGame *>(mGame);

   GameConnection *gc = game->getConnectionToServer();

   BfObject *co = NULL;

   if(gc)
      co = gc->getControlObject();

   if(!co)
      return;

   Point rp = game->worldToScreenPoint(&nearestPoint);
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

   F32 vertices[] = {
         rp.x, rp.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   // This loops twice: once to render the objective arrow, once to render the outline
   for(S32 i = 0; i < 2; i++)
   {
      glColor(i == 1 ? &fillColor : outlineColor, alpha);
      renderVertexArray(vertices, 3, i == 1 ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
   }

//   Point cen = rp - arrowDir * 12;

   // Try labelling the objective arrows... kind of lame.
   //drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}
#endif


// Server only
void GameType::gameOverManGameOver()
{
   if(mGameOver)     // Only do this once
      return;

   onGameOver();                         // Call game-specific end-of-game code
   // onGameOver() goes before mGameOver = true; so win messages won't get blocked, part of preving messages going away too fast

   mBetweenLevels = true;
   mGameOver = true;                     // Show scores at end of game
   s2cSetGameOver(true);                 // Alerts clients that the game is over
   ((ServerGame *)mGame)->gameEnded();   // Sets level-switch timer, which gives us a short delay before switching games


   mGameTimer.setGameIsOver();

   saveGameStats();

   // Kick any players who were idle the entire previous game.  But DO NOT kick the hosting player!
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);

      if(!clientInfo->isRobot())
      {
         GameConnection *connection = clientInfo->getConnection();
            
         if(!connection->getObjectMovedThisGame() && !connection->isLocalConnection())    // Don't kick the host, please!
            connection->disconnect(NetConnection::ReasonIdle, "");
      }
   }
}


const char *GameType::getGameTypeName() const
{
   return getGameTypeName(getGameTypeId());
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
   gameStats->gameType = getGameTypeName();
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

         // Find players on the current team 
         if(clientInfo->getTeamIndex() != i) 
            continue;

         // Skip inactive players.  You snooze, you lose!
         if(!clientInfo->isRobot() && !clientInfo->getConnection()->getObjectMovedThisGame())
            continue;

         teamStats->playerStats.push_back(PlayerStats());
         PlayerStats *playerStats = &teamStats->playerStats.last();

         Statistics *statistics = clientInfo->getStatistics();
            
         playerStats->name           = clientInfo->getName().getString();    // TODO: What if this is a bot??  What should go here??
         playerStats->nonce          = *clientInfo->getId();
         playerStats->isRobot        = clientInfo->isRobot();
         playerStats->points         = clientInfo->getScore();

         playerStats->kills          = statistics->getKills();
         playerStats->deaths         = statistics->getDeaths();
         playerStats->suicides       = statistics->getSuicides();
         playerStats->fratricides    = statistics->getFratricides();

         // Bots have no conn, so we'll just set these stats manually.  We could streamline this bit here
         // by moving these settings over to the clientInfo.
         if(clientInfo->isRobot())
         {
            playerStats->isHosting         = false;      // bots never host
            playerStats->switchedTeamCount = 0;          // bots never switch teams
         }
         else
         {
            GameConnection *conn = clientInfo->getConnection();
            playerStats->isHosting         = conn->isLocalConnection();
            playerStats->switchedTeamCount = conn->switchedTeamCount;
         }

         playerStats->isAdmin           = clientInfo->isAdmin();
         playerStats->isLevelChanger    = clientInfo->isLevelChanger();
         playerStats->isAuthenticated   = clientInfo->isAuthenticated();

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


         Vector<U32> loadouts = statistics->getLoadouts();

         for(S32 i = 0; i < loadouts.size(); i++)
         {
            LoadoutStats loadoutStats;
            loadoutStats.loadoutHash = loadouts[i];
            if(loadoutStats.loadoutHash != 0)
               playerStats->loadoutStats.push_back(loadoutStats);
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
   // Do not send statistics for games being tested in the editor
   if(mGame->isTestServer())
      return;

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

   if(getGame()->getSettings()->getIniSettings()->logStats)
   {
      processStatsResults(&stats.gameStats);

      InsertStatsToDatabaseThread *statsthread = new InsertStatsToDatabaseThread(getGame()->getSettings(), stats);
      statsthread->start();
   }
}

// Server only
void GameType::achievementAchieved(U8 achievement, const StringTableEntry &playerName)
{
   MasterServerConnection *masterConn = getGame()->getConnectionToMaster();
   if(masterConn && masterConn->isEstablished())
   {
      // Notify the master player earned a badge
      masterConn->s2mAcheivementAchieved(achievement, playerName);

      // Visually alert other players
      s2cAchievementMessage(achievement, playerName);

      // Now re-set authentication with the new badge for the earning player; this so
      // everyone can see the badge in the scoreboard
      ClientInfo *clientInfo = mGame->findClientInfo(playerName);
      if(clientInfo)
         clientInfo->setAuthenticated(clientInfo->isAuthenticated(), clientInfo->getBadges() | BIT(achievement));
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

   if(gameOver)
   {
      // Exit shuffle mode
      ClientGame *clientGame = static_cast<ClientGame *>(mGame);
      if(clientGame->getUIMode() == TeamShuffleMode) 
         clientGame->enterMode(PlayMode);

      // Tell timer it's all over
      mGameTimer.setGameIsOver();
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


// Only runs on server
void GameType::addWall(const WallRec &wall, Game *game)
{
   mWalls.push_back(wall);       // Add wall to our list of walls
   wall.constructWalls(game);    // Build it!
}


// Runs on server, after level has been loaded from a file.  Can be overridden, but isn't.
void GameType::onLevelLoaded()
{
   GridDatabase *gridDatabase =  mGame->getGameObjDatabase();

   mLevelHasLoadoutZone      = gridDatabase->hasObjectOfType(LoadoutZoneTypeNumber);
   mLevelHasPredeployedFlags = gridDatabase->hasObjectOfType(FlagTypeNumber);
   mLevelHasFlagSpawns       = gridDatabase->hasObjectOfType(FlagSpawnTypeNumber);

   TNLAssert(dynamic_cast<ServerGame *>(mGame), "Wrong game here!");
   static_cast<ServerGame *>(mGame)->startAllBots();           // Cycle through all our bots and start them up
}


bool GameType::hasFlagSpawns() const       {   return mLevelHasFlagSpawns;         }
bool GameType::hasPredeployedFlags() const {   return mLevelHasPredeployedFlags;   }


// Gets run in editor and game
void GameType::onAddedToGame(Game *game)
{
   if(mGame->isServer())
      mShowAllBots = mGame->isTestServer();  // Default to true to show all bots if on testing mode
}


// Spawn a ship or a bot... returns true if ship/bot was spawned, false if it was not
// Server only! (overridden in NexusGame)
bool GameType::spawnShip(ClientInfo *clientInfo)
{
   // Check if player is "on hold" due to inactivity; if so, delay spawn and alert client.  Never delays bots.
   // Note, if we know that this is the beginning of a new level, we can save a wee bit of bandwidth by passing
   // NULL as first arg to setSpawnDelayed(), but we don't check this currently, and it's probably not worth doing
   // if it's not apparent.  isInitialUpdate() might work for this purpose.  Will require some testing.
   if(clientInfo->isSpawnDelayed())
      return false;

   //if(mGame->isOrIsAboutToBeSuspended())
   //   return false;

   if(static_cast<FullClientInfo *>(clientInfo)->shouldDelaySpawn())
   {
      clientInfo->setSpawnDelayed(true);
      return false;
   }

   //static_cast<ServerGame *>(getGame())->unsuspendGame(false);

   U32 teamIndex = clientInfo->getTeamIndex();

   Point spawnPoint = getSpawnPoint(teamIndex);

   clientInfo->respawnTimer.clear();   // Prevent spawning a second copy of the same player ship

   if(clientInfo->isRobot())
   {
      Robot *robot = (Robot *) clientInfo->getShip();
      robot->setTeam(teamIndex);
      spawnRobot(robot);
   }
   else
   {
      // Player's name, team, and spawn location
      Ship *newShip = new Ship(clientInfo, teamIndex, spawnPoint);
      clientInfo->getConnection()->setControlObject(newShip);
      clientInfo->setShip(newShip);

      newShip->setOwner(clientInfo);
      newShip->addToGame(mGame, mGame->getGameObjDatabase());

      if(!levelHasLoadoutZone())
      {
         // Set loadout if this is a SpawnWithLoadout type of game, or there is no loadout zone
         setClientShipLoadout(clientInfo, clientInfo->getLoadout());      
      }
      else
      {
         // Still using old loadout because we haven't entered a loadout zone yet...
         setClientShipLoadout(clientInfo, clientInfo->mOldLoadout, true); 

         // Unless we're actually spawning onto a loadout zone
         Vector<DatabaseObject *> loadoutZones;
         getGame()->getGameObjDatabase()->findObjects(LoadoutZoneTypeNumber, loadoutZones);
         LoadoutZone *zone;
         for(S32 i = 0; i < loadoutZones.size(); i++) 
         {
            zone = static_cast<LoadoutZone*>(loadoutZones.get(i));
            if(newShip->isOnObject(zone))
               zone->collide(newShip);
         } 
      }

      clientInfo->mOldLoadout.clear();
   }

   return true;
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


// Get a list of all spawns that belong to the specified team (when that is relevant)
Vector<AbstractSpawn *> GameType::getSpawnPoints(TypeNumber typeNumber, S32 teamIndex)    // teamIndex is optional
{
   bool checkTeam = isTeamGame() && teamIndex != TeamNotSpecified;   // Only find items on the passed team

   Vector<AbstractSpawn *> spawnPoints;

   const Vector<DatabaseObject *> *objects = getGame()->getGameObjDatabase()->findObjects_fast();

   for(S32 i = 0; i < objects->size(); i++)
   {
      if(objects->get(i)->getObjectTypeNumber() == typeNumber)
      {
         AbstractSpawn *spawn = static_cast<AbstractSpawn *>(objects->get(i));

         if(!checkTeam || spawn->getTeam() == teamIndex || spawn->getTeam() == TEAM_NEUTRAL)
            spawnPoints.push_back(spawn);
      }
   }

   return spawnPoints;
}


// Pick a random ship spawn point for the specified team
Point GameType::getSpawnPoint(S32 teamIndex)
{
   // Invalid team id
   if(teamIndex < 0 || teamIndex >= mGame->getTeamCount())
      return Point(0,0);

   Vector<AbstractSpawn *> spawns = getSpawnPoints(ShipSpawnTypeNumber, teamIndex);

   // If team has no spawn points, we'll just have them spawn at 0,0
   if(spawns.size() == 0)
      return Point(0,0);

   S32 spawnIndex = TNL::Random::readI() % spawns.size();    // Pick random spawn point
   return spawns[spawnIndex]->getPos();
}


// This gets run when the ship hits a loadout zone -- server only
void GameType::SRV_updateShipLoadout(BfObject *shipObject)
{
   ClientInfo *clientInfo = shipObject->getOwner();

   if(clientInfo)
      setClientShipLoadout(clientInfo, clientInfo->getLoadout());
}


// Return error message if loadout is invalid, return "" if it looks ok
// Runs on client and server
string GameType::validateLoadout(const Vector<U8> &loadout)
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

//      if(loadout[i] == WeaponSeeker)  // Reject Seeker, not supported yet
//         return "Illegal weapon in loadout";
   }

   return "";     // Passed validation
}


// Set the "on-deck" loadout for a ship, and make it effective immediately if we're in a loadout zone
// Server only, called in direct response to request from client via c2sRequestLoadout()
void GameType::SRV_clientRequestLoadout(ClientInfo *clientInfo, const Vector<U8> &loadout)
{
   Ship *ship = clientInfo->getShip();

   if(ship)
   {
      BfObject *object = ship->isInZone(LoadoutZoneTypeNumber);

      if(object)
         if(object->getTeam() == ship->getTeam() || object->getTeam() == -1)
            setClientShipLoadout(clientInfo, loadout, false);
   }
}


// Called from above and elsewhere
// Server only -- to trigger this on client, use GameConnection::c2sRequestLoadout()
void GameType::setClientShipLoadout(ClientInfo *clientInfo, const Vector<U8> &loadout, bool silent)
{
   if(validateLoadout(loadout) != "")
      return;

   Ship *ship = clientInfo->getShip();

   bool loadoutChanged = false;
   if(ship)
      loadoutChanged = ship->setLoadout(loadout, silent);

   if(loadoutChanged)
   {
      // This builds a loadout 'hash' by devoting the first 16 bits to modules, the
      // second 16 bits to weapons.  The integer created might look like so:
      //    00000000000001110000000000000011
      U32 loadoutHash = 0;
      for(S32 i = 0; i < ShipModuleCount; i++)
         loadoutHash |= BIT(loadout[i]);

      for(S32 i = ShipModuleCount; i < ShipWeaponCount + ShipModuleCount; i++)
         loadoutHash |= BIT(loadout[i]) << 16;

      clientInfo->getStatistics()->addLoadout(loadoutHash);
   }
}


static void markAllMountedItemsAsBeingInScope(Ship *ship, GameConnection *conn)
{
   for(S32 i = 0; i < ship->getMountedItemCount(); i++)  
   {
      TNLAssert(ship->getMountedItem(i), "When would this item be NULL?  Do we really need to check this?");
      if(ship->getMountedItem(i))
         conn->objectInScope(ship->getMountedItem(i));
   }
}


// Runs only on server, I think
void GameType::performScopeQuery(GhostConnection *connection)
{
   GameConnection *conn = (GameConnection *) connection;
   ClientInfo *clientInfo = conn->getClientInfo();
   BfObject *co = conn->getControlObject();

   //TNLAssert(gc, "Invalid GameConnection in gameType.cpp!");
   //TNLAssert(co, "Invalid ControlObject in gameType.cpp!");

   conn->objectInScope(this);   // Put GameType in scope, always

   if(!conn->isReadyForRegularGhosts()) // This may prevent scoping any ships until after ClientInfo is all received on client side. (spy bugs scopes ships)
      return;

   const Vector<SafePtr<BfObject> > &scopeAlwaysList = mGame->getScopeAlwaysList();

   // Make sure the "always-in-scope" objects are actually in scope
   for(S32 i = 0; i < scopeAlwaysList.size(); i++)
      if(!scopeAlwaysList[i].isNull())
         if(scopeAlwaysList[i]->getObjectTypeNumber() != FlagTypeNumber || !((MountableItem*)(((SafePtr<BfObject> *)&scopeAlwaysList[i])->getPointer()))->isMounted())
            conn->objectInScope(scopeAlwaysList[i]);

   // readyForRegularGhosts is set once all the RPCs from the GameType
   // have been received and acknowledged by the client
   if(conn->isReadyForRegularGhosts() && co)
   {
      performProxyScopeQuery(co, clientInfo);
      conn->objectInScope(co);            // Put controlObject in scope ==> This is where the update mask gets set to 0xFFFFFFFF
   }

   // What does the spy bug see?
   bool sameQuery = false;  // helps speed up by not repeatedly finding same objects


   const Vector<DatabaseObject *> *spyBugs = mGame->getGameObjDatabase()->findObjects_fast(SpyBugTypeNumber);
   
   for(S32 i = spyBugs->size()-1; i >= 0; i--)
   {
      SpyBug *sb = static_cast<SpyBug *>(spyBugs->get(i));

      if(sb->isVisibleToPlayer(clientInfo, isTeamGame()))
      {
         Point pos = sb->getActualPos();
         Rect queryRect(pos, pos);

         Point scopeRange(SpyBug::SPY_BUG_RANGE, SpyBug::SPY_BUG_RANGE);
         queryRect.expand(scopeRange);

         fillVector.clear();
         mGame->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, fillVector, queryRect, sameQuery);
         sameQuery = true;

         for(S32 j = 0; j < fillVector.size(); j++)
         {
            connection->objectInScope(static_cast<BfObject *>(fillVector[j]));
            if(isShipType(fillVector[j]->getObjectTypeNumber()))
               markAllMountedItemsAsBeingInScope(static_cast<Ship *>(fillVector[j]), conn);
         }
      }
   }
}


// Here is where we determine which objects are visible from player's ships.  Marks items as in-scope so they 
// will be sent to client.
// Only runs on server. 
void GameType::performProxyScopeQuery(BfObject *scopeObject, ClientInfo *clientInfo)
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
      bool sameQuery = false;  // helps speed up by not repeatedly finding same objects

      for(S32 i = 0; i < mGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mGame->getClientInfo(i);

         if(clientInfo->getTeamIndex() != teamId)      // Wrong team
            continue;

         Ship *ship = clientInfo->getShip();
         if(!ship)       // Can happen!
            continue;

         Rect queryRect(ship->getActualPos(), ship->getActualPos());
         queryRect.expand(mGame->getScopeRange(ship->hasModule(ModuleSensor)));

         TestFunc testFunc;
         if(scopeObject == ship)    
            testFunc = &isAnyObjectType;
         else
            if(ship && ship->hasModule(ModuleSensor))
               testFunc = &isVisibleOnCmdrsMapWithSensorType;
            else     // No sensor
               testFunc = &isVisibleOnCmdrsMapType;

         mGame->getGameObjDatabase()->findObjects(testFunc, fillVector, queryRect, sameQuery);
         sameQuery = true;
      }
   }
   else     // Not a team game OR not in commander's map -- Do a simple query of the objects within scope range of the ship
   {
      // Note that if we make mine visibility controlled by server, here's where we'd put the code
      Point pos = scopeObject->getPos();
      TNLAssert(dynamic_cast<Ship *>(scopeObject), "Control object not a ship!");
      Ship *co = static_cast<Ship *>(scopeObject);

      Rect queryRect(pos, pos);
      queryRect.expand( mGame->getScopeRange(co->hasModule(ModuleSensor)) );

      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isAnyObjectType, fillVector, queryRect);
   }

   // Set object-in-scope for all objects found above
   for(S32 i = 0; i < fillVector.size(); i++)
   {
      connection->objectInScope(static_cast<BfObject *>(fillVector[i]));
      if(isShipType(fillVector[i]->getObjectTypeNumber()))
         markAllMountedItemsAsBeingInScope(static_cast<Ship *>(fillVector[i]), connection);
   }

   // Make bots visible if showAllBots has been activated
   if(mShowAllBots && connection->isInCommanderMap())
      for(S32 i = 0; i < mGame->getBotCount(); i++)
         connection->objectInScope(mGame->getBot(i));  
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
      Point scopeRange(Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL, Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL);
      Rect queryRect(pos, pos);

      queryRect.expand(scopeRange);
      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isShipType, fillVector, queryRect);

      for(S32 j = 0; j < fillVector.size(); j++)
      {
         Ship *theShip = static_cast<Ship *>(fillVector[j]);     // Safe because we only looked for ships and robots
         Point delta = theShip->getActualPos() - pos;
         delta.x = fabs(delta.x);
         delta.y = fabs(delta.y);

         if(
               (theShip->hasModule(ModuleSensor) &&
                     delta.x < Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_HORIZONTAL && 
                     delta.y < Game::PLAYER_SENSOR_PASSIVE_VISUAL_DISTANCE_VERTICAL) ||
               (delta.x < Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL && 
                     delta.y < Game::PLAYER_VISUAL_DISTANCE_VERTICAL)
            )
            ioi.teamVisMask |= (1 << theShip->getTeam());      // Mark object as visible to theShip's team
      }
   }
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


// This method can be overridden by other game types that handle colors differently
const Color *GameType::getTeamColor(const BfObject *object) const
{
   return getTeamColor(object->getTeam());
}


// This method can be overridden by other game types that handle colors differently
const Color *GameType::getTeamColor(S32 teamIndex) const
{
   return mGame->getTeamColor(teamIndex);
}


// Runs on the server.
// Adds a new client to the game when a player or bot joins, or when a level cycles.
// Note that when a new game starts, players will be added in order from
// strongest to weakest.  Bots will be added to their predefined teams, or if that is invalid, to the lowest ranked team.
void GameType::serverAddClient(ClientInfo *clientInfo)
{
   getGame()->countTeamPlayers();     // Also calcs team ratings

   if(!clientInfo->isRobot())
   {
      GameConnection *conn = clientInfo->getConnection();

      if(conn)
      {
         conn->setScopeObject(this);
         conn->reset();
      }
   }

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

   // Robots use their own team number, so if we have a valid one, override that assigned above
   if(clientInfo->isRobot())                              
   {
      Ship *robot = clientInfo->getShip();

      if(clientInfo->getShip()->getTeam() >= 0 && robot->getTeam() < mGame->getTeamCount())   // No neutral or hostile bots -- why not?
         minTeamIndex = robot->getTeam();

      robot->setChangeTeamMask();            // Needed to avoid gray robot ships when using /addbot
   }
   
   clientInfo->setTeamIndex(minTeamIndex);   // Add new player to their assigned team

   // This message gets sent to all clients, even the client being added, though they presumably know most of this stuff already
   // This clientInfo belongs to the server; has no badge info for client...
   s2cAddClient(clientInfo->getName(), clientInfo->isAuthenticated(), clientInfo->getBadges(), false, clientInfo->isAdmin(), 
                clientInfo->isLevelChanger(), clientInfo->isRobot(), clientInfo->isSpawnDelayed(), clientInfo->isBusy(), true, true);


   if(clientInfo->getTeamIndex() >= 0) 
      s2cClientJoinedTeam(clientInfo->getName(), clientInfo->getTeamIndex(), isTeamGame() && !isGameOver());

   spawnShip(clientInfo);
}


// Determines who can damage what.  Can be overridden by individual games.  Currently only overridden by Rabbit.
bool GameType::objectCanDamageObject(BfObject *damager, BfObject *victim)
{
   if(!damager)            // Anonomyous projectiles are deadly to all!
      return true;


   U8 typeNumber = damager->getObjectTypeNumber();

   // Asteroids always do damage
   if(typeNumber == AsteroidTypeNumber)
      return true;

   WeaponType weaponType;

   if(typeNumber == BulletTypeNumber)
      weaponType = static_cast<Projectile *>(damager)->mWeaponType;
   else if(typeNumber == BurstTypeNumber || typeNumber == MineTypeNumber || typeNumber == SpyBugTypeNumber)
      weaponType = static_cast<Burst *>(damager)->mWeaponType;
   else if(typeNumber == SeekerTypeNumber)
      weaponType = static_cast<Seeker *>(damager)->mWeaponType;
   else
   {
      TNLAssert(false, "Unknown Damage type");
      return false;
   }

   ClientInfo *damagerOwner = damager->getOwner();

   // Check for self-inflicted damage
   if(damagerOwner && damagerOwner == victim->getOwner())
      return GameWeapon::weaponInfo[weaponType].damageSelfMultiplier != 0;

   // Check for friendly fire
   else if(damager->getTeam() == victim->getTeam())
      return !isTeamGame() || GameWeapon::weaponInfo[weaponType].canDamageTeammate;

   return true;
}


// Handle scoring when ship is killed
void GameType::controlObjectForClientKilled(ClientInfo *victim, BfObject *clientObject, BfObject *killerObject)
{
   if(isGameOver())  // Avoid flooding messages on game over
      return;

   ClientInfo *killer = killerObject ? killerObject->getOwner() : NULL;

   if(!victim)
      return;        // Do nothing; it's probably a "Ship 0 0 0" in a level, where there is no ClientInfo

   victim->getStatistics()->addDeath();

   StringTableEntry killerDescr = killerObject->getKillString();

   if(killer)     // Known killer
   {
      if(killer == victim)    // We killed ourselves -- should have gone easy with the bouncers!
      {
         killer->getStatistics()->addSuicide();
         updateScore(killer, KillSelf);
      }

      // Should do nothing with friendly fire disabled
      else if(isTeamGame() && killer->getTeamIndex() == victim->getTeamIndex())   // Same team in a team game
      {
         killer->getStatistics()->addFratricide();
         updateScore(killer, KillTeammate);
      }

      else                                                                        // Different team, or not a team game
      {
         killer->getStatistics()->addKill();
         updateScore(killer, KillEnemy);
      }

      s2cKillMessage(victim->getName(), killer->getName(), killerObject->getKillString());
   }
   else              // Unknown killer... not a scorable event.  Unless killer was an asteroid!
   {
      if(killerObject->getObjectTypeNumber() == AsteroidTypeNumber)       // Asteroid
         updateScore(victim, KilledByAsteroid, 0);
      else if(U32(killerObject->getTeam()) < U32(getGame()->getTeamCount()) && isTeamGame()) // We may have a non-hostile killer team we can use to give credit.
      {
         updateScore(killerObject->getTeam(), clientObject->getTeam() == killerObject->getTeam() ? KillTeammate : KillEnemy);
      }
      else                                       // Check for turret shot - Can get here if turret is Hostile Team
      {
         BfObject *shooter = NULL;
         if(killerObject->getObjectTypeNumber() == BulletTypeNumber)
            shooter = static_cast<Projectile *>(killerObject)->mShooter;
         if(killerObject->getObjectTypeNumber() == BurstTypeNumber)
            shooter = static_cast<Burst *>(killerObject)->mShooter;
         if(killerObject->getObjectTypeNumber() == SeekerTypeNumber)
            shooter = static_cast<Seeker *>(killerObject)->mShooter;

         if(shooter && shooter->getObjectTypeNumber() == TurretTypeNumber)
            updateScore(victim, KilledByTurret, 0);
      }

      s2cKillMessage(victim->getName(), NULL, killerDescr);
   }

   victim->respawnTimer.reset(RespawnDelay);
}


// Handle score for ship and robot
// Runs on server only?
void GameType::updateScore(Ship *ship, ScoringEvent scoringEvent, S32 data)
{
   TNLAssert(ship, "Ship should never be NULL here!!");
   updateScore(ship->getClientInfo(), ship->getTeam(), scoringEvent, data);
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
      TNLAssert(points != naScore, "Bad score value");

      if(points != 0)
      {
         player->addScore(points);
         newScore = player->getScore();

         // Broadcast player scores for rendering on the client
         if(!isTeamGame())
            for(S32 i = 0; i < mGame->getClientCount(); i++)  // TODO: try to get rid of this for loop
               if(getGame()->getClientInfo(i) == player)      // and this pointer checks (we need to get the index, for veriable "i")
                  s2cSetPlayerScore(i, player->getScore());
      }
   }

   if(isTeamGame())
   {
      // Just in case...  completely superfluous, gratuitous check
      if(U32(teamIndex) >= U32(mGame->getTeamCount()))
         return;

      S32 points = getEventScore(TeamScore, scoringEvent, data);
      TNLAssert(points != naScore, "Bad score value");
      if(points == 0)
         return;

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

      // Now add the score
      Team *team = (Team *)mGame->getTeam(teamIndex);
      team->addScore(points);
      s2cSetTeamScore(teamIndex, team->getScore());     // Broadcast new team score

      updateLeadingTeamAndScore();
      newScore = mLeadingTeamScore;
   }

   if(newScore >= mWinningScore)        // End game if max score has been reached
      gameOverManGameOver();
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
      S32 digits = score == 0 ? 1 : (S32(log10(F32(abs(score)))) + ((score < 0 && getGameTypeId() != CoreGame) ? 2 : 1));

      mDigitsNeededToDisplayScore = MAX(digits, mDigitsNeededToDisplayScore);

      if(score > mLeadingTeamScore)
      {
         mLeadingTeamScore = ((Team *)(mGame->getTeam(i)))->getScore();
         mLeadingTeam = i;
      }
   }
}


// Sets mLeadingTeamScore and mLeadingTeam; runs on client only
void GameType::updateLeadingPlayerAndScore()
{
   mLeadingPlayerScore = S32_MIN;
   mLeadingPlayer = -1;
   mSecondLeadingPlayerScore = S32_MIN;
   mSecondLeadingPlayer = -1;

   // Find the leading player
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      // Check to make sure client hasn't disappeared somehow
      if(!mGame->getClientInfo(i))
         continue;

      S32 score = mGame->getClientInfo(i)->getScore();

      if(score > mLeadingPlayerScore)
      {
         // Demote leading player to 2nd place
         mSecondLeadingPlayerScore = mLeadingPlayerScore;
         mSecondLeadingPlayer = mLeadingPlayer;

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
      mGame->getClientInfo(i)->endOfGameScoringHandler();
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
      BfObject *controlObject = game->getConnectionToServer()->getControlObject();
      if(!controlObject || !isShipType(controlObject->getObjectTypeNumber()))
         return;

      Ship *ship = static_cast<Ship *>(controlObject);   // Returns player's ship...

      gt->c2sChangeTeams(1 - ship->getTeam());           // If two teams, team will either be 0 or 1, so "1 - " will toggle
      game->getUIManager()->reactivate(GameUI);          // Jump back into the game (this option takes place immediately)
   }
   else
   {
      // Show menu to let player select a new team
      TeamMenuUserInterface *ui = game->getUIManager()->getTeamMenuUserInterface();
      ui->nameToChange = game->getClientInfo()->getName();
      game->getUIManager()->activate(ui);                  
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
   game->getUIManager()->activate(ui);
}


// Add any additional game-specific admin menu items, processed below
void GameType::addAdminGameMenuOptions(MenuUserInterface *menu)
{
   ClientGame *game = static_cast<ClientGame *>(mGame);

   if(isTeamGame() && game->getTeamCount() > 1)
      menu->addMenuItem(new MenuItem("CHANGE A PLAYER'S TEAM", switchPlayersTeamCallback, "", KEY_C));
}
#endif


// Broadcast info about the current level... code gets run on client, obviously
// Note that if we add another arg to this, we need to further expand FunctorDecl methods in tnlMethodDispatch.h
// Also serves to tell the client we're on a new level.
GAMETYPE_RPC_S2C(GameType, s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, 
                                                StringTableEntry levelCreds, S32 objectCount, F32 lx, F32 ly, F32 ux, F32 uy, 
                                                bool levelHasLoadoutZone, bool engineerEnabled, bool engineerAbuseEnabled),
                                            (levelName, levelDesc, teamScoreLimit, 
                                                levelCreds, objectCount, lx, ly, ux, uy, 
                                                levelHasLoadoutZone, engineerEnabled, engineerAbuseEnabled))
{
#ifndef ZAP_DEDICATED
   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   clientGame->FXManager::clearSparks();

   mLevelName = levelName;
   mLevelDescription = levelDesc;
   mLevelCredits = levelCreds;

   mWinningScore = teamScoreLimit;
   mObjectsExpected = objectCount;

   mEngineerEnabled = engineerEnabled;
   mEngineerUnrestrictedEnabled = engineerAbuseEnabled;

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


GAMETYPE_RPC_C2S(GameType, c2sSetTime, (U32 time), (time))
{
   processClientRequestForChangingGameTime(time, time == 0, true, 2);
}


GAMETYPE_RPC_C2S(GameType, c2sAddTime, (U32 time), (time))
{
   processClientRequestForChangingGameTime(time += mGameTimer.getCurrent(), false, false, 1);
}


void GameType::processClientRequestForChangingGameTime(S32 time, bool isUnlimited, bool changeTimeIfAlreadyUnlimited, S32 voteType)
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())  // Extra check in case of hacked client
      return;

   // No setting time after game is over...
   if(mGameOver)
      return;

   // Use voting when no level change password and more then 1 players
   if(!clientInfo->isAdmin() && 
         getGame()->getSettings()->getLevelChangePassword() == "" && 
         getGame()->getPlayerCount() > 1)
   {
      if(static_cast<ServerGame *>(getGame())->voteStart(clientInfo, voteType, time))     // Returns true if handled
         return;
   }


   if(isUnlimited)
      setTimeRemaining(MAX_GAME_TIME, true);
   else if(changeTimeIfAlreadyUnlimited || !mGameTimer.isUnlimited())
      setTimeRemaining(time, false);
   else
      return;

   broadcastNewRemainingTime();

   static const StringTableEntry msg("%e0 has changed the game time");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sChangeTeams, (S32 team), (team))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());
   ClientInfo *clientInfo = source->getClientInfo();

   if(isGameOver()) // No changing team while on game over
      return;

   if(!clientInfo->isAdmin() && source->mSwitchTimer.getCurrent())    // If we're not admin and we're waiting for our switch-expiration to reset,
      return;                                                     // return without processing the change team request

   // Vote to change team might have different problems than the old way...
   if( (!clientInfo->isLevelChanger() || getGame()->getSettings()->getLevelChangePassword() == "") && 
        getGame()->getPlayerCount() > 1 )
   {
      if(((ServerGame *)getGame())->voteStart(clientInfo, 4, team))
         return;
   }

   // Don't let spawn delay kick in for caller.  This prevents a race condition with spawn undelay and becoming unbusy
   source->resetTimeSinceLastMove();

   changeClientTeam(clientInfo, team);

   if(!clientInfo->isAdmin() && getGame()->getPlayerCount() > 1)
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

   TNLAssert(client->isRobot() || client->getConnection()->getControlObject() == client->getShip(), "Not equal?!?");
   Ship *ship = client->getShip();    // Get the ship that's switching

   if(ship)
   {
      // Find all spybugs and mines that this player owned, and reset ownership
      fillVector.clear();
      mGame->getGameObjDatabase()->findObjects((TestFunc)isGrenadeType, fillVector);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         BfObject *obj = static_cast<BfObject *>(fillVector[i]);

         if((obj->getOwner()) == ship->getOwner())
            obj->setOwner(NULL);
      }

      if(ship->isRobot())              // Players get a new ship, robots reuse the same object
         ship->setChangeTeamMask();
      else
         client->respawnTimer.clear(); // If we've just died, this will keep a second copy of ourselves from appearing

      ship->kill();           // Destroy the old ship
   }

   if(team < 0)                                                                     // If no team provided...
      client->setTeamIndex((client->getTeamIndex() + 1) % mGame->getTeamCount());   // ...find the next one...
   else                                                                             // ...otherwise...
      client->setTeamIndex(team);                                                   // ...use the one provided

   if(client->getTeamIndex() >= 0)                                                  // But if we know the team...
      s2cClientJoinedTeam(client->getName(), client->getTeamIndex(), !isGameOver()); // ...announce the change

   spawnShip(client);                                                               // Create a new ship

   if(!client->isRobot())
      client->getConnection()->switchedTeamCount++;                                 // Track number of times the player switched teams
}


// A player (either us or a remote player) has joined the game.  This will also be called for all players (including us) when changing levels.
// This suggests that RemoteClientInfos are not retained from game to game, but are generated anew.
// ** Note that this method is essentially a mechanism for passing clientInfos from server to client. **
GAMETYPE_RPC_S2C(GameType, s2cAddClient, 
                (StringTableEntry name, bool isAuthenticated, Int<BADGE_COUNT> badges, bool isLocalClient, 
                 bool isAdmin, bool isLevelChanger, bool isRobot, bool isSpawnDelayed, bool isBusy, bool playAlert, bool showMessage),
                (name, isAuthenticated, badges, isLocalClient, isAdmin, isLevelChanger, isRobot, isSpawnDelayed, isBusy, playAlert, showMessage))
{
#ifndef ZAP_DEDICATED

   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   // The new ClientInfo will be deleted in s2cRemoveClient   
   ClientInfo *clientInfo = new RemoteClientInfo(clientGame, name, isAuthenticated, badges, isRobot, isAdmin, isLevelChanger, isSpawnDelayed, isBusy);

   clientGame->onPlayerJoined(clientInfo, isLocalClient, playAlert, showMessage);

#endif
}


// Remove a client from the game
// Server only
void GameType::serverRemoveClient(ClientInfo *clientInfo)
{
   if(clientInfo->getConnection())
   {
      // Blow up the ship...
      TNLAssert(clientInfo->getConnection()->getControlObject() == clientInfo->getShip(), "Not equal?!?");

      Ship *ship = clientInfo->getShip();
      if(ship)
         ship->kill();
   }

   s2cRemoveClient(clientInfo->getName());            // Tell other clients that this one has departed

   getGame()->removeFromClientList(clientInfo);   

   // Note that we do not need to delete clientConnection... TNL handles that, and the destructor gets runs shortly after we get here

   static_cast<ServerGame *>(getGame())->suspendIfNoActivePlayers();
}


// This method should only be used for periodic updates, not for events that change the actual time left in the game
void GameType::syncTimeRemaining(U32 timeLeft)
{
   mGameTimer.sync(timeLeft);
}


void GameType::setTimeRemaining(U32 timeLeft, bool isUnlimited)
{
   TNLAssert(getGame()->isServer(), "This should only run on the server!");

   U32 offset = mGameTimer.getCurrent() + mGameTimer.getRenderingOffset() - timeLeft;

   mGameTimer.reset(timeLeft);
   mGameTimer.setIsUnlimited(isUnlimited);
   mGameTimer.setRenderingOffset(offset);
}


// Game time has changed -- need to do an update
void GameType::setTimeRemaining(U32 timeLeft, bool isUnlimited, S32 renderingOffset)
{
   TNLAssert(!getGame()->isServer(), "This should only run on the client!");

   mGameTimer.reset(timeLeft);
   mGameTimer.setIsUnlimited(isUnlimited);
   mGameTimer.setRenderingOffset(renderingOffset);
}


// Send remaining time to all clients -- used for synchronization only
void GameType::broadcastTimeSyncSignal()
{
   s2cSyncTimeRemaining(mGameTimer.getCurrent());
}


// Send remaining time to all clients after time has been updated
void GameType::broadcastNewRemainingTime()
{
   s2cSetNewTimeRemaining(mGameTimer.getCurrent(), mGameTimer.isUnlimited(), mGameTimer.getRenderingOffset());
}


// Server notifies clients that a player has changed name
GAMETYPE_RPC_S2C(GameType, s2cRenameClient, (StringTableEntry oldName, StringTableEntry newName), (oldName, newName))
{
#ifndef ZAP_DEDICATED
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      if(clientInfo->getName() == oldName)
      {
         clientInfo->setName(newName);
         clientInfo->setAuthenticated(false, NO_BADGES);
         break;
      }
   }

   // Notifiy the player
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);
   clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "An admin as renamed %s to %s", oldName.getString(), newName.getString());
#endif
}

// Tell all clients name has changed, and update server side name
// Server only
void GameType::updateClientChangedName(ClientInfo *clientInfo, StringTableEntry newName)
{
   logprintf(LogConsumer::LogConnection, "Name changed from %s to %s", clientInfo->getName().getString(), newName.getString());

   s2cRenameClient(clientInfo->getName(), newName);
   clientInfo->setName(newName);
}


// Server has notified us that a player has left the game
GAMETYPE_RPC_S2C(GameType, s2cRemoveClient, (StringTableEntry name), (name))
{
#ifndef ZAP_DEDICATED
   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

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
GAMETYPE_RPC_S2C(GameType, s2cSyncTimeRemaining, (U32 timeLeftInMs), (timeLeftInMs))
{
   syncTimeRemaining(timeLeftInMs);
}


// Server has sent us (the client) a message telling us how much longer we have in the current game
GAMETYPE_RPC_S2C(GameType, s2cSetNewTimeRemaining, (U32 timeLeftInMs, bool isUnlimited, S32 renderingOffset), (timeLeftInMs, isUnlimited, renderingOffset))
{
   setTimeRemaining(timeLeftInMs, isUnlimited, renderingOffset);
}


// Server has sent us (the client) a message telling us the winning score has changed, and who changed it
GAMETYPE_RPC_S2C(GameType, s2cChangeScoreToWin, (U32 winningScore, StringTableEntry changer), (winningScore, changer))
{
#ifndef ZAP_DEDICATED
   mWinningScore = winningScore;

   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   clientGame->displayMessage(Color(0.6f, 1, 0.8f) /*Nuclear green */, 
               "%s changed the winning score to %d.", changer.getString(), mWinningScore);
#endif
}


// Announce a new player has joined the team
GAMETYPE_RPC_S2C(GameType, s2cClientJoinedTeam, 
                (StringTableEntry name, RangedU32<0, GameType::MAX_TEAMS> teamIndex, bool showMessage), 
                (name, teamIndex, showMessage))
{
#ifndef ZAP_DEDICATED
   ClientInfo *remoteClientInfo = mGame->findClientInfo(name);      // Get RemoteClientInfo for player changing teams
   if(!remoteClientInfo)
      return;

   remoteClientInfo->setTeamIndex((S32) teamIndex);

   // The following works as long as everyone runs with a unique name.  Fails if two players have names that collide and have
   // been corrected by the server.
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);
   ClientInfo *localClientInfo = clientGame->getClientInfo();

   if(showMessage)
   {
      if(localClientInfo->getName() == name)      
         clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "You have joined team %s.", mGame->getTeamName(teamIndex).getString());
      else
         clientGame->displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined team %s.", name.getString(), mGame->getTeamName(teamIndex).getString());
   }

   // Make this client forget about any mines or spybugs he knows about... it's a bit of a kludge to do this here,
   // but this RPC only runs when a player joins the game or changes teams, so this will never hurt, and we can
   // save the overhead of sending a separate message which, while theoretically cleaner, will never be needed practically.
   fillVector.clear();
   clientGame->getGameObjDatabase()->findObjects((TestFunc)isGrenadeType, fillVector);

   for(S32 i = 0; i < fillVector.size(); i++)
      static_cast<Burst *>(fillVector[i])->mIsOwnedByLocalClient = false;
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
   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   if(clientGame->getClientInfo()->getName() != name)    // Don't show message to self
      clientGame->displayMessage(Colors::cyan, "%s has been granted administrator access.", name.getString());
#endif
}


// Announce a new player has permission to change levels
GAMETYPE_RPC_S2C(GameType, s2cClientBecameLevelChanger, (StringTableEntry name, bool isLevelChanger), (name, isLevelChanger))
{
#ifndef ZAP_DEDICATED
   // Get a RemoteClientInfo representing the client that just became a level changer
   ClientInfo *clientInfo = mGame->findClientInfo(name);      
   if(!clientInfo)
      return;

   // Record that fact in our local copy of info about them
   clientInfo->setIsLevelChanger(isLevelChanger);

   if(isLevelChanger)
   {
      // Now display a message to the local client, unless they were the ones who were granted the privs, in which case they already
      // saw a different message.
      TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
      ClientGame *clientGame = static_cast<ClientGame *>(mGame);

      if(clientGame->getClientInfo()->getName() != name)    // Don't show message to self
         clientGame->displayMessage(Colors::cyan, "%s can now change levels.", name.getString());
   }
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
                   mLevelHasLoadoutZone, mEngineerEnabled, mEngineerUnrestrictedEnabled);

   for(S32 i = 0; i < mGame->getTeamCount(); i++)
   {
      Team *team = (Team *)mGame->getTeam(i);
      const Color *color = team->getColor();

      s2cAddTeam(team->getName(), color->r, color->g, color->b, team->getScore(), i == 0);
   }

   notifyClientsWhoHasTheFlag();

   // Add all the client and team information
   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      GameConnection *conn = clientInfo->getConnection();

      bool isLocalClient = (conn == theConnection);

      s2cAddClient(clientInfo->getName(), clientInfo->isAuthenticated(), clientInfo->getBadges(), isLocalClient, 
                   clientInfo->isAdmin(), clientInfo->isLevelChanger(), clientInfo->isRobot(), clientInfo->isSpawnDelayed(),
                   clientInfo->isBusy(), false, false);

      S32 team = clientInfo->getTeamIndex();

      if(team >= 0) 
         s2cClientJoinedTeam(clientInfo->getName(), team, false);
   }

   //for(S32 i = 0; i < Robot::getBotCount(); i++)  //Robot is part of mClientList
   //{
   //   s2cAddClient(Robot::robots[i]->getName(), false, false, true, false);
   //   s2cClientJoinedTeam(Robot::robots[i]->getName(), Robot::robots[i]->getTeam());
   //}

   // Sending an empty list clears the barriers
   Vector<F32> v;
   s2cAddWalls(v, 0, false);

   for(S32 i = 0; i < mWalls.size(); i++)
      s2cAddWalls(mWalls[i].verts, mWalls[i].width, mWalls[i].solid);

   broadcastNewRemainingTime();
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

   TNLAssert(dynamic_cast<ClientGame *>(mGame) != NULL, "Not a ClientGame"); // If this asserts, need to revert to dynamic_cast with NULL check
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);


   clientGame->computeWorldObjectExtents();          // Make sure our world extents reflect all the objects we've loaded
   Barrier::prepareRenderingGeometry(clientGame);    // Get walls ready to render

   clientGame->getUIManager()->getGameUserInterface()->mShowProgressBar = false;
   //clientGame->setInCommanderMap(false);          // Start game in regular mode, If we change here, need to tell the server we are in this mode. Map can change while in commander map.
   //clientGame->clearZoomDelta();                  // No in zoom effect
   
   clientGame->getUIManager()->getGameUserInterface()->mProgressBarFadeTimer.reset(1000);

   // Prepare scoreboard
   updateLeadingPlayerAndScore();

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
   // Empty wall deletes all existing walls
   if(!verts.size())
      mGame->deleteObjects((TestFunc)isWallType);
   else
   {
      WallRec wall(width, solid, verts);
      wall.constructWalls(mGame);
   }
}


extern void writeServerBanList(CIniFile *ini, BanList *banList);

// Runs the server side commands, which the client may or may not know about

// This is server side commands, For client side commands, use UIGame.cpp, GameUserInterface::processCommand.
// When adding new commands, please update the giant CommandInfo chatCmds[] array in UIGame.cpp)
void GameType::processServerCommand(ClientInfo *clientInfo, const char *cmd, Vector<StringPtr> args)
{
   ServerGame *serverGame = static_cast<ServerGame *>(mGame);

   if(!stricmp(cmd, "yes"))
      serverGame->voteClient(clientInfo, true);
   else if(!stricmp(cmd, "no"))
      serverGame->voteClient(clientInfo, false);
   else if(!stricmp(cmd, "loadini") || !stricmp(cmd, "loadsetting"))
   {
      if(clientInfo->isAdmin())
      {
         bool prev_enableServerVoiceChat = serverGame->getSettings()->getIniSettings()->enableServerVoiceChat;
         loadSettingsFromINI(&gINI, serverGame->getSettings());;
         if(prev_enableServerVoiceChat != serverGame->getSettings()->getIniSettings()->enableServerVoiceChat)
            for(S32 i = 0; i < mGame->getClientCount(); i++)
               if(!mGame->getClientInfo(i)->isRobot())
               {
                  GameConnection *gc = mGame->getClientInfo(i)->getConnection();
                  gc->s2rVoiceChatEnable(serverGame->getSettings()->getIniSettings()->enableServerVoiceChat && !gc->mChatMute);
               }
         clientInfo->getConnection()->s2cDisplayMessage(0, 0, "Configuration settings loaded");
      }
      else
         clientInfo->getConnection()->s2cDisplayErrorMessage("!!! Need admin");
   }
   else
      clientInfo->getConnection()->s2cDisplayErrorMessage("!!! Invalid Command");
}

// Server only
void GameType::balanceTeams()
{
   // All clients, players + bots
   S32 currentClientCount = getGame()->getClientCount();
   S32 currentBotCount = getGame()->getBotCount();

   S32 minimumPlayersNeeded = getGame()->getSettings()->getIniSettings()->minBalancedPlayers;

   // Not enough players!  Add bots until we're balanced
   if(currentClientCount < minimumPlayersNeeded)
   {
      Vector<StringTableEntry> dummy;
      for(S32 i = 0; i < minimumPlayersNeeded - currentClientCount; i++)
         addBot(dummy);
   }

   // We have more than enough players, kick bots to keep balance
   if(currentClientCount > minimumPlayersNeeded && currentBotCount > 0)
   {
      // Re-evaluate teams.  For some reason getPlayerBotCount() isn't correct unless this is done
      getGame()->countTeamPlayers();

      for(S32 i = 0; i < currentBotCount; i++)
      {
         // Find the team with the most players on it
         S32 mostPlayersTeamIndex = 0;
         S32 mostPlayers = 0;

         for(S32 i = 0; i < mGame->getTeamCount(); i++)
         {
            TNLAssert(dynamic_cast<Team *>(mGame->getTeam(i)), "Invalid team");
            S32 currentCount = static_cast<Team *>(mGame->getTeam(i))->getPlayerBotCount();

            if(currentCount > mostPlayers)
            {
               mostPlayers = currentCount;
               mostPlayersTeamIndex = i;
            }
         }

         // Now kick a bot from that team.  Is there a better / more efficent way to do this?
         for(S32 i = 0; i < mGame->getClientCount(); i++)
         {
            ClientInfo *clientInfo = mGame->getClientInfo(i);
            if(clientInfo->isRobot() && clientInfo->getTeamIndex() == mostPlayersTeamIndex)
            {
               getGame()->deleteBot(clientInfo->getName());
               break;  // Just one!
            }
         }

         // Re-evaluate client count; don't kick any more bots than we need to
         if(getGame()->getClientCount() <= minimumPlayersNeeded)
            break;
      }
   }
}

// Server only
string GameType::addBot(Vector<StringTableEntry> args)
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

   string errorMessage;
   if(!robot->processArguments(args_count, args_char, mGame, errorMessage))
   {
      delete robot;
      return "!!! " + errorMessage;
   }

   robot->addToGame(mGame, mGame->getGameObjDatabase());

   if(!robot->start())
   {
      delete robot;
      return "!!! Could not start robot; please see server logs";
   }

   serverAddClient(robot->getClientInfo());

   return "";
}


void GameType::addBotFromClient(Vector<StringTableEntry> args)
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = getGame()->getSettings();

   GameConnection *conn = clientInfo->getConnection();

   static const S32 ABSOLUTE_MAX_BOTS = 255;    // Hard limit on number of bots on the server

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

   else if((getGame()->getBotCount() >= settings->getIniSettings()->maxBots && !clientInfo->isAdmin()) ||
           (getGame()->getBotCount() >= ABSOLUTE_MAX_BOTS))
      conn->s2cDisplayErrorMessage("!!! Can't add more bots -- this server is full");

   else if(args.size() >= 2 && !safeFilename(args[1].getString()))
      conn->s2cDisplayErrorMessage("!!! Invalid filename");

   else
   {
      string errorMessage = addBot(args);

      if(errorMessage != "")
         conn->s2cDisplayErrorMessage(errorMessage);
      else
      {
         // Disable bot balancing
         mBotBalancingDisabled = true;

         StringTableEntry msg = StringTableEntry("Robot added by %e0");
         Vector<StringTableEntry> e;
         e.push_back(clientInfo->getName());

         broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
      }
   }
}


GAMETYPE_RPC_C2S(GameType, c2sAddBot,
      (Vector<StringTableEntry> args),
      (args))
{
   addBotFromClient(args);
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

   while(count > 0 && prevRobotSize != getGame()->getBotCount()) // loop may end when cannot add anymore bots
   {
      count--;
      prevRobotSize = getGame()->getBotCount();
      addBotFromClient(args);
   }
}


GAMETYPE_RPC_C2S(GameType, c2sSetWinningScore, (U32 score), (score))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();
   GameSettings *settings = getGame()->getSettings();

   // Level changers and above
   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side


   if(score <= 0)    // i.e. score is invalid
      return;  // Error message handled client-side

   ServerGame *serverGame = static_cast<ServerGame *>(mGame);

   // No changing score in Core
   if(serverGame->getGameType()->getGameTypeId() == CoreGame)
      return;

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

   ServerGame *serverGame = static_cast<ServerGame *>(mGame);

   // No changing score in Core
   if(serverGame->getGameType()->getGameTypeId() == CoreGame)
      return;

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
   }

   StringTableEntry msg("%e0 has reset the score of the game");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sKickBot, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   if(!clientInfo->isLevelChanger())
      return;  // Error message handled client-side

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   S32 botCount = getGame()->getBotCount();

   if(botCount == 0)
   {
      conn->s2cDisplayErrorMessage("!!! There are no robots to kick");
      return;
   }

   // Only delete one robot - the most recently added
   getGame()->deleteBot(botCount - 1);

   // Disable bot balancing
   mBotBalancingDisabled = true;

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

   if(getGame()->getBotCount() == 0)
   {
      conn->s2cDisplayErrorMessage("!!! There are no robots to kick");
      return;
   }

   // Delete all bots
   getGame()->deleteAllBots();

   // Disable bot balancing
   mBotBalancingDisabled = true;

   StringTableEntry msg = StringTableEntry("All robots kicked by %e0");
   Vector<StringTableEntry> e;
   e.push_back(clientInfo->getName());

   broadcastMessage(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sShowBots, (), ())
{
   if(!getGame()->isTestServer())   // Error message for this condition already displayed on client; this is just a quick hack check
      return;

   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = source->getClientInfo();

   // Show all robots affects all players
   mShowAllBots = !mShowAllBots;  // Toggle

   GameConnection *conn = clientInfo->getConnection();
   TNLAssert(conn == source, "If this never fires, we can get rid of conn!");

   if(getGame()->getBotCount() == 0)
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
   GameSettings *settings = getGame()->getSettings();

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
   GameSettings *settings = getGame()->getSettings();

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
   if(bannedClientInfo->isRobot())
      return;  // Error message handled client-side

   Address ipAddress = bannedClientInfo->getConnection()->getNetAddress();

   S32 banDuration = duration == 0 ? settings->getBanList()->getDefaultBanDuration() : duration;

   // Add the ban
   settings->getBanList()->addToBanList(ipAddress, banDuration, !bannedClientInfo->isAuthenticated());  // non-authenticated only if player is not authenticated
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
   GameConnection *conn = (GameConnection *) getRPCSourceConnection();
   ClientInfo *clientInfo = conn->getClientInfo();
   GameSettings *settings = getGame()->getSettings();

   if(!clientInfo->isAdmin())
      return;  // Error message handled client-side

   Address ipAddress(ipAddressString.getString());

   if(!ipAddress.isValid())
      return;  // Error message handled client-side

   S32 banDuration = duration == 0 ? settings->getBanList()->getDefaultBanDuration() : duration;

   // Now check to see if the client is connected and disconnect all of them if they are
   bool playerDisconnected = false;

   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      GameConnection *baneeConn = mGame->getClientInfo(i)->getConnection();

      if(baneeConn && baneeConn->getNetAddress().isEqualAddress(ipAddress))
      {
         if(baneeConn == conn)
         {
            conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Can't ban yourself");
            return;
         }

         baneeConn->disconnect(NetConnection::ReasonBanned, "");
         playerDisconnected = true;
      }
   }

   // banip should always add to the ban list, even if the IP isn't connected
   settings->getBanList()->addToBanList(ipAddress, banDuration, true);  // most problem comes from non-authenticated users
   logprintf(LogConsumer::ServerFilter, "%s - banned for %d minutes", ipAddress.toString(), banDuration);

   // Save BanList in memory
   writeServerBanList(&gINI, settings->getBanList());

   // Save new INI settings to disk
   gINI.WriteFile();


   if(!playerDisconnected)
      conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Client has been banned but is no longer connected");
   else
   {
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
   renamedClientInfo->setName("");                          // Avoid unique self
   StringTableEntry uniqueName = getGame()->makeUnique(newName.getString()).c_str();  // New name
   renamedClientInfo->setName(oldName);                     // Restore name to properly get it updated to clients
   renamedClientInfo->setAuthenticated(false, NO_BADGES);   // Don't underline anymore because of rename
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

   ClientInfo *targetClientInfo = mGame->findClientInfo(playerName);

   if(!targetClientInfo)
      return;  // Error message handled client-side

   if(targetClientInfo->isAdmin())
      return;  // Error message handled client-side

   GameConnection *gc = targetClientInfo->getConnection();

   if(!gc)
      return;

   // Toggle
   gc->mChatMute = !gc->mChatMute;

   // if server voice chat is allowed, send voice chat status.
   if(getGame()->getSettings()->getIniSettings()->enableServerVoiceChat)
      gc->s2rVoiceChatEnable(!gc->mChatMute);

   GameConnection *conn = clientInfo->getConnection();

   conn->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, gc->mChatMute ? "Player is muted" : "Player is unmuted");
}


GAMETYPE_RPC_C2S(GameType, c2sClearScriptCache, (), ())
{
   LuaScriptRunner::clearScriptCache();
   ClientInfo *clientInfo = ((GameConnection *) getRPCSourceConnection())->getClientInfo();

   if(!clientInfo->isAdmin())    // Error message handled client-side
      return;  

   clientInfo->getConnection()->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "Script cache cleared; scripts will be reloaded on next use");
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

   if(!playerClientInfo->isRobot())
      playerClientInfo->getConnection()->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "An admin has shuffled you to a different team");
}



GAMETYPE_RPC_C2S(GameType, c2sKickPlayer, (StringTableEntry kickeeName), (kickeeName))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientInfo *sourceClientInfo = source->getClientInfo();

   if(!sourceClientInfo->isAdmin())
      return;

   ClientInfo *kickee = mGame->findClientInfo(kickeeName);    

   if(!kickee)    // Hmmm... couldn't find the dude.  Maybe he disconnected?
      return;

   if(!kickee->isRobot())
   {
      if(kickee->isAdmin())
      {
         source->s2cDisplayErrorMessage("Can't kick an administrator!");
         return;
      }

      if(kickee->getConnection()->isEstablished())
      {
         GameConnection *kickeeConnection = kickee->getConnection();
         ConnectionParameters &p = kickeeConnection->getConnectionParameters();
         BanList *banList = getGame()->getSettings()->getBanList();

         if(p.mIsArranged)
            banList->kickHost(p.mPossibleAddresses[0]);           // Banned for 30 seconds

         banList->kickHost(kickeeConnection->getNetAddress());    // Banned for 30 seconds
         kickeeConnection->disconnect(NetConnection::ReasonKickedByAdmin, "");
      }
   }
   
   // Get rid of robots that have the to-be-kicked name
   getGame()->deleteBot(kickeeName);

   Vector<StringTableEntry> e;
   e.push_back(kickeeName);                     
   e.push_back(sourceClientInfo->getName());    // --> Name of player doing the administering

   broadcastMessage(GameConnection::ColorAqua, SFXIncomingMessage, "%e0 was kicked from the game by %e1.", e);
}


GAMETYPE_RPC_C2S(GameType, c2sSendCommand, (StringTableEntry cmd, Vector<StringPtr> args), (cmd, args))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   processServerCommand(source->getClientInfo(), cmd.getString(), args);     // was conn instead of source...
}


//Send an announcement

TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sSendAnnouncement, (string message), (message), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhostParent, 1)
{
	GameConnection *source = (GameConnection *)getRPCSourceConnection();
	
	for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
		ClientInfo *clientInfo = mGame->getClientInfo(i);
		
		if(clientInfo->isRobot())
			continue;

		RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayAnnouncement, (message));
			
		source->postNetEvent(theEvent);
		clientInfo->getConnection()->postNetEvent(theEvent);
	}
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

      // No sending to bots - they don't have a game connection
      if(clientInfo->isRobot())
         continue;

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

   sendChat(sourceClientInfo->getName(), sourceClientInfo, message, global);
}


void GameType::sendChatFromRobot(bool global, const StringPtr &message, ClientInfo *botClientInfo)
{
   sendChat(botClientInfo->getName(), botClientInfo, message, global);
}


// For the moment, all Controller chats are global
void GameType::sendChatFromController(const StringPtr &message)
{
   sendChat("LevelController", NULL, message, true);
}


// Sends a quick-chat message (which, due to its repeated nature can be encapsulated in a StringTableEntry item)
GAMETYPE_RPC_C2S(GameType, c2sSendChatSTE, (bool global, StringTableEntry message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   if(!source->checkMessage(message.getString(), global ? 0 : 1))
      return;

   ClientInfo *sourceClientInfo = source->getClientInfo();
   sendChat(sourceClientInfo->getName(), sourceClientInfo, message.getString(), global);
}


// Send a chat message that will be displayed in-game
// If not global, send message only to other players on team
// Note that sender may be NULL, if the message has been sent by a LevelController script
void GameType::sendChat(const StringTableEntry &senderName, ClientInfo *senderClientInfo, const StringPtr &message, bool global)
{
   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayChatMessage, (global, senderName, message));

   for(S32 i = 0; i < mGame->getClientCount(); i++)
   {
      ClientInfo *clientInfo = mGame->getClientInfo(i);
      S32 senderTeamIndex = senderClientInfo ? senderClientInfo->getTeamIndex() : NO_TEAM;

      if(global || clientInfo->getTeamIndex() == senderTeamIndex)
      {
         if(clientInfo->getConnection())
            clientInfo->getConnection()->postNetEvent(theEvent);
      }
   }

   // And fire an event handler...
   // But don't add event if called by robot - it is already called in Robot::globalMsg/teamMsg
   if(senderClientInfo && !senderClientInfo->isRobot())
      EventManager::get()->fireEvent(NULL, EventManager::MsgReceivedEvent, message, senderClientInfo->getPlayerInfo(), global);
}


extern Color gGlobalChatColor;
extern Color gTeamChatColor;


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cDisplayAnnouncement, (string message), (message), NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 1)
{
#ifndef ZAP_DEDICATED
	ClientGame* clientGame = static_cast<ClientGame *>(mGame);
	GameUserInterface* gameUI = clientGame->getUIManager()->getGameUserInterface();
#endif
}


// Server sends message to the client for display using StringPtr
GAMETYPE_RPC_S2C(GameType, s2cDisplayChatPM, (StringTableEntry fromName, StringTableEntry toName, StringPtr message), (fromName, toName, message))
{
#ifndef ZAP_DEDICATED

   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   ClientInfo *fullClientInfo = clientGame->getClientInfo();
   GameUserInterface *gameUI = clientGame->getUIManager()->getGameUserInterface();

   Color theColor = Colors::yellow;

   if(fullClientInfo->getName() == toName && toName == fromName)      // Message sent to self
      gameUI->onChatMessageRecieved(theColor, "%s: %s", toName.getString(), message.getString());

   else if(fullClientInfo->getName() == toName)                       // To this player
      gameUI->onChatMessageRecieved(theColor, "from %s: %s", fromName.getString(), message.getString());

   else if(fullClientInfo->getName() == fromName)                     // From this player
      gameUI->onChatMessageRecieved(theColor, "to %s: %s", toName.getString(), message.getString());

   else  
      TNLAssert(false, "Should never get here... shouldn't be able to see PM that is not from or not to you"); 
#endif
}


GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message), (global, clientName, message))
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   if(clientGame->isOnMuteList(clientName.getString()))
      return;

   Color *color = global ? &gGlobalChatColor : &gTeamChatColor;
   clientGame->getUIManager()->getGameUserInterface()->onChatMessageRecieved(*color, "%s: %s", clientName.getString(), message.getString());
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
GAMETYPE_RPC_C2S(GameType, c2sChooseNextWeapon, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   BfObject *controlObject = source->getControlObject();

   if(controlObject && isShipType(controlObject->getObjectTypeNumber()))
      static_cast<Ship *>(controlObject)->selectNextWeapon();
}


// Client tells server that they chose the next weapon
GAMETYPE_RPC_C2S(GameType, c2sChoosePrevWeapon, (), ())
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   BfObject *controlObject = source->getControlObject();

   if(controlObject && isShipType(controlObject->getObjectTypeNumber()))
      static_cast<Ship *>(controlObject)->selectPrevWeapon();
}


// Client tells server that they dropped flag or other item
GAMETYPE_RPC_C2S(GameType, c2sDropItem, (), ())
{
   //logprintf("%s GameType->c2sDropItem", isGhost()? "Client:" : "Server:");
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   BfObject *controlObject = source->getControlObject();

   if(!isShipType(controlObject->getObjectTypeNumber()))
      return;

   Ship *ship = static_cast<Ship *>(controlObject);

   S32 count = ship->getMountedItemCount();
   for(S32 i = count - 1; i >= 0; i--)
      ship->getMountedItem(i)->dismount(MountableItem::DISMOUNT_NORMAL);
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sResendItemStatus, (U16 itemId), (itemId), NetClassGroupGameMask, RPCGuaranteed, RPCToGhostParent, 0)
{
   if(mCacheResendItem.size() == 0)
      mCacheResendItem.resize(1024);

   for(S32 i = 0; i < 1024; i += 256)
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
         for(S32 j = 0; j < 1024; j += 256)
         {
            if(mCacheResendItem[S32(itemId & 255) | j].isNull())
               mCacheResendItem[S32(itemId & 255) | j].set(item);

            return;
         }
         mCacheResendItem[S32(itemId & 255) | (TNL::Random::readI(0, 3) * 256)].set(item);
         break;
      }
   }
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cAchievementMessage,
                            (U32 achievement, StringTableEntry clientName), (achievement, clientName),
                             NetClassGroupGameMask, RPCGuaranteed, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);

   string message = "";
   string textEffectText = "";

   if(achievement == BADGE_TWENTY_FIVE_FLAGS)
   {
      message = "%s has earned the TWENTY FIVE FLAGS badge!";
      textEffectText = "25 FLAGS BADGE";
   }
   else if(achievement == BADGE_ZONE_CONTROLLER)
   {
      message = "%s has earned the ZONE CONTROLLER badge!";
      textEffectText = "ZONE CONTROLLER";
   }
   else
      return;

   clientGame->displayMessage(Colors::yellow, message.c_str(), clientName.getString());
   SoundSystem::playSoundEffect(SFXAchievementEarned);

   Ship *ship = clientGame->findShip(clientName);
   if(ship)
      clientGame->emitTextEffect(textEffectText, Colors::yellow, ship->getRenderPos() + Point(0, 150));
#endif
}


// Client tells server that they chose the specified weapon
GAMETYPE_RPC_C2S(GameType, c2sSelectWeapon, (RangedU32<0, ShipWeaponCount> indx), (indx))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   BfObject *controlObject = source->getControlObject();

   if(controlObject && isShipType(controlObject->getObjectTypeNumber()))
      static_cast<Ship *>(controlObject)->selectWeapon(indx);
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
      mRatings.push_back(info->getCalculatedRating());
   }

   // Next come the robots ... Robots is part of mClientList
   //for(S32 i = 0; i < Robot::getBotCount(); i++)
   //{
   //   mPingTimes.push_back(0);
   //   mScores.push_back(Robot::robots[i]->getScore());
   //   mRatings.push_back(max(min((U32)(Robot::robots[i]->getRating() * 100.0) + 100, maxRating), minRating));
   //}

   NetObject::setRPCDestConnection(requestor->getConnection());
   s2cScoreboardUpdate(mPingTimes, mScores, mRatings);
   NetObject::setRPCDestConnection(NULL);
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cScoreboardUpdate,
                 (Vector<RangedU32<0, GameType::MaxPing> > pingTimes, Vector<SignedInt<24> > scores, Vector<SignedFloat<8> > ratings),
                 (pingTimes, scores, ratings), NetClassGroupGameMask, RPCGuaranteedOrderedBigData, RPCToGhost, 0)
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
   ClientGame *clientGame = static_cast<ClientGame *>(mGame);
   
   if(killer)  // Known killer, was self, robot, or another player
   {
      if(killer == victim)
         if(killerDescr == "mine")
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by own mine", victim.getString());
         else
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s zapped self", victim.getString());
      else
         if(killerDescr == "mine")
            clientGame->displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by mine laid by %s", victim.getString(), killer.getString());
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

   // If globally muted or voice chat is disabled on the server, don't send to anyone
   if(source->mChatMute || !getGame()->getSettings()->getIniSettings()->enableServerVoiceChat)
      return;

   if(source)
   {
      RefPtr<NetEvent> event = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cVoiceChat, (sourceClientInfo->getName(), voiceBuffer));

      for(S32 i = 0; i < mGame->getClientCount(); i++)
      {
         ClientInfo *clientInfo = mGame->getClientInfo(i);
         GameConnection *dest = clientInfo->getConnection();

         if(dest && dest->mVoiceChatEnabled && clientInfo->getTeamIndex() == sourceClientInfo->getTeamIndex() && (dest != source || echo))
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

   ClientGame *clientGame = static_cast<ClientGame *>(mGame);
   if(clientGame->isOnVoiceMuteList(clientName.getString()))
      return;

   ByteBufferPtr playBuffer = clientInfo->getVoiceDecoder()->decompressBuffer(*(voiceBuffer.getPointer()));
   SoundSystem::queueVoiceChatBuffer(clientInfo->getVoiceSFX(), playBuffer);

#endif
}


// Server tells clients that another player is idle and will not be joining us for the moment
TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cSetIsSpawnDelayed, (StringTableEntry name, bool idle), (name, idle), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = getGame()->findClientInfo(name);

   TNLAssert(clientInfo, "Could not find clientInfo!");  // with RPCGuaranteedOrdered, GameType::s2cAddClient should always be sent first.

   if(!clientInfo)
      return;

   clientInfo->setSpawnDelayed(idle);        // Track other players delay status.  ClientInfo here is a RemoteClientInfo.
#endif
}


// Server tells clients that a player is engineering a teleport
TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cSetPlayerEngineeringTeleporter, (StringTableEntry name, bool isEngineeringTeleporter),
      (name, isEngineeringTeleporter), NetClassGroupGameMask, RPCGuaranteed, RPCToGhost, 0)
{
#ifndef ZAP_DEDICATED
   ClientInfo *clientInfo = getGame()->findClientInfo(name);

   if(!clientInfo)
      return;

   clientInfo->setIsEngineeringTeleporter(isEngineeringTeleporter);
#endif
}


Game *GameType::getGame() const
{
   return mGame;
}


GameTypeId GameType::getGameTypeId() const { return BitmatchGame; }

const char *GameType::getShortName()         const { return "BM";       }
const char *GameType::getInstructionString() const { return "Blast as many ships as you can!"; }


bool GameType::isFlagGame()          const { return false; }
bool GameType::canBeTeamGame()       const { return true;  }
bool GameType::canBeIndividualGame() const { return true;  }


bool GameType::teamHasFlag(S32 teamIndex) const
{
   return false;
}


// Look at every flag and see if its mounted on a ship belonging to the specified team
// Does the actual work for subclass implementations of teamHasFlag
bool GameType::doTeamHasFlag(S32 teamIndex) const
{
   return getGame()->getTeamHasFlag(teamIndex);
}


// Server only
void GameType::updateWhichTeamsHaveFlags()
{
   getGame()->clearTeamHasFlagList();

   const Vector<DatabaseObject *> *flags = getGame()->getGameObjDatabase()->findObjects_fast(FlagTypeNumber);

   for(S32 i = 0; i < flags->size(); i++)
   {
      FlagItem *flag = static_cast<FlagItem *>(flags->get(i));
      if(flag->isMounted() && flag->getMount())
         getGame()->setTeamHasFlag(flag->getMount()->getTeam(), true);
   }

   notifyClientsWhoHasTheFlag();
}


// A flag was mounted on a ship -- in some GameTypes we need to notifiy the clients so they can 
// update their displays to show who has the flag.  Will be overridden in some GameTypes.
// Server only!
void GameType::onFlagMounted(S32 teamIndex)
{
   // Do nothing
}


// Notify the clients when flag status changes... only called by some GameTypes
// Server only!
void GameType::notifyClientsWhoHasTheFlag()
{
   U16 packedBits = 0;

   for(S32 i = 0; i < getGame()->getTeamCount(); i++)
      if(getGame()->getTeamHasFlag(i))
         packedBits += BIT(i);

   s2cSendFlagPossessionStatus(packedBits);
}


GAMETYPE_RPC_S2C(GameType, s2cSendFlagPossessionStatus, (U16 packedBits), (packedBits))
{
   for(S32 i = 0; i < getGame()->getTeamCount(); i++)
      getGame()->setTeamHasFlag(i, packedBits & BIT(i));
}


S32 GameType::getWinningScore() const
{
   return mWinningScore;
}


void GameType::setWinningScore(S32 score)
{
   mWinningScore = score;
}


// Mostly used while reading a level file
void GameType::setGameTime(F32 timeInSeconds)
{
   mGameTimer.reset(U32(timeInSeconds * 1000));    // reset handles case of time being 0
}


// Return time in seconds; returns 0 if game is unlimited
U32 GameType::getTotalGameTime() const
{
   return mGameTimer.getTotalGameTime() / 1000;
}


// Return time remaining in seconds; currently used for Lua on the server, and display purposes on the client
S32 GameType::getRemainingGameTime() const         
{
   return mGameTimer.getCurrent() / 1000;
}


S32 GameType::getRemainingGameTimeInMs() const
{
   return mGameTimer.getCurrent();
}


S32 GameType::getRenderingOffset() const
{
   return mGameTimer.getRenderingOffset();
}


bool GameType::isTimeUnlimited() const
{
   return mGameTimer.isUnlimited();
}


// Only called by various voting codes, server only
void GameType::extendGameTime(S32 timeInMs)
{
   setTimeRemaining(mGameTimer.getCurrent() + timeInMs, false); 
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


S32 GameType::getFlagCount()
{
   return getGame()->getGameObjDatabase()->getObjectCount(FlagTypeNumber);
}


bool GameType::isCarryingItems(Ship *ship)
{
   return ship->getMountedItemCount() > 0;
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


S32 GameType::getDigitsNeededToDisplayScore() const
{
   return mDigitsNeededToDisplayScore;
}


// Send a message to all clients
void GameType::broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, const StringTableEntry &message)
{
   if(!isGameOver())  // Avoid flooding messages on game over.
      for(S32 i = 0; i < mGame->getClientCount(); i++)
         if(!mGame->getClientInfo(i)->isRobot())
            mGame->getClientInfo(i)->getConnection()->s2cDisplayMessage(color, sfx, message);
}


// Send a message to all clients (except robots, of course!)
void GameType::broadcastMessage(GameConnection::MessageColors color, SFXProfiles sfx, 
                                const StringTableEntry &formatString, const Vector<StringTableEntry> &e)
{
   if(!isGameOver())  // Avoid flooding messages on game over.
      for(S32 i = 0; i < mGame->getClientCount(); i++)
         if(!mGame->getClientInfo(i)->isRobot())
            mGame->getClientInfo(i)->getConnection()->s2cDisplayMessageE(color, sfx, formatString, e);
}


bool GameType::isGameOver() const
{
   return mGameOver;
}


// Could possibly get rid of this, and put names into gameType subclasses where they really belong, except that we
// currently use gameTypeId when we send lists of levels to the clients, and we need a way to transform those into
// gameType strings.  This is ugly but practical.  To get rid of it, we might be able to send the gameType strings
// themselves as StringTableEntries, which would be almost as efficient.
// Expand GAME_TYPE_TABLE into an array of names
const char *GameTypeNames[] = {
#  define GAME_TYPE_ITEM(a, b, c, name) name,
       GAME_TYPE_TABLE
#  undef GAME_TYPE_ITEM
};


// Return string like "Capture The Flag" -- static
const char *GameType::getGameTypeName(GameTypeId gameType)
{
   TNLAssert(gameType < ARRAYSIZE(GameTypeNames), "Array index out of bounds!");
   return GameTypeNames[gameType];
}


// Return string like "CTFGameType" -- static
const char *GameType::getGameTypeClassName(GameTypeId gameType)
{
   TNLAssert(gameType < ARRAYSIZE(GameTypeNames), "Array index out of bounds!");
   return gameTypeClassNames[gameType];
}


// static
Vector<string> GameType::getGameTypeNames()
{
   Vector<string> gameTypes;

   for(U32 i = 0; i < ARRAYSIZE(GameTypeNames); i++)
      gameTypes.push_back(GameTypeNames[i]);

   return gameTypes;
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


bool GameType::isEngineerUnrestrictedEnabled()
{
   return mEngineerUnrestrictedEnabled;
}


void GameType::setEngineerUnrestrictedEnabled(bool enabled)
{
   mEngineerUnrestrictedEnabled = enabled;
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
   // Do nothing -- flags are now tracked by the database
}


// Runs only on server
void GameType::itemDropped(Ship *ship, MoveItem *item, MountableItem::DismountMode dismountMode)   
{ 
   TNLAssert(isServer(), "Should not run on client!");
   if(item->getObjectTypeNumber() == FlagTypeNumber)
      updateWhichTeamsHaveFlags();
}


// These methods will be overridden by some game types
void GameType::shipTouchFlag(Ship *ship, FlagItem *flag) { /* Do nothing */ }
void GameType::shipTouchZone(Ship *ship, GoalZone *zone) { /* Do nothing */ }
void GameType::majorScoringEventOcurred(S32 team)        { /* Do nothing */ }

};

