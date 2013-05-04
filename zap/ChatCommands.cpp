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

#include "ChatCommands.h"

#include "ClientGame.h"
#include "gameType.h"
#include "LevelDatabaseDownloadThread.h"
#include "LevelSpecifierEnum.h"

#include "UIManager.h"
#include "UIGame.h"

#include "stringUtils.h"
#include "RenderUtils.h"
#include "OpenglUtils.h"


namespace Zap {
   extern S32 LOADOUT_PRESETS;
}

namespace ChatCommands
{

// Returns a pointer of string of chars, after "count" number of args
static const char *findPointerOfArg(const char *message, S32 count)
{
   S32 spacecount = 0;
   S32 cur = 0;
   char prevchar = 0;

   // Message needs to include everything including multiple spaces.  Message starts after second space.
   while(message[cur] != '\0' && spacecount != count)
   {
      if(message[cur] == ' ' && prevchar != ' ')
         spacecount++;        // Double space does not count as a seperate parameter
      prevchar = message[cur];
      cur++;
   }
   return &message[cur];
}


// static method
void addTimeHandler(ClientGame *game, const Vector<string> &words)
{
   if(words.size() < 2 || words[1] == "")
      game->displayErrorMessage("!!! Need to supply a time (in minutes)");
   else
   {
      U8 mins;    // Use U8 to limit number of mins that can be added, while nominally having no limit!
                  // Parse 2nd arg -- if first digit isn't a number, user probably screwed up.
                  // atoi will return 0, but this probably isn't what the user wanted.

      bool err = false;
      if(words[1][0] >= '0' && words[1][0] <= '9')
         mins = atoi(words[1].c_str());
      else
         err = true;

      if(err || mins == 0)
         game->displayErrorMessage("!!! Invalid value... game time not changed");
      else
      {
         if(game->getGameType())
         {
            game->displayMessage(Colors::cmdChatColor, "Extended game by %d minute%s", mins, (mins == 1) ? "" : "s");
            game->getGameType()->addTime(mins * 60 * 1000);
         }
      }
   }
}


// Set specified volume to the specefied level
static void setVolume(ClientGame *game, VolumeType volType, const Vector<string> &words)
{
   S32 vol;

   if(words.size() < 2)
   {
      game->displayErrorMessage("!!! Need to specify volume");
      return;
   }

   string volstr = words[1];

   // Parse volstr -- if first digit isn't a number, user probably screwed up.
   // atoi will return 0, but this probably isn't what the user wanted.
   if(volstr[0] >= '0' && volstr[0] <= '9')
      vol = max(min(atoi(volstr.c_str()), 10), 0);
   else
   {
      game->displayErrorMessage("!!! Invalid value... volume not changed");
      return;
   }

   IniSettings *ini = game->getSettings()->getIniSettings();

   switch(volType)
   {
      case SfxVolumeType:
         ini->sfxVolLevel = (F32) vol / 10.f;
         game->displayMessage(Colors::cmdChatColor, "SFX volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
         return;

      case MusicVolumeType:
         ini->setMusicVolLevel((F32) vol / 10.f);
         game->displayMessage(Colors::cmdChatColor, "Music volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
         return;

      case VoiceVolumeType:
      {
         F32 oldVol = ini->voiceChatVolLevel;
         ini->voiceChatVolLevel = (F32) vol / 10.f;
         game->displayMessage(Colors::cmdChatColor, "Voice chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
         if((oldVol == 0) != (vol == 0) && game->getConnectionToServer())
            game->getConnectionToServer()->s2rVoiceChatEnable(vol != 0);
         return;
      }

      case ServerAlertVolumeType:
         game->getConnectionToServer()->c2sSetServerAlertVolume((S8) vol);
         game->displayMessage(Colors::cmdChatColor, "Server alerts chat volume changed to %d %s", vol, vol == 0 ? "[MUTE]" : "");
         return;
   }
}


void sVolHandler(ClientGame *game, const Vector<string> &words)
{
   setVolume(game, SfxVolumeType, words);
}

void mVolHandler(ClientGame *game, const Vector<string> &words)
{
   setVolume(game, MusicVolumeType, words);
}

void vVolHandler(ClientGame *game, const Vector<string> &words)
{
   setVolume(game, VoiceVolumeType, words);
}

void servVolHandler(ClientGame *game, const Vector<string> &words)
{
   setVolume(game, ServerAlertVolumeType, words);
}


void mNextHandler(ClientGame *game, const Vector<string> &words)
{
   game->playNextTrack();
}


void mPrevHandler(ClientGame *game, const Vector<string> &words)
{
   game->playPrevTrack();
}


void getMapHandler(ClientGame *game, const Vector<string> &words)
{
   GameConnection *gc = game->getConnectionToServer();

   if(gc->isLocalConnection())
      game->displayErrorMessage("!!! Can't download levels from a local server");
   else
   {
      string filename;

      if(words.size() > 1 && words[1] != "")
         filename = words[1];
      else
         filename = "downloaded_" + makeFilenameFromString(game->getGameType() ?
               game->getGameType()->getLevelName()->getString() : "Level");

      // Add an extension if needed
      if(filename.find(".") == string::npos)
         filename += ".level";

      game->setRemoteLevelDownloadFilename(filename);

      gc->c2sRequestCurrentLevel();
   }
}


void nextLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(NEXT_LEVEL, false);
}


void prevLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(PREVIOUS_LEVEL, false);
}


void restartLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(REPLAY_LEVEL, false);
}

void randomLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! You don't have permission to change levels"))
      game->getConnectionToServer()->c2sRequestLevelChange(RANDOM_LEVEL, false);
}


void shutdownServerHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasOwner("!!! You don't have permission to shut the server down"))
   {
      U16 time = 0;
      bool timefound = true;
      string reason;

      if(words.size() > 1)
         time = (U16) atoi(words[1].c_str());
      if(time <= 0)
      {
         time = 10;
         timefound = false;
      }

      S32 first = timefound ? 2 : 1;
      for(S32 i = first; i < words.size(); i++)
      {
         if(i != first)
            reason = reason + " ";
         reason = reason + words[i];
      }

      game->getConnectionToServer()->c2sRequestShutdown(time, reason.c_str());
   }
}


void kickPlayerHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to kick players"))
   {
      if(words.size() < 2 || words[1] == "")
         game->displayErrorMessage("!!! Need to specify who to kick");
      else
      {
         // Did user provide a valid, known name?
         string name = words[1];
         
         if(!game->checkName(name))
         {
            game->displayErrorMessage("!!! Could not find player: %s", words[1].c_str());
            return;
         }

         if(game->getGameType())
            game->getGameType()->c2sKickPlayer(words[1].c_str());
      }
   }
}


void submitPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(words.size() < 2)
      return;

   GameConnection *conn = game->getConnectionToServer();
   conn->submitPassword(words[1].c_str());
}


/////
// Debugging command handlers


static bool isLocalTestServer(ClientGame *game, const char *failureMessage)
{
   if(Game::isLocalTestServer())
      return true;
   
   game->displayErrorMessage(failureMessage);
   return false;
}


// Can work on any server, confers no advantage
void showCoordsHandler(ClientGame *game, const Vector<string> &words)
{
   game->getUIManager()->getGameUserInterface()->toggleShowingShipCoords();
}


// Also lets players see invisible objects; uses illegal reachover and only works with local server
void showIdsHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Ids can only be displayed on a test server"))
      game->getUIManager()->getGameUserInterface()->toggleShowingObjectIds();
}


// Could possibly reveal out-of-scope turrets and forcefields and such; in any case uses illegal reachover and only works with local server
void showZonesHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Zones can only be displayed on a local host"))
      game->getUIManager()->getGameUserInterface()->toggleShowingMeshZones();
}


// Will work on any server, but offers advantage of being able to see out-of-scope bots; increases network traffic somewhat
void showBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Robots can only be displayed on a test server"))
   {
      if(game->getGameType())
         game->getGameType()->c2sShowBots();
   }
}


void showPathsHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Robots can only be shown on a test server")) 
      game->getUIManager()->getGameUserInterface()->toggleShowDebugBots();
}


// Will only work on local server; may confer some advantage, use is apparent to all players when bots are frozen
void pauseBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Robots can only be frozen on a test server")) 
      EventManager::get()->togglePauseStatus();
}


// Will only work on local server; may confer some advantage, use is apparent to all players when bots are frozen
void stepBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(isLocalTestServer(game, "!!! Robots can only be stepped on a test server")) 
   {
      S32 steps = words.size() > 1 ? atoi(words[1].c_str()) : 1;
      EventManager::get()->addSteps(steps);
   }
}

// End debugging command handlers
/////


void setOwnerPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasOwner("!!! You don't have permission to set the owner password"))
      game->changePassword(GameConnection::OwnerPassword, words, true);
}


void setAdminPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasOwner("!!! You don't have permission to set the admin password"))
      game->changePassword(GameConnection::AdminPassword, words, true);
}


void setServerPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server password"))
      game->changePassword(GameConnection::ServerPassword, words, false);
}


void setLevPassHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the level change password"))
      game->changePassword(GameConnection::LevelChangePassword, words, false);
}


void setServerNameHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server name"))
      game->changeServerParam(GameConnection::ServerName, words);
}


void setServerDescrHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the server description"))
      game->changeServerParam(GameConnection::ServerDescr, words);
}


void setLevelDirHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to set the leveldir param"))
      game->changeServerParam(GameConnection::LevelDir, words);
}


void deleteCurrentLevelHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permission to delete the current level"))
      game->changeServerParam(GameConnection::DeleteLevel, words);    // handles deletes too
}


void idleHandler(ClientGame *game, const Vector<string> &words)
{
   // No sense entering idle if you're already delayed in some capactiy... just sit tight!
   if(game->isSpawnDelayed())
      return;

   game->requestSpawnDelayed();
}


void showPresetsHandler(ClientGame *game, const Vector<string> &words)
{
   for(S32 i = 0; i < LOADOUT_PRESETS; i++)
   {
      string loadoutStr = game->getSettings()->getLoadoutPreset(i).toString();
      string display;
      
      if(loadoutStr != "")
         display = "Preset " + itos(i + 1) + ": " + replaceString(loadoutStr, ",", "; ");
      else
         display = "Preset " + itos(i + 1) + " is undefined";

      game->displayMessage(Colors::cyan, display.c_str());
   }
}


void lineWidthHandler(ClientGame *game, const Vector<string> &words)
{
   F32 linewidth;
   if(words.size() < 2 || words[1] == "")
      game->displayErrorMessage("!!! Need to supply line width");
   else
   {
      linewidth = (F32)atof(words[1].c_str());
      if(linewidth < 0.125f)
         linewidth = 0.125f;

      gDefaultLineWidth = linewidth;
      gLineWidth1 = linewidth * 0.5f;
      gLineWidth3 = linewidth * 1.5f;
      gLineWidth4 = linewidth * 2;

      glLineWidth(gDefaultLineWidth);    //make this change happen instantly
   }
}


void maxFpsHandler(ClientGame *game, const Vector<string> &words)
{
   S32 number = words.size() > 1 ? atoi(words[1].c_str()) : 0;

   if(number < 1)                              // Don't allow zero or negative numbers
      game->displayErrorMessage("!!! Usage: /maxfps <frame rate>, default = 100");
   else
      game->getSettings()->getIniSettings()->maxFPS = number;
}


void lagHandler(ClientGame *game, const Vector<string> &words)
{
   U32 sendLag  = words.size() > 1 ? atoi(words[1].c_str()) : 0;
   F32 sendLoss = words.size() > 2 ? atof(words[2].c_str()) : 0;

   U32 receiveLag;
   F32 receiveLoss = sendLoss;

   static const U32 MaxLag = 5000;

   if(sendLag > MaxLag)
   {
      game->displayErrorMessage("!!! Send lag too high or invalid");
      return;
   }
   if(sendLoss < 0 || sendLoss > 100)         // Percent range
   {
      game->displayErrorMessage("!!! Send packet loss must be between 0 and 100 percent");
      return;
   }

   if(words.size() > 3)
   {
      receiveLag = atoi(words[3].c_str());
      if(words.size() > 4)
         receiveLoss = atof(words[4].c_str());
      if(receiveLoss < 0 || receiveLoss > 100)         // Percent range
      {
         game->displayErrorMessage("!!! Receive packet loss must be between 0 and 100 percent");
         return;
      }
      if(receiveLag > MaxLag)
      {
         game->displayErrorMessage("!!! Receive lag too high or invalid");
         return;
      }
   }
   else
   {
      receiveLag = (sendLag + 1) / 2;
      sendLag /= 2;
   }

   game->getConnectionToServer()->setSimulatedNetParams(sendLoss / 100, sendLag, receiveLoss / 100, receiveLag);
}


void clearCacheHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permissions"))

   if(game->getGameType())
         game->getGameType()->c2sClearScriptCache();
}


void pmHandler(ClientGame *game, const Vector<string> &words)
{
   if(words.size() < 3)
      game->displayErrorMessage("!!! Usage: /pm <player name> <message>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());
      else
      {
         S32 argCount = 2 + countCharInString(words[1], ' ');  // Set pointer after 2 args + number of spaces in player name
         const char *message = game->getUIManager()->getGameUserInterface()->getChatMessage();   // Get the original line
         message = findPointerOfArg(message, argCount);        // Get the rest of the message

         GameType *gt = game->getGameType();

         if(gt)
            gt->c2sSendChatPM(words[1], message);
      }
   }
}


void muteHandler(ClientGame *game, const Vector<string> &words)
{
   if(words.size() < 2)
      game->displayErrorMessage("!!! Usage: /mute <player name>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());

      // Un-mute if already on the list
      else if(game->isOnMuteList(words[1]))
      {
         game->removeFromMuteList(words[1]);
         game->displaySuccessMessage("Player %s has been unmuted", words[1].c_str());
      }

      // Mute!
      else
      {
         game->addToMuteList(words[1]);
         game->displaySuccessMessage("Player %s has been muted", words[1].c_str());
      }
   }
}


void voiceMuteHandler(ClientGame *game, const Vector<string> &words)
{
   if(words.size() < 2)
      game->displayErrorMessage("!!! Usage: /vmute <player name>");
   else
   {
      if(!game->checkName(words[1]))
         game->displayErrorMessage("!!! Unknown name: %s", words[1].c_str());

      // Un-mute if already on the list
      else if(game->isOnVoiceMuteList(words[1]))
      {
         game->removeFromVoiceMuteList(words[1]);
         game->displaySuccessMessage("Voice for %s has been unmuted", words[1].c_str());
      }

      // Mute!
      else
      {
         game->addToVoiceMuteList(words[1]);
         game->displaySuccessMessage("Voice for %s has been muted", words[1].c_str());
      }
   }
}


void setTimeHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter time in minutes");
         return;
      }

      S32 timeMillis = S32(60 * 1000 * atof(words[1].c_str()));      // Reminder: We'll have problems if game time rises above S32_MAX

      if(timeMillis < 0 || (timeMillis == 0 && stricmp(words[1].c_str(), "0") && stricmp(words[1].c_str(), "unlim")) )  // 0 --> unlimited
      {
         game->displayErrorMessage("!!! Invalid value... game time not changed");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sSetTime(timeMillis);
   }
}


void setWinningScoreHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter winning score limit");
         return;
      }

      S32 score = atoi(words[1].c_str());

      if(score <= 0)    // i.e. score is invalid
      {
         game->displayErrorMessage("!!! Invalid score... winning score not changed");
         return;
      }

      if(game->getGameType())
      {
         if(game->getGameType()->getGameTypeId() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sSetWinningScore(score);
      }
   }
}


void resetScoreHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permission"))
   {
      if(game->getGameType())
      {
         if(game->getGameType()->getGameTypeId() == CoreGame)
            game->displayErrorMessage("!!! Cannot change score in Core game type");
         else
            game->getGameType()->c2sResetScore();
      }
   }
}


static bool fixupArgs(ClientGame *game, Vector<StringTableEntry> &args)
{
   // c2sAddBot expects the args is a slightly different order than what we have; it wants team first, then bot name, then bot args
   // However, we want users to be able to enter the bot name first, followed by an optional team.
   // If the user specifies a team name as the 2nd arg, translate that into a team number.
   // If the first arg is a string and there is a second arg, switch them.  If first arg is a string, and there is no second arg,
   // insert the NO_TEAM arg.  If first arg is numeric, hope user entered a team index first, and do not switch the args.
   // Normal arg order is bot name, team, bot args

   // First thing is to try to translate the 2nd arg into a team number
   if(args.size() >= 2 && !isInteger(args[1].getString()))
   {
      S32 teamIndex = game->getTeamIndexFromTeamName(args[1].getString());
      if(teamIndex == NO_TEAM)
      {
         game->displayErrorMessage("!!! Invalid team specified");
         return false;
      }

      args[1] = itos(teamIndex);
   }

   bool firstArgIsInt  = args.size() >= 1 && isInteger(args[0].getString());
   
   if(firstArgIsInt)          // If first arg is numeric, hope user entered a team index first, and do not switch the args.
      return true;

   if(args.size() >= 2)       // If the first arg is a string and there is a second arg, switch them.
   {
      StringTableEntry temp = args[0];
      args[0] = args[1];
      args[1] = temp;
   }
   else if(args.size() == 1)  // If first arg is a string, and there is no second arg, insert the NO_TEAM arg.
   {
      args.push_back(args[0]);
      args[0] = itos(NO_TEAM).c_str();
   }

   return true;
}


void announceHandler(ClientGame *game, const Vector<string> &words)
{
	if(game->hasAdmin("!!! You need to be an admin to use /announce"))
   {
	   // Rebuild our announcement from the split up vector
		string message = "";
		for(S32 i = 1; i < words.size(); i++)
		{
		   if(i != 1)
		      message = message + " ";

		   message = message + words[i];
		}
	
		GameType* gt = game->getGameType();
					
		if(gt)
			gt->c2sSendAnnouncement(message);
	}
}


void addBotHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to add a bot"))
   {
      // Build args by skipping first word (the command)
      Vector<StringTableEntry> args;
      for(S32 i = 1; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(!fixupArgs(game, args))    // Reorder args for c2sAddBot, translate team names to indices, and do a little checking
         return;     

      if(game->getGameType())
         game->getGameType()->c2sAddBot(args);
   }
}


void addBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to add a bots"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Specify number of bots to add");
         return;
      }

      S32 count = 0;
      count = atoi(words[1].c_str());

      if(count <= 0 || count > 1000)
      {
         game->displayErrorMessage("!!! Invalid number of bots to add");
         return;
      }

      // Build args by skipping first two words (the command, and count)
      Vector<StringTableEntry> args;
      for(S32 i = 2; i < words.size(); i++)
         args.push_back(StringTableEntry(words[i]));

      if(!fixupArgs(game, args))        // Reorder args for c2sAddBot translate team names to indices
         return;

      if(game->getGameType())
         game->getGameType()->c2sAddBots(count, args);
   }
}


void kickBotHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to kick a bot"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBot();
   }
}


void kickBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasLevelChange("!!! Need level change permissions to kick all bots"))
   {
      if(game->getGameType())
         game->getGameType()->c2sKickBots();
   }
}


void setMaxBotsHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter maximum number of bots");
         return;
      }

      S32 count = 0;
      count = atoi(words[1].c_str());

      if(count <= 0)
      {
         game->displayErrorMessage("!!! Invalid number of bots");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sSetMaxBots(count);
   }
}


void shuffleTeams(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! You don't have permissions to shuffle the teams"))
   {
      if(game->getTeamCount() < 2)
      {
         game->displayErrorMessage("!!! Two or more teams required to shuffle");
         return;
      }

      game->getUIManager()->getGameUserInterface()->activateHelper(HelperMenu::ShuffleTeamsHelperType, true);
   }
}


void banPlayerHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! /ban <player name> [duration in minutes]");
         return;
      }

      ClientInfo *bannedClientInfo = game->findClientInfo(words[1].c_str());

      if(!bannedClientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(bannedClientInfo->isRobot())
      {
         game->displayErrorMessage("!!! Cannot ban robots, you silly fool!");
         return;
      }

      if(bannedClientInfo->isAdmin())
      {
         if(!game->hasOwner("!!! Cannot ban an admin"))
            return;
      }

      S32 banDuration = 0;
      if (words.size() > 2)
         banDuration = atoi(words[2].c_str());

      if(game->getGameType())
         game->getGameType()->c2sBanPlayer(words[1].c_str(), banDuration);
   }
}


void banIpHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! /banip <player name> [duration in minutes]");
         return;
      }

      Address ipAddress(words[1].c_str());

      if(!ipAddress.isValid())
      {
         game->displayErrorMessage("!!! Invalid IP address to ban");
         return;
      }

      S32 banDuration = 0;
      if (words.size() > 2)
         banDuration = atoi(words[2].c_str());

      if(game->getGameType())
         game->getGameType()->c2sBanIp(words[1].c_str(), banDuration);
   }
}


void renamePlayerHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 3)
      {
         game->displayErrorMessage("!!! /rename <from name> <to name>");
         return;
      }

      ClientInfo *clientInfo = game->findClientInfo(words[1].c_str());

      if(!clientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(clientInfo->isAuthenticated())
      {
         game->displayErrorMessage("!!! Cannot rename authenticated players");
         return;
      }

      if(game->getGameType())
         game->getGameType()->c2sRenamePlayer(words[1].c_str(), words[2].c_str());
   }
}


void globalMuteHandler(ClientGame *game, const Vector<string> &words)
{
   if(game->hasAdmin("!!! Need admin permission"))
   {
      if(words.size() < 2)
      {
         game->displayErrorMessage("!!! Enter player name");
         return;
      }

      ClientInfo *clientInfo = game->findClientInfo(words[1].c_str());

      if(!clientInfo)
      {
         game->displayErrorMessage("!!! Player name not found");
         return;
      }

      if(clientInfo->isAdmin())
      {
         if(!game->hasOwner("!!! Cannot mute an admin"))
            return;
      }

      if(game->getGameType())
         game->getGameType()->c2sGlobalMutePlayer(words[1].c_str());
   }
}

void downloadMapHandler(ClientGame *game, const Vector<string> &args)
{
   // the download thread deletes itself, so there is no memory leak
   static Thread* downloadThread;
   if(args.size() < 2)
   {
      game->displayErrorMessage("You must specify a level");
      return;
   }

   downloadThread = new LevelDatabaseDownloadThread(args[1], game);
   downloadThread->start();
}


};
