//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_LEVEL_INFO_DISPLAYER_H_
#define _UI_LEVEL_INFO_DISPLAYER_H_

#include "SlideOutWidget.h"      // Parent

using namespace TNL; 


namespace Zap {

class GameType;

namespace UI {


class LevelInfoDisplayer : public SlideOutWidget
{
   typedef SlideOutWidget Parent;

private:
   Timer mDisplayTimer;

public:
   LevelInfoDisplayer();
   virtual ~LevelInfoDisplayer();

   void resetDisplayTimer();
   void idle(U32 timeDelta);
   void render(const GameType *gameType, S32 teamCount, bool isInDatabase) const;

   void clearDisplayTimer();
   virtual bool isActive() const;

   bool isDisplayTimerActive() const;
   bool isVisible() const;
};


} } // Nested namespace

#endif
