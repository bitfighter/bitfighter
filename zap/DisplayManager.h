//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef DISPLAYMANAGER_H_
#define DISPLAYMANAGER_H_

#include "ScreenInfo.h"

namespace Zap
{

class DisplayManager
{
private:
   static ScreenInfo *mScreenInfo;

public:
   DisplayManager();
   virtual ~DisplayManager();

   static void initialize();
   static void cleanup();

   static ScreenInfo *getScreenInfo();
};

} /* namespace Zap */

#endif /* DISPLAYMANAGER_H_ */
