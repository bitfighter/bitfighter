//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TIME_LEFT_RENDERER
#define _TIME_LEFT_RENDERER

#include "tnlTypes.h"
#include "Point.h"

using namespace TNL;

namespace Zap 
{

class GameType;
class Game;
class ScreenInfo;

namespace UI
{

class TimeLeftRenderer
{
private:
   ScreenInfo *mScreenInfo;

   Point renderTimeLeft      (const GameType *gameType, bool render = true) const;     // Returns width and height
   S32 renderHeadlineScores  (const Game *game, S32 ypos) const;
   S32 renderTeamScores      (const GameType *gameType, S32 bottom, bool render) const;
   S32 renderIndividualScores(const GameType *gameType, S32 bottom, bool render) const;

public:
   static const S32 TimeLeftIndicatorMargin = 7;

   TimeLeftRenderer();

   Point render(const GameType *gameType, bool scoreboardVisible, bool render) const;
};

} } // Nested namespace


#endif

