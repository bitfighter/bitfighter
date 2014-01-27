//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_LEVEL_INFO_DISPLAYER_H_
#define _UI_LEVEL_INFO_DISPLAYER_H_

#include "SlideOutWidget.h"      // Parent
#include <string>

using namespace TNL; 
using namespace std;


namespace Zap {

class GameType;
class ClientGame;

namespace UI {


class LevelInfoDisplayer : public SlideOutWidget
{
   typedef SlideOutWidget Parent;

private:
   const ClientGame *mGame;
   Timer mDisplayTimer;

   S32 getSideBoxWidth() const;

   string getGameTypeName() const;
   string getShortGameTypeName() const;

public:
   LevelInfoDisplayer(const ClientGame *game);
   virtual ~LevelInfoDisplayer();

   void onGameTypeChanged();

   void resetDisplayTimer();
   void idle(U32 timeDelta);
   void render() const;

   void clearDisplayTimer();
   virtual bool isActive() const;

   bool isDisplayTimerActive() const;
   bool isVisible() const;
};


} } // Nested namespace

#endif
