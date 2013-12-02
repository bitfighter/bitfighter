//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "DisplayManager.h"

namespace Zap
{

// Declare statics
ScreenInfo *DisplayManager::mScreenInfo = NULL;

// Constructor
DisplayManager::DisplayManager()
{
   // Do nothing
}

// Destructor
DisplayManager::~DisplayManager()
{
   // Do nothing
}

void DisplayManager::initialize()
{
   mScreenInfo = new ScreenInfo();
}

void DisplayManager::cleanup()
{
   delete mScreenInfo;
   mScreenInfo = NULL;
}

ScreenInfo *DisplayManager::getScreenInfo()
{
   return mScreenInfo;
}

} /* namespace Zap */
