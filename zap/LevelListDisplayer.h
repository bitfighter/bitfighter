//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_LIST_DISPLAYER_H_
#define _LEVEL_LIST_DISPLAYER_H_

#include "Timer.h"
#include "tnlVector.h"

#include <string>

namespace Zap
{
using std::string;

class LevelListDisplayer
{
private:
   Vector<string> mLevelLoadDisplayNames;
   S32 mLevelLoadDisplayTotal;
   bool mLevelLoadDisplay;
   Timer mLevelLoadDisplayFadeTimer;

   void addProgressListItem(string item);

public:
   LevelListDisplayer();

   void idle(U32 timeDelta);
   void render() const;
   void addLevelName(const string &levelName);

   void showLevelLoadDisplay(bool show, bool fade);
   void clearLevelLoadDisplay();
};

}

#endif
