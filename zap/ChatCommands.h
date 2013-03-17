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

#ifndef _CHATCMDS_H_
#define _CHATCMDS_H_

#include "UIGame.h"

namespace Zap
{

static const S32 MAX_CMDS = 9;

enum ArgTypes {
   NAME,    // Player name (can be tab-completed)
   TEAM,    // Team name (can be tab-completed)
   xINT,     // Integer argument
   STR,     // String argument
   PT,      // Point argument (only used by Lua scripts)
   ARG_TYPES
};

enum HelpCategories {
   ADV_COMMANDS,
   SOUND_COMMANDS,
   LEVEL_COMMANDS,
   ADMIN_COMMANDS,
   DEBUG_COMMANDS,
   COMMAND_CATEGORIES
};


struct CommandInfo 
{
   string cmdName;
   void (*cmdCallback)(ClientGame *game, const Vector<string> &args);
   ArgTypes cmdArgInfo[MAX_CMDS];
   S32 cmdArgCount;
   HelpCategories helpCategory;
   S32 helpGroup;
   S32 lines;                    // # lines required to display help (usually 1, occasionally 2)
   string helpArgString[MAX_CMDS];
   string helpTextString;
};

}

using namespace Zap;
namespace ChatCommands 
{

void announceHandler           (ClientGame *game, const Vector<string> &args);
void mVolHandler               (ClientGame *game, const Vector<string> &args);    
void sVolHandler               (ClientGame *game, const Vector<string> &args);    
void vVolHandler               (ClientGame *game, const Vector<string> &args);
void servVolHandler            (ClientGame *game, const Vector<string> &args);
void mNextHandler              (ClientGame *game, const Vector<string> &args);
void mPrevHandler              (ClientGame *game, const Vector<string> &args);
void getMapHandler             (ClientGame *game, const Vector<string> &args);
void nextLevelHandler          (ClientGame *game, const Vector<string> &args);
void prevLevelHandler          (ClientGame *game, const Vector<string> &args);
void restartLevelHandler       (ClientGame *game, const Vector<string> &args);
void randomLevelHandler        (ClientGame *game, const Vector<string> &args);
void shutdownServerHandler     (ClientGame *game, const Vector<string> &args);
void kickPlayerHandler         (ClientGame *game, const Vector<string> &args);
void submitPassHandler         (ClientGame *game, const Vector<string> &args);
void showCoordsHandler         (ClientGame *game, const Vector<string> &args);
void showIdsHandler            (ClientGame *game, const Vector<string> &args);
void showZonesHandler          (ClientGame *game, const Vector<string> &args);
void showPathsHandler          (ClientGame *game, const Vector<string> &args);
void pauseBotsHandler          (ClientGame *game, const Vector<string> &args);
void stepBotsHandler           (ClientGame *game, const Vector<string> &args);
void setAdminPassHandler       (ClientGame *game, const Vector<string> &args);
void setServerPassHandler      (ClientGame *game, const Vector<string> &args);
void setLevPassHandler         (ClientGame *game, const Vector<string> &args);
void setServerNameHandler      (ClientGame *game, const Vector<string> &args);
void setServerDescrHandler     (ClientGame *game, const Vector<string> &args);
void setLevelDirHandler        (ClientGame *game, const Vector<string> &args);
void pmHandler                 (ClientGame *game, const Vector<string> &args);
void muteHandler               (ClientGame *game, const Vector<string> &args);
void voiceMuteHandler          (ClientGame *game, const Vector<string> &args);
void maxFpsHandler             (ClientGame *game, const Vector<string> &args);
void lagHandler                (ClientGame *game, const Vector<string> &args);
void clearCacheHandler         (ClientGame *game, const Vector<string> &args);
void lineWidthHandler          (ClientGame *game, const Vector<string> &args);
void idleHandler               (ClientGame *game, const Vector<string> &args);
void showPresetsHandler        (ClientGame *game, const Vector<string> &args);
void deleteCurrentLevelHandler (ClientGame *game, const Vector<string> &args);
void addTimeHandler            (ClientGame *game, const Vector<string> &args);
void setTimeHandler            (ClientGame *game, const Vector<string> &args);
void setWinningScoreHandler    (ClientGame *game, const Vector<string> &args);
void resetScoreHandler         (ClientGame *game, const Vector<string> &args);
void addBotHandler             (ClientGame *game, const Vector<string> &args);
void addBotsHandler            (ClientGame *game, const Vector<string> &args);
void kickBotHandler            (ClientGame *game, const Vector<string> &args);
void kickBotsHandler           (ClientGame *game, const Vector<string> &args);
void showBotsHandler           (ClientGame *game, const Vector<string> &args);
void setMaxBotsHandler         (ClientGame *game, const Vector<string> &args);
void banPlayerHandler          (ClientGame *game, const Vector<string> &args);
void banIpHandler              (ClientGame *game, const Vector<string> &args);
void renamePlayerHandler       (ClientGame *game, const Vector<string> &args);
void globalMuteHandler         (ClientGame *game, const Vector<string> &args);
void shuffleTeams              (ClientGame *game, const Vector<string> &args);


};

#endif
