//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _DISPLAY_MANAGER_H_
#define _DISPLAY_MANAGER_H_

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

#endif /* _DISPLAY_MANAGER_H_ */
