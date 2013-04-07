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

#ifndef _TIME_LEFT_RENDERER
#define _TIME_LEFT_RENDERER

#include "tnlTypes.h"

using namespace TNL;

namespace Zap 
{

class GameType;
class Game;

namespace UI
{

class TimeLeftRenderer
{
private:
   S32  renderTimeLeft           (const GameType *gameType) const;
   S32  renderHeadlineScores     (const Game *game, S32 ypos) const;
   void renderTeamScores         (const GameType *gameType, S32 bottom) const;
   void renderLeadingPlayerScores(const GameType *gameType, S32 bottom) const;

public:
   void render                   (const GameType *gameType, bool scoreboardVisible) const;
};

}
}

#endif

