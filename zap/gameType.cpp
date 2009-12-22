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
#include "UIGame.h"
#include "UIMenus.h"
#include "UINameEntry.h"
#include "gameNetInterface.h"
#include "flagItem.h"
#include "gameItems.h"     // For asteroid def.
#include "engineeredObjects.h"
#include "gameObjectRender.h"
#include "config.h"
#include "projectile.h"     // For s2cClientJoinedTeam()

#include "../glut/glutInclude.h"

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{

//RDW GCC needs this to be properly defined.  -- isn't this defined in gameType.h? -CE
//GCC can't link without this definition.  One of the calls to min in
//UITeamDefMenu.cpp requires that this have actual storage (for some reason).
// I don't even know what that means!! -CE
#ifdef TNL_OS_MAC_OSX
const S32 GameType::gMaxTeams;
#endif

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
   0  // Last item must be null
};

S32 gDefaultGameTypeIndex = 0;  // What we'll default to if the name provided is invalid or missing

///////////////////////////////////////////////


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
   mCanSwitchTeams = true;    // Players can switch right away
   mLocalClient = NULL;       // Will be assigned by the server after a connection is made
   mZoneGlowTimer.setPeriod(mZoneGlowTime);
   mGlowingZoneTeam = -1;     // By default, all zones glow
   mLevelHasLoadoutZone = false;
}


bool GameType::processArguments(S32 argc, const char **argv)
{
   if(argc > 0)      // First arg is game length, in minutes
      mGameTimer.reset(U32(atof(argv[0]) * 60 * 1000));

   if(argc > 1)      // Second arg is winning score
      mWinningScore = atoi(argv[1]);

   return true;
}


// Describe the arguments processed above...
Vector<GameType::ParameterDescription> GameType::describeArguments()
{
   Vector<GameType::ParameterDescription> descr;
   GameType::ParameterDescription item;

   item.name = "Game Time:";
   item.help = "Time game will last";
   item.value = 8;
   item.units = "mins";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   item.name = "Score to Win:";
   item.help = "Game ends when one team gets this score";
   item.value = 10;
   item.units = "points";
   item.minval = 1;
   item.maxval = 99;
   descr.push_back(item);

   return descr;
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
      printf("\tCan Damage Shooter: %s\n", gWeapons[i].canDamageSelf ? "Yes" : "No");
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

         string teamScoreStr = (teamScore == naScore) ? "N/A" : UserInterface::itos(teamScore);
         string indScoreStr =  (indScore == naScore)  ? "N/A" : UserInterface::itos(indScore);

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
   for(S32 i = 0; gGameTypeNames[i]; i++)
      if(!strcmp(gGameTypeNames[i], gtype))
         return gGameTypeNames[i];

   // If we get to here, no valid game type was specified, so we'll return the default
   return gGameTypeNames[gDefaultGameTypeIndex];
}


void GameType::idle(GameObject::IdleCallPath path)
{
   U32 deltaT = mCurrentMove.time;

   if(isGhost())     // i.e. client only
   {
      // Update overlay message timers
      mLevelInfoDisplayTimer.update(deltaT);
      mInputModeChangeAlertDisplayTimer.update(deltaT);

      mGameTimer.update(deltaT);
      mZoneGlowTimer.update(deltaT);
      return;
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

   // If game time has expired... game is over, man, it's over
   if(mGameTimer.update(deltaT))
      gameOverManGameOver();
}


// Sorts players by score
S32 QSORT_CALLBACK scoreSort(RefPtr<ClientRef> *a, RefPtr<ClientRef> *b)
{
   return a->getPointer()->score < b->getPointer()->score;
}


// Sorts teams by score
S32 QSORT_CALLBACK teamScoreSort(Team *a, Team *b)
{
   return a->score > b->score;
}


extern CmdLineSettings gCmdLineSettings;
extern IniSettings gIniSettings;

void GameType::renderInterfaceOverlay(bool scoreboardVisible)
{
   if(mLevelInfoDisplayTimer.getCurrent() || gGameUserInterface.mMissionOverlayActive)
   {
      F32 alpha = 1;
      if(mLevelInfoDisplayTimer.getCurrent() < 1000 && !gGameUserInterface.mMissionOverlayActive)
         alpha = mLevelInfoDisplayTimer.getCurrent() * 0.001f;

      glEnable(GL_BLEND);
         glColor4f(1, 1, 1, alpha);
         UserInterface::drawCenteredStringf(UserInterface::canvasHeight / 2 - 180, 30, "Level: %s", mLevelName.getString());
         UserInterface::drawCenteredStringf(UserInterface::canvasHeight / 2 - 140, 30, "Game Type: %s", getGameTypeString());
         glColor4f(0, 1, 1, alpha);
         UserInterface::drawCenteredString(UserInterface::canvasHeight / 2 - 100, 20, getInstructionString());
         glColor4f(1, 0, 1, alpha);
         UserInterface::drawCenteredString(UserInterface::canvasHeight / 2 - 75, 20, mLevelDescription.getString());

         glColor4f(0, 1, 0, alpha);
         UserInterface::drawCenteredStringf(UserInterface::canvasHeight - 100, 20, "Press [%s] to see this information again", keyCodeToString(keyMISSION));

         if(strcmp(mLevelCredits.getString(), ""))    // Credits string is not empty
         {
            glColor4f(1, 0, 0, alpha);
            UserInterface::drawCenteredStringf(UserInterface::canvasHeight / 2 + 50, 20, "%s", mLevelCredits.getString());
         }

         glColor4f(1, 1, 0, alpha);
         UserInterface::drawCenteredStringf(UserInterface::canvasHeight / 2 - 50, 20, "Score to Win: %d", mWinningScore);

      glDisable(GL_BLEND);

      mInputModeChangeAlertDisplayTimer.reset(0);     // Supress mode change alert if this message is displayed...
   }

   if(mInputModeChangeAlertDisplayTimer.getCurrent() != 0)
   {
      // Display alert about input mode changing
      F32 alpha = 1;
      if(mInputModeChangeAlertDisplayTimer.getCurrent() < 1000)
         alpha = mInputModeChangeAlertDisplayTimer.getCurrent() * 0.001f;

      glEnable(GL_BLEND);
      glColor4f(1, 0.5 , 0.5, alpha);
      UserInterface::drawCenteredStringf(UserInterface::vertMargin + 130, 20, "Input mode changed to %s", gIniSettings.inputMode == Joystick ? "Joystick" : "Keyboard");
      glDisable(GL_BLEND);
   }

   if((mGameOver || scoreboardVisible) && mTeams.size() > 0)      // Render scoreboard
   {
      U32 totalWidth = UserInterface::canvasWidth - UserInterface::horizMargin * 2;
      S32 teams = isTeamGame() ? mTeams.size() : 1;

      U32 columnCount = teams;
      if(columnCount > 2)
         columnCount = 2;

      U32 teamWidth = totalWidth / columnCount;
      U32 maxTeamPlayers = 0;
      countTeamPlayers();

      // Check to make sure at least one team has at least one player...
      for(S32 i = 0; i < mTeams.size(); i++)
      {
         if(isTeamGame())
         {     // (braces required)
            if(mTeams[i].numPlayers > maxTeamPlayers)
               maxTeamPlayers = mTeams[i].numPlayers;
         }
         else
            maxTeamPlayers += mTeams[i].numPlayers;
      }
      // ...if not, then go home!
      if(!maxTeamPlayers)
         return;

      U32 teamAreaHeight = isTeamGame() ? 40 : 0;
      U32 numTeamRows = (mTeams.size() + 1) >> 1;

      U32 totalHeight = (UserInterface::canvasHeight - UserInterface::vertMargin * 2) / numTeamRows - (numTeamRows - 1) * 2;
      U32 maxHeight = min(30, (totalHeight - teamAreaHeight) / maxTeamPlayers);

      U32 sectionHeight = (teamAreaHeight + maxHeight * maxTeamPlayers);
      totalHeight = sectionHeight * numTeamRows + (numTeamRows - 1) * 2;

      for(S32 i = 0; i < teams; i++)
      {
         S32 yt = (UserInterface::canvasHeight - totalHeight) / 2 + (i >> 1) * (sectionHeight + 2);  // y-top
         S32 yb = yt + sectionHeight;     // y-bottom
         S32 xl = 10 + (i & 1) * teamWidth;
         S32 xr = xl + teamWidth - 2;

         Color c = getTeamColor(i);
         glEnable(GL_BLEND);

         glColor4f(c.r, c.g, c.b, 0.6);
         glBegin(GL_POLYGON);
            glVertex2f(xl, yt);
            glVertex2f(xr, yt);
            glVertex2f(xr, yb);
            glVertex2f(xl, yb);
         glEnd();

         glDisable(GL_BLEND);

         glColor3f(1,1,1);
         if(isTeamGame())     // Render team scores
         {
            renderFlag(Point(xl + 20, yt + 18), c);
            renderFlag(Point(xr - 20, yt + 18), c);

            glColor3f(1,1,1);
            glBegin(GL_LINES);
               glVertex2f(xl, yt + teamAreaHeight);
               glVertex2f(xr, yt + teamAreaHeight);
            glEnd();

            UserInterface::drawString(xl + 40, yt + 2, 30, getTeamName(i));
            UserInterface::drawStringf(xr - 140, yt + 2, 30, "%d", mTeams[i].score);
         }

         Vector<RefPtr<ClientRef> > playerScores;

         // Now for player scores.  First build a list, then sort it, then display it.
         S32 curRowY = yt + teamAreaHeight + 1;
         S32 fontSize = U32(maxHeight * 0.8f);

         for(S32 j = 0; j < mClientList.size(); j++)
            if(mClientList[j]->teamId == i || !isTeamGame())
               playerScores.push_back(mClientList[j]);

         playerScores.sort(scoreSort);


         for(S32 j = 0; j < playerScores.size(); j++)
         {
            UserInterface::drawString(xl + 40, curRowY, fontSize, playerScores[j]->name.getString());

            static char buff[255] = "";

            if(isTeamGame())
               dSprintf(buff, sizeof(buff), "%2.2f", (F32)playerScores[j]->rating);
            else
               dSprintf(buff, sizeof(buff), "%d", playerScores[j]->score);

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
      Vector<Team> teams;

      for(S32 i = 0; i < mTeams.size(); i++)
         teams.push_back(mTeams[i]);

      teams.sort(teamScoreSort);

      for(S32 i = 0; i < teams.size(); i++)
      {
         Point pos(UserInterface::canvasWidth - UserInterface::horizMargin - 35, UserInterface::canvasHeight - UserInterface::vertMargin - lroff - i * 38);
         renderFlag(pos + Point(-20, 18), teams[i].color);
         glColor3f(1,1,1);
         UserInterface::drawStringf(pos.x, pos.y, 32, "%d", teams[i].score);
      }
   }
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


   glEnable(GL_BLEND);
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
   glDisable(GL_BLEND);

   Point cen = rp - arrowDir * 12;

   // Try labelling the objective arrows... kind of lame.
   //UserInterface::drawStringf(cen.x - UserInterface::getStringWidthf(10,"%2.1f", dist/100) / 2, cen.y - 5, 10, "%2.1f", dist/100);

   // Add an icon to the objective arrow...  kind of lame.
   //renderSmallFlag(cen, c, alpha);
}

void GameType::renderTimeLeft()
{
   glColor3f(1,1,1);
   U32 timeLeft = mGameTimer.getCurrent();      // Time remaining in game

   U32 minsRemaining = timeLeft / (60000);
   U32 secsRemaining = (timeLeft - (minsRemaining * 60000)) / 1000;
   UserInterface::drawStringf(UserInterface::canvasWidth - UserInterface::horizMargin - 65,
      UserInterface::canvasHeight - UserInterface::vertMargin - 20, 20, "%02d:%02d", minsRemaining, secsRemaining);
}

void GameType::renderTalkingClients()
{
   S32 y = 150;
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->voiceSFX->isPlaying())
      {
         Color teamColor = mTeams[mClientList[i]->teamId].color;
         glColor(teamColor);
         UserInterface::drawString(10, y, 20, mClientList[i]->name.getString());
         y += 25;
      }
   }
}

void GameType::gameOverManGameOver()
{
   if(mGameOver)     // Only do this once
      return;

   mBetweenLevels = true;
   mGameOver = true;             // Show scores at end of game
   s2cSetGameOver(true);         // Alerts clients that the game is over
   gServerGame->gameEnded();     // Sets level-switch timer, which gives us a short delay before switching games

   onGameOver();
}


// Handle the end-of-game...  handles all games... not in any subclasses
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
      S32 winningScore = mTeams[0].score;
      for(S32 i = 1; i < mTeams.size(); i++)
      {
         if(mTeams[i].score == winningScore)
            tied = true;
         else if(mTeams[i].score > winningScore)
         {
            teamWinner = i;
            winningScore = mTeams[i].score;
            tied = false;
         }
      }
      if(!tied)
      {
         e.push_back(teamString);
         e.push_back(mTeams[teamWinner].name);
      }
   }
   else                    // Individual game -> find player with highest score
   {
      if(mClientList.size())
      {
         ClientRef *winningClient = mClientList[0];
         for(S32 i = 1; i < mClientList.size(); i++)
         {
            if(mClientList[i]->score == winningClient->score)
               tied = true;
            else if(mClientList[i]->score > winningClient->score)
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

// Runs on server, after level has been loaded from a file
void GameType::onLevelLoaded()
{
   mSpyBugs.clear();

   // Find all spybugs in the game
   gServerGame->getGridDatabase()->findObjects(SpyBugType, mSpyBugs, gServerWorldBounds);
  
   Vector<GameObject *> fillVector;
   getGame()->getGridDatabase()->findObjects(LoadoutZoneType, fillVector, gServerWorldBounds);

   mLevelHasLoadoutZone = (fillVector.size() > 0);
}


void GameType::onAddedToGame(Game *theGame)
{
   theGame->setGameType(this);
}


extern void constructBarriers(Game *theGame, const Vector<F32> &barrier, F32 width, bool solid);
extern S32 gMaxPlayers;

Team GameType::readTeamFromLevelLine(S32 argc, const char **argv)
{
   Team t;
   if(argc < 5)                     // Enough arguments?
   {
      t.numPlayers = -1;            // Signal that this is a bogus object
      return t;
   }

   t.numPlayers = 0;

   t.name.set(argv[1]);
   t.color.read(argv + 2);

   return t;
}


// Returns true if we ceated an object here, false otherwise
bool GameType::processLevelItem(S32 argc, const char **argv)
{
   if(!stricmp(argv[0], "Team"))
   {
      if(mTeams.size() >= gMaxTeams)   // Too many teams?
         return false;

      Team team = readTeamFromLevelLine(argc, argv);
      if(team.numPlayers != -1)
         mTeams.push_back(team);
   }
   else if(!strcmp(argv[0], "Script"))
   {
      mScriptArgs.clear();    // Clear out any args from a previous Script line
      for(S32 i = 1; i < argc; i++)
         mScriptArgs.push_back(string(argv[i]).c_str());    // Use string to make a const char copy of the param
   }
   else if(!stricmp(argv[0], "Spawn"))
   {
      if(argc < 4)
         return false;
      S32 teamIndex = atoi(argv[1]);
      Point p;
      p.read(argv + 2);
      p *= getGame()->getGridSize();
      if(teamIndex >= 0 && teamIndex < mTeams.size())    // Ignore if team is invalid
         mTeams[teamIndex].spawnPoints.push_back(p);
   }
   else if(!stricmp(argv[0], "FlagSpawn"))      // FlagSpawn <team> <x> <y> [timer]
   {
      if(argc < 4)
         return false;
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
   else if(!stricmp(argv[0], "BarrierMaker"))
   {
      BarrierRec barrier;
      if(argc < 2)
         return false;
      barrier.width = atof(argv[1]);
      for(S32 i = 2; i < argc; i++)
         barrier.verts.push_back(atof(argv[i]) * getGame()->getGridSize());
      if(barrier.verts.size() > 3)
      {
         barrier.solid = false;
         mBarriers.push_back(barrier);
         constructBarriers(getGame(), barrier.verts, barrier.width, barrier.solid);
      }
   }
   // TODO: Integrate code above with code above!!  EASY!!
   else if(!stricmp(argv[0], "BarrierMakerS"))
   {
      BarrierRec barrier;
      if(argc < 2)
         return false;
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
   else if(!stricmp(argv[0], "LevelName"))
   {
      string s;
      for(S32 i = 1; i < argc; i++)
      {
         s += argv[i];
         if(i < argc - 1)
            s += " ";
      }
      mLevelName.set(s.substr(0, gMaxGameNameLength).c_str());
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
      mLevelDescription.set(s.substr(0, gMaxGameDescrLength).c_str());
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
      mLevelCredits.set(s.substr(0, gMaxGameDescrLength).c_str());
   }
   else if(!stricmp(argv[0], "MinPlayers"))     // Recommend a min players for this map
   {
      if (argc > 1)
         minRecPlayers = max(min(atoi(argv[1]), gMaxPlayers), 0);
      else
         minRecPlayers = 0;
   }
   else if(!stricmp(argv[0], "MaxPlayers"))     // Recommend a max players for this map
   {
      if (argc > 1)
         maxRecPlayers = max(min(atoi(argv[1]), gMaxPlayers), 0);
      else
         maxRecPlayers = 0;
   }
   else
      return false;

   return true;
}


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
   U32 teamIndex = cl->teamId;

   Point spawnPoint = getSpawnPoint(teamIndex);

   //                       Player's name, team, and spawning location
   Ship *newShip = new Ship(cl->name, teamIndex, spawnPoint);

   newShip->addToGame(getGame());
   theClient->setControlObject(newShip);
   newShip->setOwner(theClient);

   if(isSpawnWithLoadoutGame() || !levelHasLoadoutZone())
      setClientShipLoadout(cl, theClient->getLoadout());                  // Set loadout if this is a SpawnWithLoadout type of game, or there is no loadout zone
}


// Note that we need to have spawn method here so we can override it for different game types, such as Nexus (hunters)
void GameType::spawnRobot(Robot *robot)
{
   Point spawnPoint = getSpawnPoint(robot->getTeam());

   if( !robot->initialize(spawnPoint))
   {
      robot->deleteObject();
      return;
   }



   //robot->addToGame(getGame());

   // Should probably do this, but... not now.
   //if(isSpawnWithLoadoutGame())
   //   setClientShipLoadout(cl, theClient->getLoadout());                  // Set loadout if this is a SpawnWithLoadout type of game
}


Point GameType::getSpawnPoint(S32 team)
{
   TNLAssert(mTeams[team].spawnPoints.size(), "No spawn points!");   // Basically, game dies if there are no spawn points for a team.  Don't let this happen.

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


void GameType::setClientShipLoadout(ClientRef *cl, const Vector<U32> &loadout)
{
   if(loadout.size() != ShipModuleCount + ShipWeaponCount)     // Reject improperly sized loadouts.  Currently 2 + 3
      return;

   Ship *theShip = dynamic_cast<Ship *>(cl->clientConnection->getControlObject());
   if(theShip)
      theShip->setLoadout(loadout);
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
         performProxyScopeQuery(co, (GameConnection *) connection);
         gc->objectInScope(co);     // Put controlObject in scope ==> This is where the update mask gets set to 0xFFFFFFFF
      }
   }

   static Vector<GameObject *> fillVector;

   // What does the spy bug see?
   S32 teamId = gc->getClientRef()->teamId;

   for(S32 i = 0; i < mSpyBugs.size(); i++)
   {
      SpyBug *sb = dynamic_cast<SpyBug *>(mSpyBugs[i]);
      if(!sb->isVisibleToPlayer( cr->teamId, cr->name, getGame()->getGameType()->isTeamGame() ))
         break;
      fillVector.clear();
      Point pos = sb->getActualPos();
      Point scopeRange(gSpyBugRange, gSpyBugRange);
      Rect queryRect(pos, pos);
      queryRect.expand(scopeRange);
      findObjects(AllObjectTypes, fillVector, queryRect);

      for(S32 j = 0; j < fillVector.size(); j++)
         connection->objectInScope(fillVector[j]);
   }
}


// Here is where we determine which objects are visible from player's ships.  Only runs on server.
void GameType::performProxyScopeQuery(GameObject *scopeObject, GameConnection *connection)
{
   static Vector<GameObject *> fillVector;
   fillVector.clear();

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
      S32 teamId = connection->getClientRef()->teamId;

      for(S32 i = 0; i < mClientList.size(); i++)
      {
         if(mClientList[i]->teamId != teamId)      // Wrong team
            continue;

         if(!mClientList[i]->clientConnection)     // No client
            continue;

         Ship *co = dynamic_cast<Ship *>(mClientList[i]->clientConnection->getControlObject());
         TNLAssert(co, "Null control object!");

         Point pos = co->getActualPos();
         Rect queryRect(pos, pos);
         queryRect.expand( Game::getScopeRange(co->isModuleActive(ModuleSensor)) );

         findObjects(scopeObject == co ? AllObjectTypes : CommandMapVisType, fillVector, queryRect);
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

      findObjects(AllObjectTypes, fillVector, queryRect);
   }

   // Set object-in-scope for all objects found above
   for(S32 i = 0; i < fillVector.size(); i++)
      connection->objectInScope(fillVector[i]);
}


void GameType::addItemOfInterest(Item *theItem)
{
   ItemOfInterest i;
   i.theItem = theItem;
   i.teamVisMask = 0;
   mItemsOfInterest.push_back(i);
}


// Here we'll cycle through all itemsOfInterest, then find any ships within scope range of each.  We'll then mark the object as being visible
// to those teams with ships close enough to see it, if any.  Called from idle()
void GameType::queryItemsOfInterest()
{
   static Vector<GameObject *> fillVector;

   for(S32 i = 0; i < mItemsOfInterest.size(); i++)
   {
      ItemOfInterest &ioi = mItemsOfInterest[i];
      ioi.teamVisMask = 0;                         // Reset mask, object becomes invisible to all teams
      Point pos = ioi.theItem->getActualPos();
      Point scopeRange(Game::PlayerSensorHorizVisDistance, Game::PlayerSensorVertVisDistance);
      Rect queryRect(pos, pos);

      queryRect.expand(scopeRange);
      findObjects(ShipType | RobotType, fillVector, queryRect);
      for(S32 j = 0; j < fillVector.size(); j++)
      {
         Ship *theShip = dynamic_cast<Ship *>(fillVector[j]);
         Point delta = theShip->getActualPos() - pos;
         delta.x = fabs(delta.x);
         delta.y = fabs(delta.y);

         if( (theShip->isModuleActive(ModuleSensor) && delta.x < Game::PlayerSensorHorizVisDistance && delta.y < Game::PlayerSensorVertVisDistance) ||
               (delta.x < Game::PlayerHorizVisDistance && delta.y < Game::PlayerVertVisDistance) )
            ioi.teamVisMask |= (1 << theShip->getTeam());      // Mark object as visible to theShip's team
      }
      fillVector.clear();
   }
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

// This method can be overridden by other game types that handle colors differently
Color GameType::getTeamColor(S32 team)
{
   if(team == -1)
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
         return(mClientList[i]->teamId);

   return(-1);    // If we can't find the team, let's call it neutral
}


const char *GameType::getTeamName(S32 team)
{
   if(team >= 0)
      return mTeams[team].name.getString();
   else if(team == -2)
      return "Hostile";
   else if(team == -1)
      return "Neutral";
   else
      return "UNKNOWN";
}


Color GameType::getTeamColor(GameObject *theObject)
{
   return getTeamColor(theObject->getTeam());
}

Color GameType::getShipColor(Ship *s)
{
   return getTeamColor(s->getTeam());
}


extern  F32 getCurrentRating(GameConnection *conn);


// Make sure that the mTeams[] structure has the proper player counts
// Needs to be called manually before accessing the structure
// Rating may only work on server... not tested on client
void GameType::countTeamPlayers()
{
   for(S32 i = 0; i < mTeams.size(); i++)
   {
      mTeams[i].numPlayers = 0;
      mTeams[i].rating = 0;
   }

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      mTeams[mClientList[i]->teamId].numPlayers++;

      GameConnection *cc = mClientList[i]->clientConnection;
      if(cc)
         mTeams[mClientList[i]->teamId].rating += max(getCurrentRating(cc), .1);
   }
}

// Adds a new client to the game when a player joins, or when a level cycles
// runs on the server
void GameType::serverAddClient(GameConnection *theClient)
{
   theClient->setScopeObject(this);

   ClientRef *cref = allocClientRef();
   cref->name = theClient->getClientName();

   cref->clientConnection = theClient;
   countTeamPlayers();     // Also calcs team ratings

   // Figure out which team has the fewest players...
   //S32 minTeamIndex = 0;
   //U32 minPlayers = mTeams[0].numPlayers;

   //for(S32 i = 1; i < mTeams.i(); size++)
   //{
   //   if(mTeams[i].numPlayers < minPlayers)
   //   {
   //      minTeamIndex = i;
   //      minPlayers = mTeams[i].numPlayers;
   //   }
   //}

   // Figure out which team has lowest total ratings...
   S32 minTeamIndex = 0;
   F32 minRating = mTeams[0].rating;

   for(S32 i = 1; i < mTeams.size(); i++)
   {

      if(mTeams[i].rating < minRating)
      {
         minTeamIndex = i;
         minRating = mTeams[i].rating;
      }
   }

   // ...and add new player to that team
   cref->teamId = minTeamIndex;
   mClientList.push_back(cref);
   theClient->setClientRef(cref);

   s2cAddClient(cref->name, false, cref->clientConnection->isAdmin());          // Tell other clients about the new guy, who is never us...
   s2cClientJoinedTeam(cref->name, cref->teamId);

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

   // Check for self-inflicted damate
   if(damagerOwner == victimOwner)
      return gWeapons[weaponType].canDamageSelf;

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

   StringTableEntry killerDescr = killerObject->getKillString();

   if(killerRef)     // Known killer
   {
      if(killerRef == clientRef)    // We killed ourselves -- should have gone easy with the bouncers!
         updateScore(killerRef, KillSelf);

      // Punish those who kill members of their own team.  Should do nothing with friendly fire disabled
      else if(isTeamGame() && killerRef->teamId == clientRef->teamId)   // Same team in a team game
         updateScore(killerRef, KillTeammate);

      else                                                              // Different team, or not a team game
         updateScore(killerRef, KillEnemy);

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
   if(!ship->isRobot())
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
         player->score += points;
         player->clientConnection->mScore += points;
         // (No need to broadcast score because individual scores are only displayed when Tab is held,
         // in which case scores, along with data like ping time, are streamed in)

         // Accumulate every client's total score counter
         for(S32 i = 0; i < mClientList.size(); i++)
         {
            mClientList[i]->clientConnection->mTotalScore += max(points, 0);

            if(mClientList[i]->clientConnection->mScore > newScore)
               newScore = mClientList[i]->clientConnection->mScore;
         }
      }
   }

   if(isTeamGame())
   {
      // Just in case...  completely superfluous, gratuitous check
      if(team < 0)
         return;

      S32 points = getEventScore(TeamScore, scoringEvent, data);
      if(points == 0)
         return;

      mTeams[team].score += points;

      // This is kind of a hack to emulate adding a point to every team *except* the scoring team.  The scoring team has its score
      // deducted, then the same amount is added to every team.  Assumes that points < 0.
      if(scoringEvent == ScoreGoalOwnTeam)
      {
         for(S32 i = 0; i < mTeams.size(); i++)
         {
            mTeams[i].score -= points;                // Add magnitiude of negative score to all teams
            s2cSetTeamScore(i, mTeams[i].score);      // Broadcast result
         }
      }
      else  // All other scoring events
         s2cSetTeamScore(team, mTeams[team].score);          // Broadcast new team score

      mLeadingTeamScore = S32_MIN;

      // Find the leading team...
      for(S32 i = 0; i < mTeams.size(); i++)
         if(mTeams[i].score > mLeadingTeamScore)
         {
            mLeadingTeamScore = mTeams[i].score;
            mLeadingTeam = i;
         }  // no break statement in above!

      newScore = mLeadingTeamScore;
   }

   checkForWinningScore(newScore);              // Check if score is high enough to trigger end-of-game
}


// Different signature for more common usage
void GameType::updateScore(ClientRef *client, ScoringEvent event, S32 data)
{
   if(client)
      updateScore(client, client->teamId, event, data);
   // else, no one to score...    sometimes client really does come in as null
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

// Add any additional game-specific menu items, processed below
void GameType::addClientGameMenuOptions(Vector<MenuItem> &menuOptions)
{
   if(isTeamGame() && mTeams.size() > 1 && !mBetweenLevels)
   {
      GameConnection *gc = gClientGame->getConnectionToServer();

      if(mCanSwitchTeams || (gc && gc->isAdmin()))
         menuOptions.push_back(MenuItem("SWITCH TEAMS", 1000, KEY_S, KEY_T));
      else
      {
         menuOptions.push_back(MenuItem("WAITING FOR SERVER TO ALLOW", 99998, KEY_UNKNOWN, KEY_UNKNOWN, Color(1, 0, 0)));
         menuOptions.push_back(MenuItem("YOU TO SWITCH TEAMS", 99999, KEY_UNKNOWN, KEY_UNKNOWN, Color(1, 0, 0)));
      }
   }
}


// Process any additional game-specific menu items added above
void GameType::processClientGameMenuOption(U32 index)
{
   if(index == 1000)
   {
      // If there are only two teams, just switch teams and skip the rigamarole
      if(mTeams.size() == 2)
      {
         GameType *gt = gClientGame->getGameType();
         if(!gt)
            return;

         Ship *s = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());  // Returns player's ship...
         gt->c2sChangeTeams(1 - s->getTeam());                    // If two teams, team will either be 0 or 1, so "1 - " will toggle
         UserInterface::reactivateMenu(gGameUserInterface);       // Jump back into the game (this option takes place immediately)
      }
      else
      {
         gTeamMenuUserInterface.activate();     // Show menu to let player select a new team
         gTeamMenuUserInterface.nameToChange = gNameEntryUserInterface.getText();      // TODO: Better place to get curernt player's name?  This may fail if users have same name, and system has changed it
      }
   }
   else if(index == 2000)
   {
      gPlayerMenuUserInterface.action = PlayerMenuUserInterface::ChangeTeam;
      gPlayerMenuUserInterface.activate();
      //UserInterface::reactivatePrevUI();                       // Should take us back to game options menu
   }
}

// Add any additional game-specific admin menu items, processed below
void GameType::addAdminGameMenuOptions(Vector<MenuItem> &menuOptions)
{
   if(isTeamGame() && mTeams.size() > 1)
      menuOptions.push_back(MenuItem("CHANGE A PLAYER'S TEAM", 2000, KEY_C, KEY_UNKNOWN));
}


// Broadcast info about the current level... code gets run on client, obviously
GAMETYPE_RPC_S2C(GameType, s2cSetLevelInfo, (StringTableEntry levelName, StringTableEntry levelDesc, S32 teamScoreLimit, StringTableEntry levelCreds, S32 objectCount /* , bool levelHasLoadoutZone [RELEASE 012]*/),
                                            (levelName, levelDesc, teamScoreLimit, levelCreds, objectCount/*, levelHasLoadoutZone [RELEASE 012] */))
{
   mLevelName = levelName;
   mLevelDescription = levelDesc;
   mLevelCredits = levelCreds;
   mWinningScore = teamScoreLimit;
   mObjectsExpected = objectCount;
   /*
   mLevelHasLoadoutZone = levelHasLoadoutZone;           // Need to pass this because we won't know for sure when the loadout zones will be sent, so searching for them is difficult
   [RELEASE 012]
   */

   gClientGame->mObjectsLoaded = 0;                      // Reset item counter
   gGameUserInterface.mShowProgressBar = true;           // Show progress bar
   mLevelInfoDisplayTimer.reset(LevelInfoDisplayTime);   // Start displaying the level info, now that we have it
}

GAMETYPE_RPC_C2S(GameType, c2sAddTime, (U32 time), (time))
{
   GameConnection *source = dynamic_cast<GameConnection *>(NetObject::getRPCSourceConnection());

   if(!source->isAdmin())                // Admins only, please!
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

   Ship *ship = dynamic_cast<Ship *>(source->getControlObject());    // Get the ship that's switching

   if(ship)
   {
      // Find all spybugs and mines that this player owned, and reset ownership
      Vector<GameObject *> fillVector;
      Rect worldBounds = gServerGame->computeWorldObjectExtents();
      gServerGame->getGridDatabase()->findObjects(SpyBugType | MineType, fillVector, gServerWorldBounds);

      for(S32 i = 0; i < fillVector.size(); i++)
         if((fillVector[i]->getOwner()) == ship->getOwner())
            fillVector[i]->setOwner(NULL);

      ship->kill();                 // Destroy the old ship

      cl->respawnTimer.clear();     // If we've just died, this will keep a second copy of ourselves from appearing
   }

   if(team == -1)                                        // If no team provided...
      cl->teamId = (cl->teamId + 1) % mTeams.size();     // ...find the next one...
   else                                                  // ...otherwise...
      cl->teamId = team;                                 // ...use the one provided

   s2cClientJoinedTeam(cl->name, cl->teamId);            // Announce the change
   spawnShip(source);                                    // Create a new ship
}

GAMETYPE_RPC_S2C(GameType, s2cAddClient, (StringTableEntry name, bool isMyClient, bool admin), (name, isMyClient, admin))
{
   ClientRef *cref = allocClientRef();
   cref->name = name;
   cref->teamId = 0;
   cref->isAdmin = admin;

   cref->decoder = new LPC10VoiceDecoder();
   cref->voiceSFX = new SFXObject(SFXVoice, NULL, 1, Point(), Point());

   mClientList.push_back(cref);
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined the game.", name.getString());

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
      (dynamic_cast<Ship *>(theControlObject))->kill();

   s2cRemoveClient(theClient->getClientName());
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
}

GAMETYPE_RPC_S2C(GameType, s2cAddTeam, (StringTableEntry teamName, F32 r, F32 g, F32 b), (teamName, r, g, b))
{
   Team team;
   team.name = teamName;
   team.color.r = r;
   team.color.g = g;
   team.color.b = b;
   mTeams.push_back(team);
}

GAMETYPE_RPC_S2C(GameType, s2cSetTeamScore, (RangedU32<0, GameType::gMaxTeams> teamIndex, U32 score), (teamIndex, score))
{
   mTeams[teamIndex].score = score;
}


// Server has sent us (the client) a message telling us how much longer we have in the current game
GAMETYPE_RPC_S2C(GameType, s2cSetTimeRemaining, (U32 timeLeft), (timeLeft))
{
   mGameTimer.reset(timeLeft);
}


// Announce a new player has joined the team
GAMETYPE_RPC_S2C(GameType, s2cClientJoinedTeam, (StringTableEntry name, RangedU32<0, GameType::gMaxTeams> teamIndex), (name, teamIndex))
{
   ClientRef *cl = findClientRef(name);
   cl->teamId = (S32) teamIndex;
   gGameUserInterface.displayMessage(Color(0.6f, 0.6f, 0.8f), "%s joined team %s.", name.getString(), getTeamName(teamIndex));

   // Make this client forget about any mines or spybugs he knows about... it's a bit of a kludge to do this here,
   // but this RPC only runs when a player joins the game or changes teams, so this will never hurt, and we can
   // save the overhead of sending a separate message which, while theoretically cleaner, will never be needed practically.
   Vector<GameObject *> fillVector;
   Rect worldBounds = gClientGame->computeWorldObjectExtents();
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
   if(gClientGame->getGameType()->mClientList.size() && name != gClientGame->getGameType()->mLocalClient->name.getString())    // Don't show message to self
      gGameUserInterface.displayMessage(Color(0,1,1), "%s has been granted administrator access.", name.getString());
}


// Announce a new player has permission to change levels
GAMETYPE_RPC_S2C(GameType, s2cClientBecameLevelChanger, (StringTableEntry name), (name))
{
   ClientRef *cl = findClientRef(name);
   cl->isLevelChanger = true;
   if(gClientGame->getGameType()->mClientList.size() && name != gClientGame->getGameType()->mLocalClient->name.getString())    // Don't show message to self
      gGameUserInterface.displayMessage(Color(0,1,1), "%s can now change levels.", name.getString());
}

// Runs after the server knows that the ghost is available and addressable via the getGhostIndex()
// Runs on server, obviously
void GameType::onGhostAvailable(GhostConnection *theConnection)
{
   NetObject::setRPCDestConnection(theConnection);    // Focus all RPCs on client only
   s2cSetLevelInfo(mLevelName, mLevelDescription, mWinningScore, mLevelCredits, gServerGame->mObjectsLoaded/*, mLevelHasLoadoutZone [RELEASE 012] */);


   for(S32 i = 0; i < mTeams.size(); i++)
   {
      s2cAddTeam(mTeams[i].name, mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);
      s2cSetTeamScore(i, mTeams[i].score);
   }

   // Add all the client and team information
   for(S32 i = 0; i < mClientList.size(); i++)
   {
      s2cAddClient(mClientList[i]->name, mClientList[i]->clientConnection == theConnection, mClientList[i]->clientConnection->isAdmin());
      s2cClientJoinedTeam(mClientList[i]->name, mClientList[i]->teamId);
   }

   // An empty list clears the barriers
   Vector<F32> v;
   s2cAddBarriers(v, 0, false);

   for(S32 i = 0; i < mBarriers.size(); i++)
   {
      s2cAddBarriers(mBarriers[i].verts, mBarriers[i].width, mBarriers[i].solid);
   }
   s2cSetTimeRemaining(mGameTimer.getCurrent());      // Tell client how much time left in current game
   s2cSetGameOver(mGameOver);
   s2cSyncMessagesComplete(theConnection->getGhostingSequence());

   NetObject::setRPCDestConnection(NULL);    // Set RPCs to go to all players
}


GAMETYPE_RPC_S2C(GameType, s2cSyncMessagesComplete, (U32 sequence), (sequence))
{
   // Now we know the game is ready to begin...
   mBetweenLevels = false;
   c2sSyncMessagesComplete(sequence);

   gGameUserInterface.mShowProgressBar = false;
   gGameUserInterface.mProgressBarFadeTimer.reset(1000);
}


GAMETYPE_RPC_C2S(GameType, c2sSyncMessagesComplete, (U32 sequence), (sequence))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();
   if(sequence != source->getGhostingSequence())
      return;

   cl->readyForRegularGhosts = true;
}

GAMETYPE_RPC_S2C(GameType, s2cAddBarriers, (Vector<F32> barrier, F32 width, bool solid), (barrier, width, solid))
{
   if(!barrier.size())
      getGame()->deleteObjects(BarrierType);
   else
      constructBarriers(getGame(), barrier, width, solid);
}


// Client sends chat message to/via game server
GAMETYPE_RPC_C2S(GameType, c2sSendChat, (bool global, StringPtr message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this,
      s2cDisplayChatMessage, (global, source->getClientName(), message));

   sendChatDisplayEvent(cl, global, theEvent);
}


// Sends a quick-chat message (which, due to its repeated nature can be encapsulated in a StringTableEntry item)
GAMETYPE_RPC_C2S(GameType, c2sSendChatSTE, (bool global, StringTableEntry message), (global, message))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   ClientRef *cl = source->getClientRef();

   RefPtr<NetEvent> theEvent = TNL_RPC_CONSTRUCT_NETEVENT(this,
      s2cDisplayChatMessageSTE, (global, source->getClientName(), message));

   sendChatDisplayEvent(cl, global, theEvent);
}


// Send a chat message that will be displayed in-game
// If not global, send message only to other players on team
void GameType::sendChatDisplayEvent(ClientRef *cl, bool global, NetEvent *theEvent)
{
   S32 teamId = 0;

   if(!global)
      teamId = cl->teamId;

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(global || mClientList[i]->teamId == teamId)
         if(mClientList[i]->clientConnection)
            mClientList[i]->clientConnection->postNetEvent(theEvent);
   }
}

extern Color gGlobalChatColor;
extern Color gTeamChatColor;

// Server sends message to the client for display using StringPtr
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
   Ship *s = dynamic_cast<Ship *>(source->getControlObject());
   if(s)
      s->selectWeapon();
}

// Client tells server that they chose the specified weapon
GAMETYPE_RPC_C2S(GameType, c2sSelectWeapon, (RangedU32<0, ShipWeaponCount> indx), (indx))
{
   GameConnection *source = (GameConnection *) getRPCSourceConnection();
   Ship *s = dynamic_cast<Ship *>(source->getControlObject());
   if(s)
      s->selectWeapon(indx);
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

   for(S32 i = 0; i < mClientList.size(); i++)
   {
      if(mClientList[i]->ping < MaxPing)
         mPingTimes.push_back(mClientList[i]->ping);
      else
         mPingTimes.push_back(MaxPing);

      mScores.push_back(mClientList[i]->score);

      GameConnection *conn = mClientList[i]->clientConnection;

      // Players rating = cumulative score / total score played while this player was playing, ranks from 0 to 1
      mRatings.push_back(getCurrentRating(conn) * 100.0 + 100);
   }

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
      mClientList[i]->score = scores[i];
      mClientList[i]->rating = min(max(((F32)ratings[i] - 100.0) / 100.0, minRating), maxRating);    // Ensure our rating is within the ranged limits used above
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
         if(mClientList[i]->teamId == cl->teamId && (mClientList[i] != cl || echo) && mClientList[i]->clientConnection)
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

