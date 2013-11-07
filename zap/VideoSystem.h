//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef VIDEOSYSTEM_H_
#define VIDEOSYSTEM_H_

#include "ConfigEnum.h"    // For DisplayMode enum
#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class GameSettings;

class VideoSystem
{
public:
   VideoSystem();
   virtual ~VideoSystem();

   static bool init();

   static void setWindowPosition(S32 left, S32 top);
   static S32 getWindowPositionCoord(bool getX);

   static S32 getWindowPositionX();
   static S32 getWindowPositionY();

   static void actualizeScreenMode(GameSettings *settings, bool changingInterfaces, bool currentUIUsesEditorScreenMode);
   static void getWindowParameters(GameSettings *settings, DisplayMode displayMode, 
                                   S32 &sdlWindowWidth, S32 &sdlWindowHeight, F64 &orthoLeft, F64 &orthoRight, F64 &orthoTop, F64 &orthoBottom);

};

} /* namespace Zap */
#endif /* VIDEOSYSTEM_H_ */
