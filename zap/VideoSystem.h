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
struct IniSettings;

class VideoSystem
{
public:
   // States for different video states
   enum videoSystem_st_t {
       init_st,
       windowed_st,
       fullscreen_stretched_st,
       fullscreen_unstretched_st,
       windowed_editor_st,
       fullscreen_editor_st,
   };

   // Reason a video state changes
   enum StateReason {
      StateReasonToggle,                           // Normal mode toggle
      StateReasonInterfaceChange,                  // Moving in/out of editor
      StateReasonExternalResize,                   // User resizes window
      StateReasonModeDirectWindowed,               // Direct change to windowed mode
      StateReasonModeDirectFullscreenStretched,    // Direct change to FS stretched
      StateReasonModeDirectFullscreenUnstretched,  // Direct change to FS unstretched
   };

private:
   static videoSystem_st_t currentState;

public:
   VideoSystem();
   virtual ~VideoSystem();

   static bool init();
   static void shutdown();

   static void setWindowPosition(S32 left, S32 top);
   static void saveWindowPostion(GameSettings *settings);
   static void saveUpdateWindowScale(GameSettings *settings);
   static bool isFullscreen();

   static void redrawViewport(GameSettings *settings);

   static void updateDisplayState(GameSettings *settings, StateReason reason);
   static void debugPrintState(VideoSystem::videoSystem_st_t currentState);
};

} /* namespace Zap */
#endif /* VIDEOSYSTEM_H_ */
