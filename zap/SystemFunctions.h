//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SYSTEM_FUNCTIONS_H_
#define _SYSTEM_FUNCTIONS_H_

#include "LevelSource.h"      // For LevelSourcePtr def

#include "tnlTypes.h"
#include "tnlVector.h"

#include <boost/shared_ptr.hpp>
#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

class GameSettings;
class ServerGame;

// This is a duplicate def also found in GameSettings.h.  Need to get rid of this one!
typedef boost::shared_ptr<GameSettings> GameSettingsPtr;


extern void initHosting(GameSettingsPtr settings, LevelSourcePtr levelSource, bool testMode, bool dedicatedServer);
extern void abortHosting_noLevels(ServerGame *serverGame);
extern bool writeToConsole();
extern string getInstalledDataDir();

}

#endif
