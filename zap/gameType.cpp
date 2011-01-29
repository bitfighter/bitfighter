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
#include "ship.h"
#include "robot.h"
#include "UIGame.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "gameNetInterface.h"
#include "flagItem.h"
#include "gameItems.h"     // For asteroid def.
#include "engineeredObjects.h"
#include "gameObjectRender.h"
#include "config.h"
#include "projectile.h"     // For s2cClientJoinedTeam()
#include "playerInfo.h"     // For LuaPlayerInfo constructor  
#include "stringUtils.h"    // For itos

#include "statistics.h"
#include "masterConnection.h"     // For s2mSendPlayerStatistics, s2mSendGameStatistics


#include "glutInclude.h"

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{
extern  F32 getCurrentRating(GameConnection *conn);      //in game.cpp

//RDW GCC needs this to be properly defined.  -- isn't this defined in gameType.h? -CE
//GCC can't link without this definition.  One of the calls to min in
//UITeamDefMenu.cpp requires that this have actual storage (for some reason).
// I don't even know what that means!! -CE
#ifdef TNL_OS_MAC_OSX
const S32 GameType::gMaxTeams;
#endif

//static Timer mTestTimer(10 * 1000);
//static bool on = true;

// List of valid game types -- these are the "official" names, not the more user-friendly names provided by getGameTypeString
// All names are of the form xxxGameType, and have a corresponding class xxxGame
const char *gGameTypeNames[] = {
   "GameType",                // Generic game type --> Bitmatch
   "CTFGameType",
   "HTFGameType",
   "HuntersGameType",
   "RabbitGameType",
   "RetrieveGameType",
   "SoccerGameType",
   "ZoneControlGameType",
   NULL  // Last item must be NULL
};

S32 gDefaultGameTypeIndex = 0;  // What we'll default to if the name provided is invalid or missing... i.e. GameType ==> Bitmatch

// mini   http://patorjk.com/software/taag/
////////////////////////////////////////   _                 _       _ 
////////////////////////////////////////  /  | o  _  ._ _|_ |_)  _ _|_ 
////////////////////////////////////////  \_ | | (/_ | | |_ | \ (/_ |  
////////////////////////////////////////

// Constructor
ClientRef::ClientRef()    
{
   ping = 0;

   mScore = 0;
   mRating = 0;

   readyForRegularGhosts = false;
   wantsScoreboardUpdates = false;
   mTeamId = 0;
   isAdmin = false;
   isRobot = false;

   mPlayerInfo = new PlayerInfo(this);
}


// Destructor
ClientRef::~ClientRef()   
{
   delete mPlayerInfo;
}


////////////////////////////////////////      __              ___           
////////////////////////////////////////     /__  _. ._ _   _  |    ._   _  
////////////////////////////////////////     \_| (_| | | | (/_ | \/ |_) (/_ 
////////////////////////////////////////                         /  |       

TNL_IMPLEMENT_NETOBJECT(GameType);

// Constructor
GameType::GameType() : mScoreboardUpdateTimer(1000) , mGameTimer(DefaultGameTime) , mGameTimeUpdateTimer(30000)
{
   mNetFlags.set(Ghostable);
   mBetweenLevels = true;
   mGameOver = false;
   mWinningScore = DefaultWinningScore;
   mLeadingTeam = -1;
   mLeadingTeamScore = 0;
   minRecPlayers = -1;
   maxRecPlayers = -1;
   mCanSwitchTeams = true;       // Players can switch right away
   mLocalClient = NULL;          // Will be assigned by the server after a connection is made
   mZoneGlowTimer.setPeriod(mZoneGlowTime);
   mGlowingZoneTeam = -1;        // By default, all zones glow
   mLevelHasLoadoutZone = false;
   mEngineerEnabled = false;     // Is engineer module allowed?  By default, no
   mShowAllBots = false;
   mTotalGamePlay = 0;
}


static Vector<DatabaseObject *> fillVector;

bool GameType::processArguments(S32 argc, const char **argv)
{
   if(argc > 0)      // First arg is game length, in minutes
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));

   if(argc > 1)      // Second arg is winning score
      mWinningScore = atoi(argv[1]);

   return true;
}


// Create some game-specific menu items for the GameParameters menu
void GameType::addGameSpecificParameterMenuItems(Vector<MenuItem *> &menuItems)
{
   menuItems.push_back(new TimeCounterMenuItem("Game Time:", 8 * 60, 99*60, "Unlimited", "Time game will last"));
   menuItems.push_back(new CounterMenuItem("Score to Win:", 10, 1, 1, 99, "points", "", "Game ends when one team gets this score"));
}


void GameType::printRules()
{
   NetClassRep::initialize();
   printf("\n\n");
   printf("Bitfighter rules\n");
   printf("================\n\n");
   printf("Projectiles:\n\n");
   for(S32 i = 0; i < WeaponCount; i++)
   {
      printf("Name: %s \n", gWeapons[i].name.getString());
      printf("\tEnergy Drain: %d\n", gWeapons[i].drainEnergy);
      printf("\tVelocity: %d\n", gWeapons[i].projVelocity);
      printf("\tLifespan (ms): %d\n", gWeapons[i].projLiveTime);
      printf("\tDamage: %2.2f\n", gWeapons[i].damageAmount);
      printf("\tDamage To Self Multiplier: %2.2f\n", gWeapons[i].damageSelfMultiplier);
      printf("\tCan Damage Teammate: %s\n", gWeapons[i].canDamageTeammate ? "Yes" : "No");
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

      // Hunters specific:
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


const char *GameType::validateGameType(const char *gtype)
{
   for(S32 i = 0; gGameTypeNames[i]; i++)    // Repeat until we hit NULL
      if(!strcmp(gGameTypeNames[i], gtype))
         return gGameTypeNames[i];

   // If we get to here, no valid game type was specified, so we'll return the default
   return gGameTypeNames[gDefaultGameTypeIndex];
}


void GameType::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;
   mTotalGamePlay += deltaT;

   if(isGhost())     // i.e. client only
   {
      // Update overlay message timers
      mLevelInfoDisplayTimer.update(deltaT);
      mInputModeChangeAlertDisplayTimer.update(deltaT);

      mGameTimer.update(deltaT);
      mZoneGlowTimer.update(deltaT);

      return;  // We're out of here!
   }

   // From here on, server only
   queryItemsOfInterest();
   if(mScoreboardUpdateTimer.update(deltaT))
   {
      mScoreboardUpdateTimer.reset();
      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->clientConnection)
         {
            mClientList[i]->ping = (U32) mClientList[i]->clientConnection->getRoundTripTime();
            if(mClientList[i]->ping > MaxPing)
               mClientList[i]->ping = MaxPing;
         }
      }

      // Send scores/pings to client if game is over, or client has requested them
      for(S32 i = 0; i < mClientList.size(); i++)
         if(mGameOver || mClientList[i]->wantsScoreboardUpdates)
            updateClientScoreboard(mClientList[i]);
   }

   // Periodically send time-remaining updates to the clients
   if(mGameTimeUpdateTimer.update(deltaT))
   {
      mGameTimeUpdateTimer.reset();
      s2cSetTimeRemaining(mGameTimer.getCurrent());
   }

   // Cycle through all clients
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->respawnTimer.update(deltaT))                            // Need to respawn?
         spawnShip(mClientList[i]->clientConnection);

      if(mClientList[i]->clientConnection->mSwitchTimer.getCurrent())            // Are we still counting down until the player can switch?
         if(mClientList[i]->clientConnection->mSwitchTimer.update(deltaT))       // Has the time run out?
         {
            NetObject::setRPCDestConnection(mClientList[i]->clientConnection);   // Limit who gets this message
            s2cCanSwitchTeams(true);                                             // If so, let the client know they can switch again
            NetObject::setRPCDestConnection(NULL);
         }
   }

   // Need more asteroids?
   for(S32 i = 0; i < mAsteroidSpawnPoints.size(); i++)
   {
      if(mAsteroidSpawnPoints[i].timer.update(deltaT))
      {
         Asteroid *asteroid = dynamic_cast<Asteroid *>(TNL::Object::create("Asteroid"));   // Create a new asteroid

         F32 ang = TNL::Random::readF() * Float2Pi;

         asteroid->setPosAng(mAsteroidSpawnPoints[i].getPos(), ang);

         asteroid->addToGame(gServerGame);                                                 // And add it to the list of game objects

         mAsteroidSpawnPoints[i].timer.reset();                                            // Reset the spawn timer
      }
   }

   //if(mTestTimer.update(deltaT))
   //{
   //   Worm *worm = dynamic_cast<Worm *>(TNL::Object::create("Worm"));
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


// Sorts players by score, high to low
S32 QSORT_CALLBACK playerScoreSort(RefPtr<ClientRef> *a, RefPtr<ClientRef> *b)
{
   return b->getPointer()->getScore() - a->getPointer()->getScore();
}


// Sorts teams by score, high to low
S32 QSORT_CALLBACK teamScoreSort(Team *a, Team *b)
{
   return b->getScore() - a->getScore();  
}


// Sorts teams by player counts, high to low
S32 QSORT_CALLBACK teamSizeSort(Team *a, Team *b)
{
   return (b->numPlayers + b->numBots) - (a->numPlayers + a->numBots);
}


extern IniSettings gIniSettings;

void GameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   if(mLevelInfoDisplayTimer.getCurrent() || gGameUserInterface.mMissionOverlayActive)
   {
      F32 alpha = 1;
      if(mLevelInfoDisplayTimer.getCurrent() < 1000 && !gGameUserInterface.mMissionOverlayActive)
         alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
         glColor4f(1, 1, 1, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 180, 30, "Level: %s", mLevelName.getString());
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 140, 30, "Game Type: %s", getGameTypeString());
         glColor4f(0, 1, 1, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 100, 20, getInstructionString());
         glColor4f(1, 0, 1, alpha);
         UserInterface::drawCenteredString(canvasHeight / 2 - 75, 20, mLevelDescription.getString());

         glColor4f(0, 1, 0, alpha);
         UserInterface::drawCenteredStringf(canvasHeight - 100, 20, "Press [%s] to see this information again", keyCodeToString(keyMISSION));

         if(strcmp(mLevelCredits.getString(), ""))    // Credits string is not empty
         {
            glColor4f(1, 0, 0, alpha);
            UserInterface::drawCenteredStringf(canvasHeight / 2 + 50, 20, "%s", mLevelCredits.getString());
         }

         glColor4f(1, 1, 0, alpha);
         UserInterface::drawCenteredStringf(canvasHeight / 2 - 50, 20, "Score to Win: %d", mWinningScore);

      glDisableBlend;

      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
   {
      // Display alert about input mode changing
      F32 alpha = 1;
      if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
         alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

      glEnableBlend;
      glColor4f(1, 0.5 , 0.5, alpha);
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 130, 20, "Input mode changed to %s", gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");
      glDisableBlend;
   }

   if((mGameOver || scoreboardVisible) && mTeams.size() > 0)      // Render scoreboard
   {
      U32 totalWidth = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin * 2;
      S32 teams = isTeamGame() ? mTeams.size() : 1;

      U32 columnCount = min(teams, 2);

      U32 teamWidth = totalWidth / columnCount;
      S32 maxTeamPlayers = 0;
      countTeamPlayers();

      // Check to make sure at least one team has at least one player...
      for(S32 i = 0; i < mTeams.size(); i++)
      {
         if(isTeamGame())
         {     // (braces required)
            if(mTeams[i].numPlayers > maxTeamPlayers)
               maxTeamPlayers = mTeams[i].numPlayers + mTeams[i].numBots;
         }
         else
            maxTeamPlayers += mTeams[i].numPlayers + mTeams[i].numBots;
      }
      // ...if not, then go home!
      if(!maxTeamPlayers)
         return;

      U32 teamAreaHeight = isTeamGame() ? 40 : 0;
      U32 numTeamRows = (mTeams.size() + 1) >> 1;

      U32 totalHeight = (gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
      U32 maxHeight = min(30, (totalHeight - teamAreaHeight) / maxTeamPlayers);

      U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
      totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

      for(S32 i = 0; i < teams; i++)
      {
         S32 yt = (gScreenInfo.getGameCanvasHeight() - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);  // y-top
         S32 yb = yt + sectionHeight;     // y-bottom
         S32 xl = 10 + (i & 1) * teamWidth;
         S32 xr = xl + teamWidth - 2;

         Color c = getTeamColor(i);
         glEnableBlend;

         glColor4f(c.r, c.g, c.b, 0.6);
         glBegin(GL_POLYGON);
            glVertex2f(xl, yt);
            glVertex2f(xr, yt);
            glVertex2f(xr, yb);
            glVertex2f(xl, yb);
         glEnd();

         glDisableBlend;

         glColor3f(1,1,1);
         if(isTeamGame())     // Render team scores
         {
            renderFlag(xl + 20, yt + 18, c);
            renderFlag(xr - 20, yt + 18, c);

            glColor3f(1,1,1);
            glBegin(GL_LINES);
               glVertex2f(xl, yt + teamAreaHeight);
               glVertex2f(xr, yt + teamAreaHeight);
            glEnd();

            UserInterface::drawString(xl + 40, yt + 2, 30, getTeamName(i).getString());
            UserInterface::drawStringf(xr - 140, yt + 2, 30, "%d", mTeams[i].getScore());
         }


         // Now for player scores.  First build a list, then sort it, then display it.

         Vector<RefPtr<ClientRef> > playerScores;

         for(S32 j = 0; j < mClientList.size(); j++)
         {
            if(mClientList[j]->getTeam() == i || !isTeamGame())
               playerScores.push_back(mClientList[j]);
         }

         playerScores.sort(playerScoreSort);

         S32 curRowY = yt + teamAreaHeight + 1;
         S32 fontSize = U32(maxHeight * 0.8f);

         for(S32 j = 0; j < playerScores.size(); j++)
         {
            static const char *bot = "B ";
            S32 botsize = UserInterface::getStringWidth(fontSize / 2, bot);
            S32 x = xl + 40;

            // Add the mark of the bot
            if(playerScores[j]->isRobot)
               UserInterface::drawString(x - botsize, curRowY + fontSize / 4 + 2, fontSize / 2, bot); 

            UserInterface::drawString(x, curRowY, fontSize, playerScores[j]->name.getString());

            static char buff[255] = "";

            if(isTeamGame())
               dSprintf(buff, sizeof(buff), "%2.2f", (F32)playerScores[j]->getRating());
            else
               dSprintf(buff, sizeof(buff), "%d", playerScores[j]->getScore());

            UserInterface::drawString(xr - (120 + UserInterface::getStringWidth(fontSize, buff)), curRowY, fontSize, buff);
            UserInterface::drawStringf(xr - 70, curRowY, fontSize, "%d", playerScores[j]->ping);
            curRowY += maxHeight;
         }
      }
   }
   else if(mTeams.size() > 1 && isTeamGame())      // Render team scores in lower-right corner when scoreboard is off
   {
      S32 lroff = getLowerRightCornerScoreboardOffsetFromBottom();

      // Build a list of teams, so we can sort by score
      Vector<Team> teams(mTeams.size());

      for(S32 i = 0; i < mTeams.size(); i++)
         teams.push_back(mTeams[i]);

      teams.sort(teamScoreSort);    

      S32 maxScore = getLeadingScore();
      S32 digits;
         
      if(maxScore == 0)
         digits = 1;
      else if(maxScore > 0)
         digits = log10((F32)maxScore) + 1;
      else
         digits = log10((F32)maxScore) + 2;

      const S32 textsize = 32;
      S32 xpos = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - digits * UserInterface::getStringWidth(textsize, "0");

      for(S32 i = 0; i < teams.size(); i++)
      {
         S32 ypos = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - lroff - (teams.size() - i - 1) * 38;

         renderFlag(xpos - 20, ypos + 18, teams[i].color);
         glColor3f(1,1,1);
         UserInterface::drawStringf(xpos, ypos, textsize, "%d", teams[i].getScore());
      }
   }

   //else if(mTeams.size() > 0 && !isTeamGame())   // Render leaderboard for non-team games
   //{
   //   S32 lroff = getLowerRightCornerScoreboardOffsetFromBottom();

   //   // Build a list of teams, so we can sort by score
   //   Vector<RefPtr<ClientRef> > leaderboardList;

   //   // Add you to the leaderboard
   //   if(mLocalClient)
   //   {
   //      leaderboardList.push_back(mLocalClient);
   //      logprintf("Score = %d", mLocalClient->getScore());
   //   }

   //   // Get leading player
   //   ClientRef *winningClient = mClientList[0];
   //   for(S32 i = 1; i < mClientList.size(); i++)
   //   {
   //      if(mClientList[i]->getScore() > winningClient->getScore())
   //      {
   //         winningClient = mClientList[i];
   //      }
   //   }

   //   // Add leader to the leaderboard
   //   leaderboardList.push_back(winningClient);

   //   const S32 textsize = 20;

   //   for(S32 i = 0; i < leaderboardList.size(); i++)
   //   {
   //      const char* prefix = "";
   //      if(i == leaderboardList.size() - 1)
   //      {
   //         prefix = "Leader:";
   //      }
   //      const char* name = leaderboardList[i]->name.getString();
   //      S32 score = leaderboardList[i]->getScore();

   //      S32 xpos = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 
   //                 UserInterface::getStringWidthf(textsize, "%s %s %d", prefix, name, score);
   //      S32 ypos = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - lroff - i * 24;

   //      glColor3f(1, 1, 1);
   //      UserInterface::drawStringf(xpos, ypos, textsize, "%s %s %d", prefix, name, score);
   //   }
   //}

   renderTimeLeft();
   renderTalkingClients();
}


void GameType::renderObjectiveArrow(GameObject *target, Color c, F32 alphaMod)
{
   if(!target)
      return;

   GameConnection *gc = gClientGame->getConnectionToServer();
   GameObject *co = NULL;
   if(gc)
      co = gc->getControlObject();
   if(!co)
      return;

   Rect r = target->getBounds(MoveObject::RenderState);
   Point nearestPoint = co->getRenderPos();

   if(r.max.x < nearestPoint.x)
      nearestPoint.x = r.max.x;
   if(r.min.x > nearestPoint.x)
      nearestPoint.x = r.min.x;
   if(r.max.y < nearestPoint.y)
      nearestPoint.y = r.max.y;
   if(r.min.y > nearestPoint.y)
      nearestPoint.y = r.min.y;

   renderObjectiveArrow(nearestPoint, c, alphaMod);
}


void GameType::renderObjectiveArrow(Point nearestPoint, Color c, F32 alphaMod)
{
   GameConnection *gc = gClientGame->getConnectionToServer();
   GameObject *co = NULL;
   if(gc)
      co = gc->getControlObject();
   if(!co)
      return;

   Point rp = gClientGame->worldToScreenPoint(nearestPoint);
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
   F32 alpha = (1 - gClientGame->getCommanderZoomFraction()) * 0.6 * alphaMod;
   if(!alpha)
      return;

   // Make indicator fade as we approach the target
   if(dist < 50)
      alpha *= dist * 0.02;

   // Scale arrow accorging to distance from objective --> doesn't look very nice
   //F32 scale = max(1 - (min(max(dist,100),1000) - 100) / 900, .5);
   F32 scale = 1.0;

   Point p2 = rp - arrowDir * 23 * scale + crossVec * 8 * scale;
   Point p3 = rp - arrowDir * 23 * scale - crossVec * 8 * scale;


   glEnableBlend;
   glColor(c * 0.7, alpha);
   glBegin(GL_POLYGON);    // Fill
      glVertex(rp);
      glVertex(p2);
      glVertex(p3);
   glEnd();
   glColor(c, alpha);
   glBegin(GL_LINE_LOOP);  // Outline
      glVertex(rp);
      glVertex(p2);
      glVertex(p3);
   glEnd();
   glDisableBlend;

   Point cen = rp - arrowDir * 12;

   // Try labelling the objective arrows... kind of lame.
   //UserInterface::drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}


void GameType::renderTimeLeft()
{
   U32 timeLeft = mGameTimer.getCurrent();      // Time remaining in game

   const S32 size = 20;       // Size of time
   const S32 gtsize = 12;     // Size of game type/score indicator
   
   S32 len = UserInterface::getStringWidthf(gtsize, "[%s/%d]", getShortName(), mWinningScore);

   glColor3f(0,1,1);
   UserInterface::drawStringf(gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 65 - len - 5,
                              gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 20 + ((size - gtsize) / 2) + 2, 
                              gtsize, "[%s/%d]", getShortName(), mWinningScore);

   S32 x = gScreenInfo.getGameCanvasWidth() - UserInterface::horizMargin - 65;
   S32 y = gScreenInfo.getGameCanvasHeight() - UserInterface::vertMargin - 20;
   glColor3f(1,1,1);

   if(mGameTimer.getPeriod() == 0)
   {
      UserInterface::drawString(x, y, size, "Unlim.");
   }
   else
   {
      U32 minsRemaining = timeLeft / (60000);
      U32 secsRemaining = (timeLeft - (minsRemaining * 60000)) / 1000;

      UserInterface::drawStringf(x, y, size, "%02d:%02d", minsRemaining, secsRemaining);
   }
}


void GameType::renderTalkingClients()
{
   S32 y = 150;
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->voiceSFX->isPlaying())
      {
         Color teamColor = mTeams[mClientList[i]->getTeam()].color;
         glColor(teamColor);
         UserInterface::drawString(10, y, 20, mClientList[i]->name.getString());
         y += 25;
      }
   }
}


// Server only
void GameType::gameOverManGameOver()
{
   if(mGameOver)     // Only do this once
      return;

   mBetweenLevels = true;
   mGameOver = true;             // Show scores at end of game
   s2cSetGameOver(true);         // Alerts clients that the game is over
   gServerGame->gameEnded();     // Sets level-switch timer, which gives us a short delay before switching games

   onGameOver();                 // Call game-specific end-of-game code

   saveGameStats();
}


// Transmit statistics to the master server, LogStats to game server
void GameType::saveGameStats()
{
   MasterServerConnection *masterConn = gServerGame->getConnectionToMaster();

   countTeamPlayers();     // Make sure all team sizes are up to date

   // Assign some ids so we can find these teams later, post sorting...
   for(S32 i = 0; i < mTeams.size(); i++)
      mTeams[i].setId(i);

   Vector<Team> sortTeams(mTeams);
   sortTeams.sort(teamScoreSort);

   // Push team names and scores into a structure we can pass via rpc; these will be sorted by player counts, high to low
   Vector<StringTableEntry>          teams      (sortTeams.size());
   Vector<S32>                       scores     (sortTeams.size());
   //Vector<RangedU32<0,MAX_PLAYERS> > teamPlayers(sortTeams.size());
   //Vector<RangedU32<0,MAX_PLAYERS> > teamBots   (sortTeams.size());
   Vector<RangedU32<0,0xFFFFFF> >    colors     (sortTeams.size());

   for(S32 i = 0; i < sortTeams.size(); i++)
   {
      teams.push_back(sortTeams[i].getName());
      scores.push_back(sortTeams[i].getScore());

      colors.push_back(sortTeams[i].color.toRangedU32());

      //teamPlayers.push_back(sortTeams[i].numPlayers);
      //teamBots.push_back(sortTeams[i].numPlayers);
   }

   // And now the player data, presized to fit the number of clients; avoids dynamic resizing
   Vector<bool> lastOnTeam             (mClientList.size());
   Vector<StringTableEntry> playerNames(mClientList.size());
   Vector<Vector<U8> > playerIDs       (mClientList.size());
   Vector<bool> isBot                  (mClientList.size());
   Vector<S32> playerScores            (mClientList.size());
   Vector<U16> playerKills             (mClientList.size());
   Vector<U16> playerDeaths            (mClientList.size());
   Vector<U16> playerSuicides          (mClientList.size());
   Vector<U16> teamSwitchCount         (mClientList.size());
   Vector<Vector<U16> > shots          (mClientList.size()); 
   Vector<Vector<U16> > hits           (mClientList.size());
      

   // mSortedClientList is list of players sorted by player score; may not matter in team game, but it does in solo games
   //Vector<RefPtr<ClientRef> > mSortedClientList(mClientList);   
   //mSortedClientList.sort(playerScoreSort);

   for(S32 i = 0; i < sortTeams.size(); i++)
   {
      for(S32 j = 0; j < mClientList.size(); j++)
      {
         // Only looking for players on the current team
         if(mClientList[j]->getTeam() != sortTeams[i].getId())
            continue;

         Statistics *statistics = &mClientList[j]->clientConnection->mStatistics;
            
         lastOnTeam.push_back(false);

         playerNames    .push_back(mClientList[j]->name);    // TODO: What if this is a bot??  What should go here??
         playerIDs      .push_back(mClientList[j]->clientConnection->getClientId()->toVector());
         isBot          .push_back(mClientList[j]->isRobot);
         playerScores   .push_back(mClientList[j]->getScore());
         playerKills    .push_back(statistics->getKills());
         playerDeaths   .push_back(statistics->getDeaths());
         playerSuicides .push_back(statistics->getSuicides());
         shots          .push_back(statistics->getShotsVector());
         hits           .push_back(statistics->getHitsVector());
         teamSwitchCount.push_back(mClientList[j]->clientConnection->switchedTeamCount);
      }

      if(lastOnTeam.size() != 0)  // TODO: fix zero players in first team and some players in the next team
         lastOnTeam[lastOnTeam.size() - 1] = true;
   }

   U16 timeInSecs = U16(mTotalGamePlay / 1000);

   /*GameStatistics3 gameStat;
   gameStat.gameType = getGameTypeString();
   gameStat.teamGame = isTeamGame();
   gameStat.levelName = mLevelName;
   gameStat.teams = teams;
   gameStat.teamScores = scores;
   gameStat.color = colors;
   gameStat.timeInSecs = timeInSecs;
   gameStat.playerNames = playerNames;
   gameStat.playerIDs = playerIDs;
   gameStat.isBot = isBot;
   gameStat.lastOnTeam = lastOnTeam;
   gameStat.playerScores = playerScores;    
   gameStat.playerKills = playerKills;
   gameStat.playerDeaths = playerDeaths;
   gameStat.playerSuicides = playerSuicides;
   gameStat.shots = shots;
   gameStat.hits = hits;
   gameStat.playerSwitchedTeamCount = playerSwitchedTeamCount;*/

   if(masterConn)                                        // && gIniSettings.SendStatsToMaster)
		//masterConn->s2mSendGameStatistics_3(gameStat);

      masterConn->s2mSendGameStatistics_3(getGameTypeString(), isTeamGame(), mLevelName, teams, scores, colors, 
                                          timeInSecs, playerNames, playerIDs, isBot,
                                          lastOnTeam, playerScores, playerKills, playerDeaths, playerSuicides, teamSwitchCount, shots, hits);


   // TODO: We shoud create a method that reads the structures we just created and writes stats from them; we can use the same code
   // in the master, which will have these structs as well, to produce a consnstent text logfile format.
   // We could also have the stats be written to a local mysql database if one were available using the same code.
	switch(gIniSettings.LogStats)
	{
		case 1:
			logprintf(LogConsumer::ServerFilter, "Version=%i %s %i:%02i", BUILD_VERSION, getGameTypeString(), timeInSecs / 60, timeInSecs % 60);
			logprintf(LogConsumer::ServerFilter, "%s Level=%s", isTeamGame() ? "Team" : "NoTeam", mLevelName.getString());
			for(S32 i = 0; i < mTeams.size(); i++)
				logprintf(LogConsumer::ServerFilter, "Team=%i Score=%i Color=%s Name=%s",
					i,                           // Using unsorted, to correctly use index as team ID. Teams can have same name
					mTeams[i].getScore(),
               mTeams[i].color.toHexString().c_str(),
					mTeams[i].getName().getString());
			break;
		// case 2: // For using other formats
		default:
			logprintf(LogConsumer::LogWarning, "Format unsupported, in INI file, use LogStats = 1");
	}

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      Statistics *statistics = &mClientList[i]->clientConnection->mStatistics;
        
      //if(masterConn) //&& gIniSettings.SendStatsToMaster)
      //   masterConn->s2mSendPlayerStatistics_3(mClientList[i]->name, mClientList[i]->clientConnection->getClientId()->toVector(), 
      //                                      mClientList[i]->isRobot,
      //                                      getTeamName(mClientList[i]->getTeam()),  // Both teams might have same name...
      //                                      mClientList[i]->getScore(),
      //                                      statistics->getKills(), statistics->getDeaths(), 
      //                                      statistics->getSuicides(), statistics->getShotsVector(), statistics->getHitsVector());
		switch(gIniSettings.LogStats)
		{
			case 1:
				logprintf(LogConsumer::ServerFilter, "%s=%s Team=%i Score=%i Rating=%f kill=%i death=%i suicide=%i",
					mClientList[i]->isRobot ? "Robot" : "Player",
					mClientList[i]->name.getString(),
					mClientList[i]->getTeam(),
					mClientList[i]->getScore(),
				   getCurrentRating(mClientList[i]->clientConnection),
					statistics->getKills(),
					statistics->getDeaths(),
               statistics->getSuicides());
				break;
			// case 2: // For using other formats
		}
   }
}


// Handle the end-of-game...  handles all games... not in any subclasses
// Can be overridden for any game-specific game over stuff
void GameType::onGameOver()
{
   static StringTableEntry tieMessage("The game ended in a tie.");
   static StringTableEntry winMessage("%e0%e1 wins the game!");
   static StringTableEntry teamString("Team ");
   static StringTableEntry emptyString;

   bool tied = false;
   Vector<StringTableEntry> e;
   if(isTeamGame())   // Team game -> find top team
   {
      S32 teamWinner = 0;
      S32 winningScore = mTeams[0].getScore();
      for(S32 i = 1; i < mTeams.size(); i++)
      {
         if(mTeams[i].getScore() == winningScore)
            tied = true;
         else if(mTeams[i].getScore() > winningScore)
         {
            teamWinner = i;
            winningScore = mTeams[i].getScore();
            tied = false;
         }
      }
      if(!tied)
      {
         e.push_back(teamString);
         e.push_back(mTeams[teamWinner].getName());
      }
   }
   else                    // Individual game -> find player with highest score
   {
      if(mClientList.size())
      {
         ClientRef *winningClient = mClientList[0];
         for(S32 i = 1; i < mClientList.size(); i++)
         {
            if(mClientList[i]->getScore() == winningClient->getScore())
               tied = true;
            else if(mClientList[i]->getScore() > winningClient->getScore())
            {
               winningClient = mClientList[i];
               tied = false;
            }
         }
         if(!tied)
         {
            e.push_back(emptyString);
            e.push_back(winningClient->name);
         }
      }
   }
   if(tied)
   {
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessage(GameConnection::ColorNuclearGreen, SFXFlagDrop, tieMessage);
   }
   else
   {
      for(S32 i = 0; i < mClientList.size(); i++)
         mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXFlagCapture, winMessage, e);
   }
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cSetGameOver, (bool gameOver), (gameOver),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCToGhost, 0)
{
   mBetweenLevels = gameOver;
   mGameOver = gameOver;
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


extern Rect gServerWorldBounds;

// Find all spubugs in the game, and store them for future reference
// server only
void GameType::catalogSpybugs()
{
   mSpyBugs.clear();

   // Find all spybugs in the game
   gServerGame->getGridDatabase()->findObjects(SpyBugType, mSpyBugs, gServerWorldBounds);
}


// Runs on server, after level has been loaded from a file.  Can be overridden, but isn't.
void GameType::onLevelLoaded()
{
   catalogSpybugs();

   // Figure out if this level has any loadout zones
   fillVector.clear();
   getGame()->getGridDatabase()->findObjects(LoadoutZoneType, fillVector, gServerWorldBounds);

   mLevelHasLoadoutZone = (fillVector.size() > 0);

   Robot::startBots();
}


void GameType::onAddedToGame(Game *theGame)
{
   theGame->setGameType(this);
   if(getGame()->isServer())
      mShowAllBots = gServerGame->isTestServer();  //Default to true to show all bots if on testing mode.
}


extern void constructBarriers(Game *theGame, const Vector<F32> &barrier, F32 width, bool solid);

// Returns true if we've handled the line (even if it handling it means that the line was bogus); returns false if
// caller needs to create an object based on the line
bool GameType::processLevelItem(S32 argc, const char **argv)
{
   if(!stricmp(argv[0], "Team"))
   {
      if(mTeams.size() < gMaxTeams)   // Too many teams?
      {
         Team team;
         team.readTeamFromLevelLine(argc, argv);
   
         if(team.numPlayers != -1)
            mTeams.push_back(team);
      }
   }
   else if(!stricmp(argv[0], "TeamChange"))   // For level script. Could be removed when there is a better way to change team names and colors.
   {
      if(argc >= 2)   // Enough arguments?
      {
         Team team;
         S32 teamNumber = atoi(argv[1]);   //Team number to change
         team.readTeamFromLevelLine(argc-1, argv+1);    //skip one arg
   
         if(team.numPlayers + team.numBots != -1 && teamNumber < mTeams.size() && teamNumber >= 0)
            mTeams[teamNumber] = team;
      }
   }
   else if(!stricmp(argv[0], "Specials"))
   {         
      // Examine items on the specials line
      for(S32 i = 1; i < argc; i++)
         if(!stricmp(argv[i], "Engineer" ) )
            mEngineerEnabled = true;
   }
   else if(!strcmp(argv[0], "Script"))
   {
      mScriptArgs.clear();    // Clear out any args from a previous Script line

      if(argc <= 1)      // At a minimum, we need a script name
         mScriptName = "";

      else
      {
         mScriptName = argv[1];
   
         for(S32 i = 2; i < argc; i++)
            mScriptArgs.push_back(string(argv[i]));    // Use string to make a const char copy of the param
      }
   }
   else if(!stricmp(argv[0], "Spawn"))
   {
      if(argc >= 4)
      {
         S32 teamIndex = atoi(argv[1]);
         Point p;
         p.read(argv + 2);
         p *= getGame()->getGridSize();

         if(teamIndex >= 0 && teamIndex < mTeams.size())    // Normal teams; ignore if invalid
            mTeams[teamIndex].spawnPoints.push_back(p);
         else if(teamIndex == -1)                           // Neutral spawn point, add to all teams
            for(S32 i = 0; i < mTeams.size(); i++)
               mTeams[i].spawnPoints.push_back(p);
      }
   }
   else if(!stricmp(argv[0], "FlagSpawn"))      // FlagSpawn <team> <x> <y> [timer]
   {
      if(argc >= 4)
      {
         S32 teamIndex = atoi(argv[1]);
         Point p;
         p.read(argv + 2);
         p *= getGame()->getGridSize();
   
         S32 time = (argc > 4) ? atoi(argv[4]) : FlagSpawn::defaultRespawnTime;
   
         FlagSpawn spawn = FlagSpawn(p, time * 1000);
   
         // Following works for Nexus & Soccer games because they are not TeamFlagGame.  Currently, the only
         // TeamFlagGame is CTF.
   
         if(isTeamFlagGame() && (teamIndex >= 0 && teamIndex < mTeams.size()) )    // If we can't find a valid team...
            mTeams[teamIndex].flagSpawnPoints.push_back(spawn);
         else
            mFlagSpawnPoints.push_back(spawn);                                     // ...then put it in the non-team list
      }
   }
   else if(!stricmp(argv[0], "AsteroidSpawn"))      // AsteroidSpawn <x> <y> [timer]
   {
      if(argc >= 3)
      {
         Point p;
         p.read(argv + 1);
         p *= getGame()->getGridSize();
   
         S32 time = (argc > 3) ? atoi(argv[3]) : AsteroidSpawn::defaultRespawnTime;
   
         AsteroidSpawn spawn = AsteroidSpawn(p, time * 1000);
         mAsteroidSpawnPoints.push_back(spawn);
      }
   }
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      if(argc >= 2)
      {
         BarrierRec barrier;
         barrier.width = atof(argv[1]);
   
         if(barrier.width < Barrier::MIN_BARRIER_WIDTH)
            barrier.width = Barrier::MIN_BARRIER_WIDTH;
         else if(barrier.width > Barrier::MAX_BARRIER_WIDTH)
            barrier.width = Barrier::MAX_BARRIER_WIDTH;
   
         for(S32 i = 2; i < argc; i++)
            barrier.verts.push_back(atof(argv[i]) * getGame()->getGridSize());
   
         if(barrier.verts.size() > 3)
         {
            barrier.solid = false;
            mBarriers.push_back(barrier);
            constructBarriers(getGame(), barrier.verts, barrier.width, barrier.solid);
         }
      }
   }
   // TODO: Integrate code above with code above!!  EASY!!
   else if(!stricmp(argv[0], "BarrierMakerS"))
   {
      if(argc >= 2)
      { 
         BarrierRec barrier;
         barrier.width = atof(argv[1]);
         for(S32 i = 2; i < argc; i++)
            barrier.verts.push_back(atof(argv[i]) * getGame()->getGridSize());
         if(barrier.verts.size() > 3)
         {
            barrier.solid = true;
            mBarriers.push_back(barrier);
            constructBarriers(getGame(), barrier.verts, barrier.width, barrier.solid);
         }
      }
   }
   else if(!stricmp(argv[0], "LevelName"))
   {
      string s;
      for(S32 i = 1; i < argc; i++)
      {
         s += argv[i];
         if(i < argc - 1)
            s += " ";
      }
      mLevelName.set(s.substr(0, MAX_GAME_NAME_LEN).c_str());
   }
   else if(!stricmp(argv[0], "LevelDescription"))
   {
      string s;
      for(S32 i = 1; i < argc; i++)
      {
         s += argv[i];
         if(i < argc - 1)
            s += " ";
      }
      mLevelDescription.set(s.substr(0, MAX_GAME_DESCR_LEN).c_str());
   }
   else if(!stricmp(argv[0], "LevelCredits"))
   {
      string s;
      for(S32 i = 1; i < argc; i++)
      {
         s += argv[i];
         if(i < argc - 1)
            s += " ";
      }
      mLevelCredits.set(s.substr(0, MAX_GAME_DESCR_LEN).c_str());
   }
   else if(!stricmp(argv[0], "MinPlayers"))     // Recommend a min numbrt of players for this map
   {
      if(argc > 1)
         minRecPlayers = atoi(argv[1]);
   }
   else if(!stricmp(argv[0], "MaxPlayers"))     // Recommend a max players for this map
   {
      if(argc > 1)
         maxRecPlayers = atoi(argv[1]);
	}
   else
      return false;     // Line not processed; perhaps the caller can handle it?

   return true;         // Line processed; caller can ignore it
}


// Find client object given a player name
ClientRef *GameType::findClientRef(const StringTableEntry &name)
{
   for(S32 clientIndex = 0; clientIndex < mClientList.size(); clientIndex++)
      if(mClientList[clientIndex]->name == name)
         return mClientList[clientIndex];
   return NULL;
}


// Only gets run on the server!
void GameType::spawnShip(GameConnection *theClient)
{
   ClientRef *cl = theClient->getClientRef();
   U32 teamIndex = cl->getTeam();

   Point spawnPoint = getSpawnPoint(teamIndex);

   if(theClient->isRobot())
   {
		Robot *robot = (Robot *) theClient->getControlObject();
      robot->setOwner(theClient);
		robot->setTeam(teamIndex);
		spawnRobot(robot);
   }
   else
   {
      // Player's name, team, and spawn location
      Ship *newShip = new Ship(cl->name, theClient->isAuthenticated(), teamIndex, spawnPoint);
      theClient->setControlObject(newShip);
      newShip->setOwner(theClient);
      newShip->addToGame(getGame());
   }

   if(isSpawnWithLoadoutGame() || !levelHasLoadoutZone())
      setClientShipLoadout(cl, theClient->getLoadout());     // Set loadout if this is a SpawnWithLoadout type of game, or there is no loadout zone
   else
      setClientShipLoadout(cl, theClient->mOldLoadout, true);     // old loadout
   theClient->mOldLoadout.clear();
}


// Note that we need to have spawn method here so we can override it for different game types, such as Nexus (hunters)
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

   // Should be done in intialize
   //robot->runMain();                      // Gentlemen, start your engines!
   //robot->getEventManager().update();     // Ensure registrations made during bot initialization are ready to go


   // Should probably do this, but... not now.
   //if(isSpawnWithLoadoutGame())
   //   setClientShipLoadout(cl, theClient->getLoadout());                  // Set loadout if this is a SpawnWithLoadout type of game
}


Point GameType::getSpawnPoint(S32 team)
{
   // If team has no spawn points, we'll just have them spawn at 0,0
   if(mTeams[team].spawnPoints.size() == 0)
      return Point(0,0);

   S32 spawnIndex = TNL::Random::readI() % mTeams[team].spawnPoints.size();    // Pick random spawn point
   return mTeams[team].spawnPoints[spawnIndex];
}


// This gets run when the ship hits a loadout zone
void GameType::updateShipLoadout(GameObject *shipObject)
{
   GameConnection *gc = shipObject->getControllingClient();
   if(!gc)
      return;

   ClientRef *cl = gc->getClientRef();
   if(!cl)
      return;

   setClientShipLoadout(cl, gc->getLoadout());
}


void GameType::setClientShipLoadout(ClientRef *cl, const Vector<U32> &loadout, bool silent)
{
   if(loadout.size() != ShipModuleCount + ShipWeaponCount)     // Reject improperly sized loadouts.  Currently 2 + 3
      return;

   if(!engineerIsEnabled() && (loadout[0] == ModuleEngineer || loadout[1] == ModuleEngineer))  // Reject engineer if it is not enabled
      return;

   Ship *theShip = dynamic_cast<Ship *>(cl->clientConnection->getControlObject());
   if(theShip)
      theShip->setLoadout(loadout, silent);
}


void GameType::clientRequestLoadout(GameConnection *client, const Vector<U32> &loadout)
{
   // Not CE
   //S32 clientIndex = findClientIndexByConnection(client);
   //if(clientIndex != -1)
   //   setClientShipLoadout(clientIndex, loadout);
}



// Runs only on server, I think
void GameType::performScopeQuery(GhostConnection *connection)
{
   GameConnection *gc = (GameConnection *) connection;
   GameObject *co = gc->getControlObject();
   ClientRef *cr = gc->getClientRef();

   //TNLAssert(gc, "Invalid GameConnection in gameType.cpp!");
   //TNLAssert(co, "Invalid ControlObject in gameType.cpp!");
   //TNLAssert(cr, "Invalid ClientRef in gameType.cpp!");

   const Vector<SafePtr<GameObject> > &scopeAlwaysList = getGame()->getScopeAlwaysList();

   gc->objectInScope(this);   // Put GameType in scope, always

   // Make sure the "always-in-scope" objects are actually in scope
   for(S32 i = 0; i < scopeAlwaysList.size(); i++)
      if(!scopeAlwaysList[i].isNull())
         gc->objectInScope(scopeAlwaysList[i]);

   // readyForRegularGhosts is set once all the RPCs from the GameType
   // have been received and acknowledged by the client
   ClientRef *clientRef = gc->getClientRef();
   if(clientRef)
   {
      if(clientRef->readyForRegularGhosts && co)
      {
         performProxyScopeQuery(co, gc);
         gc->objectInScope(co);     // Put controlObject in scope ==> This is where the update mask gets set to 0xFFFFFFFF
      }
   }

   // What does the spy bug see?
   //S32 teamId = gc->getClientRef()->teamId;

   for(S32 i = 0; i < mSpyBugs.size(); i++)
   {
      SpyBug *sb = dynamic_cast<SpyBug *>(mSpyBugs[i]);
      if(!sb->isVisibleToPlayer( cr->getTeam(), cr->name, isTeamGame() ))
         break;
      Point pos = sb->getActualPos();
      Point scopeRange(gSpyBugRange, gSpyBugRange);
      Rect queryRect(pos, pos);
      queryRect.expand(scopeRange);

      fillVector.clear();
      findObjects(AllObjectTypes, fillVector, queryRect);

      for(S32 j = 0; j < fillVector.size(); j++)
         connection->objectInScope(dynamic_cast<GameObject *>(fillVector[j]));
   }
}



// Here is where we determine which objects are visible from player's ships.  Only runs on server.
void GameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
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

   // If we're in commander's map mode, then we can see what our teammates can see.  This will
   // also scope what we can see.
   if(isTeamGame() && connection->isInCommanderMap())
   {
      TNLAssert(connection->getClientRef(), "ClientRef should never be NULL!");
      S32 teamId = connection->getClientRef()->getTeam();
      fillVector.clear();

      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->getTeam() != teamId)      // Wrong team
            continue;

         TNLAssert(mClientList[i]->clientConnection, "No client connection in PerformScopequery");     // Should never happen

         Ship *ship = dynamic_cast<Ship *>(mClientList[i]->clientConnection->getControlObject());
         if(!ship)       // Can happen!
            continue;

         Point pos = ship->getActualPos();
         Rect queryRect(pos, pos);
         queryRect.expand( Game::getScopeRange(ship->isModuleActive(ModuleSensor)) );

         findObjects(( (scopeObject == ship) ? AllObjectTypes : CommandMapVisType), fillVector, queryRect);
      }
   }
   else     // Do a simple query of the objects within scope range of the ship
   {
      // Note that if we make mine visibility controlled by server, here's where we'd put the code
      Point pos = scopeObject->getActualPos();
      Ship *co = dynamic_cast<Ship *>(scopeObject);
      TNLAssert(co, "Null control object!");

      Rect queryRect(pos, pos);
      queryRect.expand( Game::getScopeRange(co->isModuleActive(ModuleSensor)) );

      fillVector.clear();
      findObjects(AllObjectTypes, fillVector, queryRect);
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


void GameType::addItemOfInterest(Item *theItem)
{
#ifdef TNL_DEBUG
   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
      TNLAssert(mItemsOfInterest[i].theItem.getPointer() != theItem, "item in ItemOfInterest already exist.");
#endif
   ItemOfInterest i;
   i.theItem = theItem;
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
         // Currently can happen when dropping HuntersFlagItem in ZoneControlGameType.
         TNLAssert(false,"item in ItemOfInterest is NULL. This can happen when an item got deleted.");
         mItemsOfInterest.erase(i);    // non-debug mode will skip TNLAssert, do this to fix this error.
         break;
      }
      ioi.teamVisMask = 0;                         // Reset mask, object becomes invisible to all teams
      Point pos = ioi.theItem->getActualPos();
      Point scopeRange(Game::PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL, Game::PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL);
      Rect queryRect(pos, pos);

      queryRect.expand(scopeRange);
      fillVector.clear();
      findObjects(ShipType | RobotType, fillVector, queryRect);

      for(S32 j = 0; j < fillVector.size(); j++)
      {
         Ship *theShip = dynamic_cast<Ship *>(fillVector[j]);     // Safe because we only looked for ships and robots
         Point delta = theShip->getActualPos() - pos;
         delta.x = fabs(delta.x);
         delta.y = fabs(delta.y);

         if( (theShip->isModuleActive(ModuleSensor) && delta.x < Game::PLAYER_SENSOR_VISUAL_DISTANCE_HORIZONTAL && delta.y < Game::PLAYER_SENSOR_VISUAL_DISTANCE_VERTICAL) ||
               (delta.x < Game::PLAYER_VISUAL_DISTANCE_HORIZONTAL && delta.y < Game::PLAYER_VISUAL_DISTANCE_VERTICAL) )
            ioi.teamVisMask |= (1 << theShip->getTeam());      // Mark object as visible to theShip's team
      }
   }
}


// Team in range?    Currently not used.
// Could use it for processArguments, but out of range will be UNKNOWN name and should not cause any errors.
bool GameType::checkTeamRange(S32 team){
	return (team < mTeams.size() && team >= -2);
}

// Zero teams will crash.
bool GameType::makeSureTeamCountIsNotZero()
{
	if(mTeams.size() == 0) {
		Team team;
		team.setName("Missing Team");
		team.color.r = 0;
		team.color.g = 0;
		team.color.b = 1;
		mTeams.push_back(team);
		return true;
	}
	return false;
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

// This method can be overridden by other game types that handle colors differently
Color GameType::getTeamColor(S32 team)
{
   if(team == -1 || team >= mTeams.size() || team < -2)
      return gNeutralTeamColor;
   else if(team == -2)
      return gHostileTeamColor;
   else
      return mTeams[team].color;
}


// Given a player's name, return his team
S32 GameType::getTeam(const char *playerName)
{
   for(S32 i = 0; i < mClientList.size(); i++)
      if(!strcmp(mClientList[i]->name.getString(), playerName))
         return(mClientList[i]->getTeam());

   return(Item::TEAM_NEUTRAL);    // If we can't find the team, let's call it neutral
}


//There is a bigger need to use StringTableEntry and not const char *
//    mainly to prevent errors on CTF neutral flag and out of range team number.
StringTableEntry GameType::getTeamName(S32 team)
{
   
   if(team >= 0 && team < mTeams.size())
      return mTeams[team].getName();
   else if(team == -2)
      return StringTableEntry("Hostile");
   else if(team == -1)
      return StringTableEntry("Neutral");
   else
      return StringTableEntry("UNKNOWN");
}


Color GameType::getTeamColor(GameObject *theObject)
{
   return getTeamColor(theObject->getTeam());
}


Color GameType::getShipColor(Ship *s)
{
   return getTeamColor(s->getTeam());
}


// Make sure that the mTeams[] structure has the proper player counts
// Needs to be called manually before accessing the structure
// Rating may only work on server... not tested on client
void GameType::countTeamPlayers()
{
   for(S32 i = 0; i < mTeams.size(); i++)
   {
      mTeams[i].numPlayers = 0;
      mTeams[i].numBots = 0;
      mTeams[i].rating = 0;
   }

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->isRobot)
         mTeams[mClientList[i]->getTeam()].numBots++;
      else
         mTeams[mClientList[i]->getTeam()].numPlayers++;

      GameConnection *cc = mClientList[i]->clientConnection;
      if(cc)
         mTeams[mClientList[i]->getTeam()].rating += max(getCurrentRating(cc), .1);
   }
}


// Adds a new client to the game when a player joins, or when a level cycles.
// Runs on the server, can be overridden.
// Note that when a new game starts, players will be added in order from
// strongest to weakest.
// Note also that theClient should never be NULL.
void GameType::serverAddClient(GameConnection *theClient)
{
   TNLAssert(theClient, "Attempting to add a NULL client to the server!");

   theClient->setScopeObject(this);

   ClientRef *cref = allocClientRef();
   cref->name = theClient->getClientName();

   cref->clientConnection = theClient;
   countTeamPlayers();     // Also calcs team ratings

   // Figure out how many players the team with the fewest players has
   S32 minPlayers = mTeams[0].numPlayers + mTeams[0].numBots ;

   for(S32 i = 1; i < mTeams.size(); i++)
   {
      if(mTeams[i].numPlayers + mTeams[i].numBots < minPlayers)
         minPlayers = mTeams[i].numPlayers + mTeams[i].numBots;
   }

   // Of the teams with minPlayers, find the one with the lowest total rating...
   S32 minTeamIndex = 0;
   F32 minRating = F32_MAX;

   for(S32 i = 0; i < mTeams.size(); i++)
   {
      if(mTeams[i].numPlayers + mTeams[i].numBots == minPlayers && mTeams[i].rating < minRating)
      {
         minTeamIndex = i;
         minRating = mTeams[i].rating;
      }
   }

   cref->isRobot = theClient->isRobot();
   if(cref->isRobot)                     // Robots use their own team number, if in range.
   {
      Ship *ship = dynamic_cast<Ship *>(theClient->getControlObject());
      if(ship)
      {
         if(ship->getTeam() >= -2 && ship->getTeam() < mTeams.size())
            minTeamIndex = ship->getTeam();
      }
   }
   // ...and add new player to that team
   cref->setTeam(minTeamIndex);
   mClientList.push_back(cref);
   theClient->setClientRef(cref);

   s2cAddClient(cref->name, false, cref->clientConnection->isAdmin(), cref->isRobot, true);    // Tell other clients about the new guy, who is never us...
   s2cClientJoinedTeam(cref->name, cref->getTeam());

   spawnShip(theClient);
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
      return gWeapons[weaponType].damageSelfMultiplier != 0;

   // Check for friendly fire
   else if(damager->getTeam() == victim->getTeam())
      return !isTeamGame() || gWeapons[weaponType].canDamageTeammate;

   return true;
}


// Handle scoring when ship is killed
void GameType::controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject)
{
   GameConnection *killer = killerObject ? killerObject->getOwner() : NULL;
   ClientRef *killerRef = killer ? killer->getClientRef() : NULL;
   ClientRef *clientRef = theClient->getClientRef();

   theClient->mStatistics.addDeath();

   StringTableEntry killerDescr = killerObject->getKillString();

   if(killer)     // Known killer
   {
      if(killer == theClient)    // We killed ourselves -- should have gone easy with the bouncers!
      {
         killer->mStatistics.addSuicide();
         updateScore(killerRef, KillSelf);
      }

      // Should do nothing with friendly fire disabled
      else if(isTeamGame() && killerRef->getTeam() == clientRef->getTeam())   // Same team in a team game
      {
         killer->mStatistics.addFratricide();
         updateScore(killerRef, KillTeammate);
      }

      else                                                                    // Different team, or not a team game
      {
         killer->mStatistics.addKill();
         updateScore(killerRef, KillEnemy);
      }

      s2cKillMessage(clientRef->name, killerRef->name, killerObject->getKillString());
   }
   else              // Unknown killer... not a scorable event.  Unless killer was an asteroid!
   {
      if( dynamic_cast<Asteroid *>(killerObject) )       // Asteroid
         updateScore(clientRef, KilledByAsteroid, 0);
      else                                               // Check for turret shot
      {
         Projectile *projectile = dynamic_cast<Projectile *>(killerObject);

         if( projectile && projectile->mShooter.isValid() && dynamic_cast<Turret *>(projectile->mShooter.getPointer()) )
            updateScore(clientRef, KilledByTurret, 0);
      }


      s2cKillMessage(clientRef->name, NULL, killerDescr);
   }

   clientRef->respawnTimer.reset(RespawnDelay);
}


// Handle score for ship and robot
// Runs on server only?
void GameType::updateScore(Ship *ship, ScoringEvent scoringEvent, S32 data)
{
   ClientRef *cl = NULL;

   TNLAssert(ship, "Ship is null in updateScore!!");

   if(ship->getControllingClient())
      cl = ship->getControllingClient()->getClientRef();  // Get client reference for ships...

   updateScore(cl, ship->getTeam(), scoringEvent, data);
}


// Handle both individual scores and team scores
// Runs on server only
void GameType::updateScore(ClientRef *player, S32 team, ScoringEvent scoringEvent, S32 data)
{
   S32 newScore = S32_MIN;

   if(player != NULL)
   {
      // Individual scores
      S32 points = getEventScore(IndividualScore, scoringEvent, data);
      if(points != 0)
      {
         player->addScore(points);
         player->clientConnection->mScore += points;
         // (No need to broadcast score because individual scores are only displayed when Tab is held,
         // in which case scores, along with data like ping time, are streamed in)

         // Accumulate every client's total score counter
         for(S32 i = 0; i < mClientList.size(); i++)
         {
            mClientList[i]->clientConnection->mTotalScore += max(points, 0);

            if(mClientList[i]->getScore() > newScore)
               newScore = mClientList[i]->getScore();
         }
      }
   }

   if(isTeamGame())
   {
      // Just in case...  completely superfluous, gratuitous check
      if(team < 0 || team >= mTeams.size())
         return;

      S32 points = getEventScore(TeamScore, scoringEvent, data);
      if(points == 0)
         return;

      mTeams[team].addScore(points);

      // This is kind of a hack to emulate adding a point to every team *except* the scoring team.  The scoring team has its score
      // deducted, then the same amount is added to every team.  Assumes that points < 0.
      if(scoringEvent == ScoreGoalOwnTeam)
      {
         for(S32 i = 0; i < mTeams.size(); i++)
         {
            mTeams[i].addScore(-points);                    // Add magnitiude of negative score to all teams
            s2cSetTeamScore(i, mTeams[i].getScore());       // Broadcast result
         }
      }
      else  // All other scoring events
         s2cSetTeamScore(team, mTeams[team].getScore());    // Broadcast new team score

      updateLeadingTeamAndScore();
      newScore = mLeadingTeamScore;
   }

   checkForWinningScore(newScore);              // Check if score is high enough to trigger end-of-game
}


// Sets mLeadingTeamScore and mLeadingTeam; runs on client and server
void GameType::updateLeadingTeamAndScore()
{
   mLeadingTeamScore = S32_MIN;

   // Find the leading team...
   for(S32 i = 0; i < mTeams.size(); i++)
      if(mTeams[i].getScore() > mLeadingTeamScore)
      {
         mLeadingTeamScore = mTeams[i].getScore();
         mLeadingTeam = i;
      }  // no break statement in above!
}


// Different signature for more common usage
void GameType::updateScore(ClientRef *client, ScoringEvent event, S32 data)
{
   if(client)
      updateScore(client, client->getTeam(), event, data);
   // else, no one to score... sometimes client really does come in as null
}


// Signature for team-only scoring event
void GameType::updateScore(S32 team, ScoringEvent event, S32 data)
{
   updateScore(NULL, team, event, data);
}


// At game end, we need to update everyone's game-normalized ratings
void GameType::updateRatings()
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      GameConnection *conn = mClientList[i]->clientConnection;
      conn->mRating = getCurrentRating(conn);
      conn->mGamesPlayed++;
      conn->mScore = 0;
      conn->mTotalScore = 0;
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
            return 0;
         case KillTeammate:
            return 0;
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
            return 0;
         case KillEnemyTurret:
            return 0;
         case KillOwnTurret:
            return 0;
         default:
            return naScore;
      }
   }
}


extern ClientInfo gClientInfo;

static void switchTeamsCallback(U32 unused)
{
   GameType *gt = gClientGame->getGameType();
   if(!gt)
      return;

   // If there are only two teams, just switch teams and skip the rigamarole
   if(gt->mTeams.size() == 2)
   {
      Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());  // Returns player's ship...
      if(!ship)
         return;

      gt->c2sChangeTeams(1 - ship->getTeam());                 // If two teams, team will either be 0 or 1, so "1 - " will toggle
      UserInterface::reactivateMenu(gGameUserInterface);       // Jump back into the game (this option takes place immediately)
   }
   else
   {
      gTeamMenuUserInterface.activate();     // Show menu to let player select a new team
      gTeamMenuUserInterface.nameToChange = gClientInfo.name.c_str();
   }
 }


// Add any additional game-specific menu items, processed below
void GameType::addClientGameMenuOptions(Vector<MenuItem *> &menuOptions)
{
   if(isTeamGame() && mTeams.size() > 1 && !mBetweenLevels)
   {
      GameConnection *gc = gClientGame->getConnectionToServer();

      if(mCanSwitchTeams || (gc && gc->isAdmin()))
         menuOptions.push_back(new MenuItem(0, "SWITCH TEAMS", switchTeamsCallback, "", KEY_S, KEY_T));
      else
      {
         menuOptions.push_back(new MenuItem(0, "WAITING FOR SERVER TO ALLOW", NULL, "", KEY_UNKNOWN, KEY_UNKNOWN, Color(1,0,0)));
         menuOptions.push_back(new MenuItem(0, "YOU TO SWITCH TEAMS",         NULL, "", KEY_UNKNOWN, KEY_UNKNOWN, Color(1,0,0)));
      }
   }
}


static void switchPlayersTeamCallback(U32 unused)
{
   gPlayerMenuUserInterface.action = PlayerMenuUserInterface::ChangeTeam;
   gPlayerMenuUserInterface.activate();
}


// Add any additional game-specific admin menu items, processed below
void GameType::addAdminGameMenuOptions(Vector<MenuItem *> &menuOptions)
{
   if(isTeamGame() && mTeams.size() > 1)
      menuOptions.push_back(new MenuItem(0, "CHANGE A PLAYER'S TEAM", switchPlayersTeamCallback, "", KEY_C));
}


// Broadcast info about the current level... code gets run on client, obviously
// Note that if we add another arg to this, we need to further expand FunctorDecl methods in tnlMethodDispatch.h
GAMETYPE_RPC_S2C(GameType, s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, 
                                                StringTableEntry levelCreds, S32 objectCount, F32 lx, F32 ly, F32 ux, F32 uy, 
                                                bool levelHasLoadoutZone, bool engineerEnabled),
                                            (levelName, levelDesc, teamScoreLimit, 
                                                levelCreds, objectCount, lx, ly, ux, uy, 
                                                levelHasLoadoutZone, engineerEnabled))
{
   mLevelName = levelName;
   mLevelDescription = levelDesc;
   mLevelCredits = levelCreds;
   mWinningScore = teamScoreLimit;
   mObjectsExpected = objectCount;
   mEngineerEnabled = engineerEnabled;

   mViewBoundsWhileLoading = Rect(lx, ly, ux, uy);
   mLevelHasLoadoutZone = levelHasLoadoutZone;           // Need to pass this because we won't know for sure when the loadout zones will be sent, so searching for them is difficult

   gClientGame->mObjectsLoaded = 0;                      // Reset item counter
   gGameUserInterface.mShowProgressBar = true;           // Show progress bar
   //gClientGame->setInCommanderMap(true);               // If we change here, need to tell the server we are in this mode.
   //gClientGame->resetZoomDelta();

   mLevelInfoDisplayTimer.reset(LevelInfoDisplayTime);   // Start displaying the level info, now that we have it

   // Now we know all we need to initialize our loadout options
   gGameUserInterface.initializeLoadoutOptions(engineerEnabled);
}

GAMETYPE_RPC_C2S(GameType, c2sAddTime, (U32 time), (time))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());

   if(!source->isLevelChanger())                         // Level changers and above
      return;

   mGameTimer.reset(mGameTimer.getCurrent() + time);     // Increment "official time"
   s2cSetTimeRemaining(mGameTimer.getCurrent());         // Broadcast time to clients

   static StringTableEntry msg("%e0 has extended the game");
   Vector<StringTableEntry> e;
   e.push_back(source->getClientName());
   for(S32 i = 0; i < mClientList.size(); i++)
      mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
}


GAMETYPE_RPC_C2S(GameType, c2sChangeTeams, (S32 team), (team))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());

   if(!source->isAdmin() && source->mSwitchTimer.getCurrent())                // If we're not admin and we're waiting for our switch-expiration to reset,
      return;                                                                 // return without processing the change team request

   changeClientTeam(source, team);

   if(!source->isAdmin())
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
void GameType::changeClientTeam(GameConnection *source, S32 team)
{
   if(mTeams.size() <= 1)     // Can't change if there's only one team...
      return;

   ClientRef *cl = source->getClientRef();
   if(cl->getTeam() == team)     // Don't explode if not switching team.
      return;

   Ship *ship = dynamic_cast<Ship *>(source->getControlObject());    // Get the ship that's switching

   if(ship)
   {
      // Find all spybugs and mines that this player owned, and reset ownership
      Rect worldBounds = gServerGame->computeWorldObjectExtents();

      fillVector.clear();
      gServerGame->getGridDatabase()->findObjects(SpyBugType | MineType, fillVector, gServerWorldBounds);

      for(S32 i = 0; i < fillVector.size(); i++)
      {
         GameObject *obj = dynamic_cast<GameObject *>(fillVector[i]);

         if((obj->getOwner()) == ship->getOwner())
            obj->setOwner(NULL);
      }

      if(ship->isRobot())           // Players get a new ship object, Robots use the same ship object.
         ship->setMaskBits(Ship::ChangeTeamMask);

      ship->kill();                 // Destroy the old ship

      cl->respawnTimer.clear();     // If we've just died, this will keep a second copy of ourselves from appearing
   }

   if(team == -1)                                        // If no team provided...
      cl->setTeam((cl->getTeam() + 1) % mTeams.size());  // ...find the next one...
   else                                                  // ...otherwise...
      cl->setTeam(team);                                 // ...use the one provided

   s2cClientJoinedTeam(cl->name, cl->getTeam());         // Announce the change
   spawnShip(source);                                    // Create a new ship
   cl->clientConnection->switchedTeamCount++;            // Count number of times team is switched.
}


GAMETYPE_RPC_S2C(GameType, s2cAddClient, 
                (StringTableEntry name, bool isMyClient, bool admin, bool isRobot, bool playAlert), 
                (name, isMyClient, admin, isRobot, playAlert))
{
   ClientRef *cref = allocClientRef();
   cref->name = name;
   cref->setTeam(0);
   cref->isAdmin = admin;
   cref->isRobot = isRobot;

   cref->decoder = new LPC10VoiceDecoder();
   cref->voiceSFX = new SFXObject(SFXVoice, NULL, 1, Point(), Point());

   mClientList.push_back(cref);

   if(isMyClient)
   {
      mLocalClient = cref;

      // Now we'll check if we need an updated scoreboard... this only needed to handle use case of user
      // holding Tab while one game transitions to the next.  Without it, ratings will be reported as 0.
      if(gGameUserInterface.isInScoreboardMode())
      {
         GameType *g = gClientGame->getGameType();
         if(g)
            g->c2sRequestScoreboardUpdates(true);
      }
      gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "Welcome to the game!");
   }
   else
   {
      gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", name.getString());      
      if(playAlert)
         SFXObject::play(SFXPlayerJoined, 1);
   }
}


void GameType::serverRemoveClient(GameConnection *theClient)
{
   ClientRef *cl = theClient->getClientRef();
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i] == cl)
      {
         mClientList.erase(i);
         break;
      }
   }

   // Blow up the ship...
   GameObject *theControlObject = theClient->getControlObject();
   if(theControlObject)
   {
      Ship *ship = dynamic_cast<Ship *>(theControlObject);
      if(ship)
         ship->kill();
   }

   s2cRemoveClient(theClient->getClientName());
}

//extern void updateClientChangedName(GameConnection *,StringTableEntry);  //in masterConnection.cpp

// Server notifies clients that a player has changed name
GAMETYPE_RPC_S2C(GameType, s2cRenameClient, (StringTableEntry oldName, StringTableEntry newName), (oldName, newName))
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->name == oldName)
      {
         mClientList[i]->name = newName;
         break;
      }
   }
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s changed to %s", oldName.getString(), newName.getString());
}

// Server notifies clients that a player has left the game
GAMETYPE_RPC_S2C(GameType, s2cRemoveClient, (StringTableEntry name), (name))
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->name == name)
      {
         mClientList.erase(i);
         break;
      }
   }
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s left the game.", name.getString());
   SFXObject::play(SFXPlayerLeft, 1);
}

GAMETYPE_RPC_S2C(GameType, s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b), (teamName, r, g, b))
{
   Team team;
   team.setName(teamName);
   team.color.r = r;
   team.color.g = g;
   team.color.b = b;
   mTeams.push_back(team);
}

GAMETYPE_RPC_S2C(GameType, s2cSetTeamScore, (RangedU32<0, GameType::gMaxTeams> teamIndex, U32 score), (teamIndex, score))
{
   mTeams[teamIndex].setScore(score);
   updateLeadingTeamAndScore();    
}


// Server has sent us (the client) a message telling us how much longer we have in the current game
GAMETYPE_RPC_S2C(GameType, s2cSetTimeRemaining, (U32 timeLeft), (timeLeft))
{
   mGameTimer.reset(timeLeft);
}


// Server has sent us (the client) a message telling us the winning score has changed, and who changed it
GAMETYPE_RPC_S2C(GameType, s2cChangeScoreToWin, (U32 winningScore, StringTableEntry changer), (winningScore, changer))
{
   mWinningScore = winningScore;
   gGameUserInterface.displayMessage(Color(0.6f, 1, 0.8f) /*Nuclear green */, 
               "%s changed the winning score to %d.", changer.getString(), mWinningScore);
}


// Announce a new player has joined the team
GAMETYPE_RPC_S2C(GameType, s2cClientJoinedTeam, 
                (StringTableEntry name, RangedU32<0, GameType::gMaxTeams> teamIndex), 
                (name, teamIndex))
{
   ClientRef *cl = findClientRef(name);      // Will be us, if we changed teams
   cl->setTeam((S32) teamIndex);

   // The following works as long as everyone runs with a unique name.  Fails if two players have names that collide and have
   // TODO: Better place to get current player's name?  This may fail if users have same name, and system has changed it
   // been corrected by the server.
   if(gClientGame->getGameType()->mLocalClient && name == gClientGame->getGameType()->mLocalClient->name)      
      gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "You have joined team %s.", getTeamName(teamIndex).getString());
   else
      gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined team %s.", name.getString(), getTeamName(teamIndex).getString());

   // Make this client forget about any mines or spybugs he knows about... it's a bit of a kludge to do this here,
   // but this RPC only runs when a player joins the game or changes teams, so this will never hurt, and we can
   // save the overhead of sending a separate message which, while theoretically cleaner, will never be needed practically.
   Rect worldBounds = gClientGame->computeWorldObjectExtents();

   fillVector.clear();
   gClientGame->getGridDatabase()->findObjects(SpyBugType | MineType, fillVector, worldBounds);

   for(S32 i = 0; i < fillVector.size(); i++)
   {
      GrenadeProjectile *gp = dynamic_cast<GrenadeProjectile *>(fillVector[i]);
      if(gp->mSetBy == name)
      {
         gp->mSetBy = "";                                    // No longer set-by-self
         U32 mask = gp->getObjectTypeMask();
         gp->setObjectTypeMask(mask &= ~CommandMapVisType);  // And no longer visible on commander's map
      }
   }
}


// Announce a new player has become an admin
GAMETYPE_RPC_S2C(GameType, s2cClientBecameAdmin, (StringTableEntry name), (name))
{
   ClientRef *cl = findClientRef(name);
   cl->isAdmin = true;
   if(gClientGame->getGameType()->mClientList.size() && name != gClientGame->getGameType()->mLocalClient->name)    // Don't show message to self
      gGameUserInterface.displayMessage(Color(0,1,1), "%s has been granted administrator access.", name.getString());
}


// Announce a new player has permission to change levels
GAMETYPE_RPC_S2C(GameType, s2cClientBecameLevelChanger, (StringTableEntry name), (name))
{
   ClientRef *cl = findClientRef(name);
   cl->isLevelChanger = true;
   if(gClientGame->getGameType()->mClientList.size() && name != gClientGame->getGameType()->mLocalClient->name)    // Don't show message to self
      gGameUserInterface.displayMessage(Color(0,1,1), "%s can now change levels.", name.getString());
}

// Runs after the server knows that the client is available and addressable via the getGhostIndex()
// Runs on server, obviously
void GameType::onGhostAvailable(GhostConnection *theConnection)
{
   NetObject::setRPCDestConnection(theConnection);    // Focus all RPCs on client only

   Rect barrierExtents = gServerGame->computeBarrierExtents();

   s2cSetLevelInfo(mLevelName, mLevelDescription, mWinningScore, mLevelCredits, gServerGame->mObjectsLoaded, 
                   barrierExtents.min.x, barrierExtents.min.y, barrierExtents.max.x, barrierExtents.max.y, 
                   mLevelHasLoadoutZone, mEngineerEnabled);

   for(S32 i = 0; i < mTeams.size(); i++)
   {
      s2cAddTeam(mTeams[i].getName(), mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);
      s2cSetTeamScore(i, mTeams[i].getScore());
   }

   // Add all the client and team information
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      bool localClient = mClientList[i]->clientConnection == theConnection;

      s2cAddClient(mClientList[i]->name, localClient, mClientList[i]->clientConnection->isAdmin(), mClientList[i]->isRobot, false);
      s2cClientJoinedTeam(mClientList[i]->name, mClientList[i]->getTeam());
   }

   //for(S32 i = 0; i < Robot::robots.size(); i++)  //Robot is part of mClientList
   //{
   //   s2cAddClient(Robot::robots[i]->getName(), false, false, true, false);
   //   s2cClientJoinedTeam(Robot::robots[i]->getName(), Robot::robots[i]->getTeam());
   //}

   // An empty list clears the barriers
   Vector<F32> v;
   s2cAddBarriers(v, 0, false);

   for(S32 i = 0; i < mBarriers.size(); i++)
      s2cAddBarriers(mBarriers[i].verts, mBarriers[i].width, mBarriers[i].solid);

   s2cSetTimeRemaining(mGameTimer.getCurrent());      // Tell client how much time left in current game
   s2cSetGameOver(mGameOver);
   s2cSyncMessagesComplete(theConnection->getGhostingSequence());

   NetObject::setRPCDestConnection(NULL);             // Set RPCs to go to all players
}


GAMETYPE_RPC_S2C(GameType, s2cSyncMessagesComplete, (U32 sequence), (sequence))
{
   // Now we know the game is ready to begin...
   mBetweenLevels = false;
   c2sSyncMessagesComplete(sequence);

   gClientGame->prepareBarrierRenderingGeometry();    // Get walls ready to render

   gGameUserInterface.mShowProgressBar = false;
   //gClientGame->setInCommanderMap(false);             // Start game in regular mode, If we change here, need to tell the server we are in this mode. Map can change while in commander map.
   //gClientGame->clearZoomDelta();                     // No in zoom effect
   
   gGameUserInterface.mProgressBarFadeTimer.reset(1000);
}


// Client acknowledges that it has recieved s2cSyncMessagesComplete, and is ready to go
GAMETYPE_RPC_C2S(GameType, c2sSyncMessagesComplete, (U32 sequence), (sequence))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   if(sequence != source->getGhostingSequence())
      return;

   cl->readyForRegularGhosts = true;
}


// Gets called multiple times as barriers are added
GAMETYPE_RPC_S2C(GameType, s2cAddBarriers, (Vector<F32> barrier, F32 width, bool solid), (barrier, width, solid))
{
   if(!barrier.size())
      getGame()->deleteObjects(BarrierType);
   else
      constructBarriers(getGame(), barrier, width, solid);
}



// Runs the server side commands, which the client may or may not know about

// This is server side commands, For client side commands, use UIGame.cpp, GameUserInterface::processCommand.
// When adding new commands, please update GameUserInterface::populateChatCmdList() and also the help screen (UIInstructions.cpp)
void GameType::processServerCommand(ClientRef *clientRef, const char *cmd, Vector<StringPtr> args)
{
   //if(!stricmp(cmd, ""))  return;     // removed, it will not cause any errors, we simply not going to find "" command

   if(!stricmp(cmd, "settime"))
   {
      if(!clientRef->clientConnection->isLevelChanger())
         clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Need level change permission");
      else if(args.size() < 1)
         clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Enter time in minutes");
      else
      {
         F32 time = atof(args[0].getString());
         if(time < 0 || time == 0 && (stricmp(args[0].getString(), "0") && stricmp(args[0].getString(), "unlim")))  // 0 --> unlimited
            clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Invalid time... game time not changed");
         else
         {
            mGameTimer.reset((U32)(60 * 1000 * time));       // Change "official time"

            s2cSetTimeRemaining(mGameTimer.getCurrent());    // Broadcast time to clients

            static StringTableEntry msg("%e0 has changed the amount of time left in the game");
            Vector<StringTableEntry> e;
            e.push_back(clientRef->clientConnection->getClientName());

            for(S32 i = 0; i < mClientList.size(); i++)
               mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
         }
      }
   }
   else if(!stricmp(cmd, "setscore"))
   {
     if(!clientRef->clientConnection->isLevelChanger())                         // Level changers and above
         clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Need level change permission");
     else if(args.size() < 1)
         clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Enter score limit");
     else
     {
         S32 score = atoi(args[0].getString());
         if(score <= 0)    // 0 can come about if user enters invalid input
            clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Invalid score... winning score not changed");
         else
         {
            mWinningScore = score;
            s2cChangeScoreToWin(mWinningScore, clientRef->clientConnection->getClientName());
         }
     }
   }
   else if(!stricmp(cmd, "showbots") || !stricmp(cmd, "showbot"))    // Maybe there is only one bot to show :-)
   {
      mShowAllBots = !mShowAllBots;  // Show all robots affects all players
      if(Robot::robots.size() == 0)
         clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! There are no robots to show");
      else
      {
         StringTableEntry msg = mShowAllBots ? StringTableEntry("Show all robots option enabled by %e0") : StringTableEntry("Show all robots option disabled by %e0");
         Vector<StringTableEntry> e;
         e.push_back(clientRef->clientConnection->getClientName());
         for(S32 i = 0; i < mClientList.size(); i++)
            mClientList[i]->clientConnection->s2cDisplayMessageE(GameConnection::ColorNuclearGreen, SFXNone, msg, e);
      }
   }
	/* /// Remove this command
   else if(!stricmp(cmd, "rename") && args.size() >= 1)
   {
		//for testing, might want to remove this once it is fully working.
		StringTableEntry oldName = clientRef->clientConnection->getClientName();
		clientRef->clientConnection->setClientName(StringTableEntry(""));       //avoid unique self
		StringTableEntry uniqueName = GameConnection::makeUnique(args[0].getString()).c_str();  //new name
		clientRef->clientConnection->setClientName(oldName);                   //restore name to properly get it updated to clients.

		clientRef->clientConnection->setAuthenticated(false);         //don't underline anymore because of rename
		updateClientChangedName(clientRef->clientConnection,uniqueName);
   }
	*/
   else
   {
      // Command not found, tell the client
      clientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Invalid Command");
   }
}

GAMETYPE_RPC_C2S(GameType, c2sSendCommand, (StringTableEntry cmd, Vector<StringPtr> args), (cmd, args))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *clientRef = source->getClientRef();

   processServerCommand(clientRef, cmd.getString(), args);
}


// Send a private message
GAMETYPE_RPC_C2S(GameType, c2sSendChatPM, (StringTableEntry toName, StringPtr message), (toName, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *sourceClientRef = source->getClientRef();

   bool found = false;
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->clientConnection && mClientList[i]->name == toName)     // Do we want a case insensitive search?
      {
         RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cDisplayChatPM, (source->getClientName(), toName, message));
         sourceClientRef->clientConnection->postNetEvent(theEvent);

         if(sourceClientRef != mClientList[i])      // A user might send a message to themselves
            mClientList[i]->clientConnection->postNetEvent(theEvent);

         found = true;
         break;
      }
   }

   if(!found)
      sourceClientRef->clientConnection->s2cDisplayMessage(GameConnection::ColorRed, SFXNone, "!!! Player name not found");
}


// Client sends chat message to/via game server
GAMETYPE_RPC_C2S(GameType, c2sSendChat, (bool global, StringPtr message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *clientRef = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this,
                                 s2cDisplayChatMessage, (global, source->getClientName(), message));
   sendChatDisplayEvent(clientRef, global, message.getString(), theEvent);
}


// Sends a quick-chat message (which, due to its repeated nature can be encapsulated in a StringTableEntry item)
GAMETYPE_RPC_C2S(GameType, c2sSendChatSTE, (bool global, StringTableEntry message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *clientRef = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this,
      s2cDisplayChatMessageSTE, (global, source->getClientName(), message));

   sendChatDisplayEvent(clientRef, global, message.getString(), theEvent);
}


// Send a chat message that will be displayed in-game
// If not global, send message only to other players on team
void GameType::sendChatDisplayEvent(ClientRef *clientRef, bool global, const char *message, NetEvent *theEvent)
{
   S32 teamId = 0;

   if(!global)
      teamId = clientRef->getTeam();

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(global || mClientList[i]->getTeam() == teamId)
         if(mClientList[i]->clientConnection)
            mClientList[i]->clientConnection->postNetEvent(theEvent);
   }

   // And fire an event handler...
   Robot::getEventManager().fireEvent(NULL, EventManager::MsgReceivedEvent, message, clientRef->getPlayerInfo(), global);
}


extern Color gGlobalChatColor;
extern Color gTeamChatColor;

// Server sends message to the client for display using StringPtr
GAMETYPE_RPC_S2C(GameType, s2cDisplayChatPM, (StringTableEntry fromName, StringTableEntry toName, StringPtr message), (fromName, toName, message))
{
   Color theColor = Color(1,1,0);
   if(mLocalClient->name == toName && toName == fromName)      // Message sent to self
      gGameUserInterface.displayMessage(theColor, "%s: %s", toName.getString(), message.getString());  

   else if(mLocalClient->name == toName)                       // To this player
      gGameUserInterface.displayMessage(theColor, "from %s: %s", fromName.getString(), message.getString());

   else if(mLocalClient->name == fromName)                     // From this player
      gGameUserInterface.displayMessage(theColor, "to %s: %s", toName.getString(), message.getString());

   else                // Should never get here... shouldn't be able to see PM that is not from or not to you
      gGameUserInterface.displayMessage(theColor, "from %s to %s: %s", fromName.getString(), toName.getString(), message.getString());
}


GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessage, (bool global, StringTableEntry clientName, StringPtr message), (global, clientName, message))
{
   Color theColor = global ? gGlobalChatColor : gTeamChatColor;
   gGameUserInterface.displayMessage(theColor, "%s: %s", clientName.getString(), message.getString());
}


// Server sends message to the client for display using StringTableEntry
GAMETYPE_RPC_S2C(GameType, s2cDisplayChatMessageSTE, (bool global, StringTableEntry clientName, StringTableEntry message), (global, clientName, message))
{
   Color theColor = global ? gGlobalChatColor : gTeamChatColor;
   gGameUserInterface.displayMessage(theColor, "%s: %s", clientName.getString(), message.getString());
}


// Client requests start/stop of streaming pings and scores from server to client
GAMETYPE_RPC_C2S(GameType, c2sRequestScoreboardUpdates, (bool updates), (updates))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   cl->wantsScoreboardUpdates = updates;
   if(updates)
      updateClientScoreboard(cl);
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


GAMETYPE_RPC_C2S(GameType, c2sReaffirmMountItem, (U16 itemId), (itemId))
{
   //logprintf("%s GameType->c2sReaffirmMountItem", isGhost()? "Client:" : "Server:");
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   for(S32 i = 0; i < gServerGame->mGameObjects.size(); i++)
   {
      Item *item = dynamic_cast<Item *>(gServerGame->mGameObjects[i]);
      if(item && item->getItemId() == itemId)
      {
         item->setMountedMask();
         break;
      }
   }

   //Ship *ship = dynamic_cast<Ship *>(source->getControlObject());
   //if(!ship)
   //   return;

   //S32 count = ship->mMountedItems.size();

   //for(S32 i = count - 1; i >= 0; i--)
   //   ship->mMountedItems[i]->setMountedMask();

}


GAMETYPE_RPC_C2S(GameType, c2sResendItemStatus, (U16 itemId), (itemId))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();

   for(S32 i = 0; i < gServerGame->mGameObjects.size(); i++)
   {
      Item *item = dynamic_cast<Item *>(gServerGame->mGameObjects[i]);
      if(item && item->getItemId() == itemId)
      {
         item->setPositionMask();
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


const U32 minRating = 0;
const U32 maxRating = 200;

Vector<RangedU32<0, GameType::MaxPing> > GameType::mPingTimes; ///< Static vector used for constructing update RPCs
Vector<SignedInt<24> > GameType::mScores;
Vector<RangedU32<minRating,maxRating> > GameType::mRatings;


void GameType::updateClientScoreboard(ClientRef *cl)
{
   mPingTimes.clear();
   mScores.clear();
   mRatings.clear();

   // First, list the players
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->ping < MaxPing)
         mPingTimes.push_back(mClientList[i]->ping);
      else
         mPingTimes.push_back(MaxPing);

      mScores.push_back(mClientList[i]->getScore());

      GameConnection *conn = mClientList[i]->clientConnection;

      // Players rating = cumulative score / total score played while this player was playing, ranks from 0 to 1
      mRatings.push_back(min((U32)(getCurrentRating(conn) * 100.0) + 100, maxRating));
   }

   // Next come the robots ... Robots is part of mClientList
   //for(S32 i = 0; i < Robot::robots.size(); i++)
   //{
   //   mPingTimes.push_back(0);
   //   mScores.push_back(Robot::robots[i]->getScore());
   //   mRatings.push_back(max(min((U32)(Robot::robots[i]->getRating() * 100.0) + 100, maxRating), minRating));
   //}

   NetObject::setRPCDestConnection(cl->clientConnection);
   s2cScoreboardUpdate(mPingTimes, mScores, mRatings);
   NetObject::setRPCDestConnection(NULL);
}


GAMETYPE_RPC_S2C(GameType, s2cScoreboardUpdate,
                 (Vector<RangedU32<0, GameType::MaxPing> > pingTimes, Vector<SignedInt<24> > scores, Vector<RangedU32<minRating,maxRating> > ratings),
                 (pingTimes, scores, ratings))
{
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(i >= pingTimes.size())
         break;

      mClientList[i]->ping = pingTimes[i];
      mClientList[i]->setScore(scores[i]);
      mClientList[i]->setRating(((F32)ratings[i] - 100.0) / 100.0);
   }
}


GAMETYPE_RPC_S2C(GameType, s2cKillMessage, (StringTableEntry victim, StringTableEntry killer, StringTableEntry killerDescr), (victim, killer, killerDescr))
{
   if(killer)  // Known killer, was self, robot, or another player
   {
      if(killer == victim)
         if(killerDescr == "mine")
            gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by own mine", victim.getString());
         else
            gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s zapped self", victim.getString());
      else
         if(killerDescr == "mine")
            gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s was destroyed by mine put down by %s", victim.getString(), killer.getString());
         else
            gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s zapped %s", killer.getString(), victim.getString());
   }
   else if(killerDescr == "mine")   // Killer was some object with its own kill description string
      gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s got blown up by a mine", victim.getString());
   else if(killerDescr != "")
      gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s %s", victim.getString(), killerDescr.getString());
   else         // Killer unknown
      gGameUserInterface.displayMessage(Color(1.0f, 1.0f, 0.8f), "%s got zapped", victim.getString());
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, c2sVoiceChat, (bool echo, ByteBufferPtr voiceBuffer), (echo, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhostParent, 0)
{
   // Broadcast this to all clients on the same team
   // Only send back to the source if echo is true.

   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   if(cl)
   {
      RefPtr<NetEvent> event = TNL_RPC_CONSTRUCT_NETEVENT(this, s2cVoiceChat, (cl->name, voiceBuffer));
      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->getTeam() == cl->getTeam() && (mClientList[i] != cl || echo) && mClientList[i]->clientConnection)
            mClientList[i]->clientConnection->postNetEvent(event);
      }
   }
}


TNL_IMPLEMENT_NETOBJECT_RPC(GameType, s2cVoiceChat, (StringTableEntry clientName, ByteBufferPtr voiceBuffer), (clientName, voiceBuffer),
   NetClassGroupGameMask, RPCUnguaranteed, RPCToGhost, 0)
{
   ClientRef *cl = findClientRef(clientName);
   if(cl)
   {
      ByteBufferPtr playBuffer = cl->decoder->decompressBuffer(*(voiceBuffer.getPointer()));
      cl->voiceSFX->queueBuffer(playBuffer);
   }
}

};


