//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SCOREBOARD_RENDERER_H_
#define _SCOREBOARD_RENDERER_H_

#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class ClientInfo;
class GameUserInterface;

class ScoreboardRenderer
{
public:
   explicit ScoreboardRenderer(GameUserInterface *ui);

   void render();

private:
   void renderTeamScoreboard(S32 index, S32 teams, bool isTeamGame,
                             S32 scoreboardTop, S32 sectionHeight, S32 teamHeaderHeight, S32 lineHeight) const;
   void renderTeamName(S32 index, S32 left, S32 right, S32 top) const;
   void renderScoreboardColumnHeaders(S32 leftEdge, S32 rightEdge, S32 y, const S32 *colIndexWidths, bool isTeamGame) const;
   void renderScoreboardLine(const Vector<ClientInfo *> &playerInfos, bool isTeamGame, S32 row,
                             S32 x, S32 y, U32 lineHeight, S32 rightEdge, S32 *colWidths) const;
   void renderBadges(ClientInfo *clientInfo, S32 x, S32 y, F32 scaleRatio) const;

   GameUserInterface *mUI;
};

}

#endif
