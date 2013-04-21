//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "VideoSystem.h"
#include "ClientGame.h"
#include "GameSettings.h"
#include "IniFile.h"
#include "Console.h"
#include "ScreenInfo.h"
#include "UI.h"
#include "version.h"
#include "OpenglUtils.h"

#include "tnlLog.h"

#include "SDL.h"

#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
   // Needed for GLES compatibility
#  define glOrtho glOrthof
#else
#  include "SDL_opengl.h"
#endif

#if !SDL_VERSION_ATLEAST(2,0,0)
#  include "SDL_syswm.h"
#endif

#include <cmath>

#if defined(TNL_OS_LINUX) && !SDL_VERSION_ATLEAST(2,0,0)
#  include <X11/Xlib.h>
#endif

namespace Zap
{

VideoSystem::VideoSystem()
{
   // Do nothing

}

VideoSystem::~VideoSystem()
{
   // Do nothing
}


extern void shutdownBitfighter();
extern string getInstalledDataDir();

static string WINDOW_TITLE = "Bitfighter " + string(ZAP_GAME_RELEASE);

void VideoSystem::init()
{
   // Make sure "SDL_Init(0)" was done before calling this function

   // First, initialize SDL's video subsystem
   if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
   {
      // Failed, exit
      logprintf(LogConsumer::LogFatalError, "SDL Video initialization failed: %s", SDL_GetError());
      shutdownBitfighter();
   }


   // Now, we want to setup our requested
   // window attributes for our OpenGL window.
   // Note on SDL_GL_RED/GREEN/BLUE/ALPHA_SIZE: On windows, it is better to not set them at all, or risk going extremely slow software rendering including if your desktop graphics set to 16 bit color.
   //SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
   //SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
   //SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
   //SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );  // depth used in editor to display spybug visible area non-overlap
   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );


   // Get information about the current desktop video settings and initialize
   // our ScreenInfo class with with current width and height

#if SDL_VERSION_ATLEAST(2,0,0)

   SDL_DisplayMode mode;

   SDL_GetCurrentDisplayMode(0, &mode);  // We only have one display..  for now

   gScreenInfo.init(mode.w, mode.h);

   S32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
   // Fake fullscreen might not be needed with SDL2 - I think it does the fast switching
   // on platforms that support it
   //   if(gClientGame->getSettings()->getIniSettings()->useFakeFullscreen)  <== don't use gClientGame here, please!
   //      flags |= SDL_WINDOW_BORDERLESS;

#ifdef TNL_OS_MOBILE
   // Force fullscreen on mobile
   flags |= SDL_WINDOW_FULLSCREEN;

#  ifdef TNL_OS_IOS
   // This hint should be only for phones, tablets could rotate freely
   SDL_SetHint("SDL_IOS_ORIENTATIONS","LandscapeLeft LandscapeRight");
#  endif
#endif

   // SDL 2.0 lets us create the window first, only once
   gScreenInfo.sdlWindow = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
         gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight(), flags);

   if (!gScreenInfo.sdlWindow)
   {
      logprintf(LogConsumer::LogFatalError, "SDL window creation failed: %s", SDL_GetError());
      shutdownBitfighter();
   }

   // Create our OpenGL context; save it in case we ever need it
   SDL_GLContext context = SDL_GL_CreateContext(gScreenInfo.sdlWindow);
   gScreenInfo.sdlGlContext = &context;

   // We can set up vsync, too
   SDL_GL_SetSwapInterval(1);

#else

   const SDL_VideoInfo* info = SDL_GetVideoInfo();

   gScreenInfo.init(info->current_w, info->current_h);

#endif


   // Set the window icon -- note that the icon must be a 32x32 bmp, and SDL will
   // downscale it to 16x16 with no interpolation.  Therefore, it's best to start
   // with a finely crafted 16x16 icon, then scale it up to 32x32 with no interpolation.
   // It will look crappy at 32x32, but good at 16x16, and that's all that really matters.

   // Save bmp as a 32 bit XRGB bmp file (Gimp can do it!)
   string iconPath = getInstalledDataDir() + getFileSeparator() + "bficon.bmp";
   SDL_Surface *icon = SDL_LoadBMP(iconPath.c_str());
#if SDL_VERSION_ATLEAST(2,0,0)
   if(icon != NULL)
   {
      // flag must be non-zero to enable color key
      SDL_SetColorKey(icon, 1, SDL_MapRGB(icon->format, 0, 0, 0));
      SDL_SetWindowIcon(gScreenInfo.sdlWindow, icon);
   }
#else
   // Set window and icon title here so window will be created with proper name later
   SDL_WM_SetCaption(WINDOW_TITLE.c_str(), WINDOW_TITLE.c_str());

   if(icon != NULL)
   {
      SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 0, 0, 0));
      SDL_WM_SetIcon(icon,NULL);
   }
#endif

   if(icon != NULL)
      SDL_FreeSurface(icon);

   // We will set the resolution, position, and flags in actualizeScreenMode()
}


#if !SDL_VERSION_ATLEAST(2,0,0)
static SDL_SysWMinfo windowManagerInfo;
#endif

void VideoSystem::setWindowPosition(S32 left, S32 top)
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_SetWindowPosition(gScreenInfo.sdlWindow, left, top);
#else /* SDL_VERSION_ATLEAST(2,0,0) */

   // SDL 1.2 requires a lot more rigmarole to get and set window positions

#ifdef TNL_OS_MAC_OSX
   // We cannot set window position on Mac with SDL 1.2.  Abort!
   return;
#endif

   SDL_VERSION(&windowManagerInfo.version);

   // No window information..  Abort!!
   if(!SDL_GetWMInfo(&windowManagerInfo))
   {
      logprintf(LogConsumer::LogError, "Failed to set window position");
      return;
   }

#ifdef TNL_OS_LINUX
   if (windowManagerInfo.subsystem == SDL_SYSWM_X11) {
       windowManagerInfo.info.x11.lock_func();
       XMoveWindow(windowManagerInfo.info.x11.display, windowManagerInfo.info.x11.wmwindow, left, top);
       windowManagerInfo.info.x11.unlock_func();
   }
#endif

#ifdef TNL_OS_WIN32
   SetWindowPos(windowManagerInfo.window, HWND_TOP, left, top, 0, 0, SWP_NOSIZE);
#endif

#endif /* SDL_VERSION_ATLEAST(2,0,0) */
}


S32 VideoSystem::getWindowPositionCoord(bool getX)
{
#if SDL_VERSION_ATLEAST(2,0,0)

   S32 x, y;
   SDL_GetWindowPosition(gScreenInfo.sdlWindow, &x, &y);

   return getX ? x : y;

#else /* SDL_VERSION_ATLEAST(2,0,0) */

   SDL_VERSION(&windowManagerInfo.version);

   if(!SDL_GetWMInfo(&windowManagerInfo))
      logprintf(LogConsumer::LogError, "Failed to set window position");

#ifdef TNL_OS_LINUX
   if (windowManagerInfo.subsystem == SDL_SYSWM_X11)
   {
      XWindowAttributes xAttributes;
      Window parent, childIgnore, rootIgnore;
      Window *childrenIgnore;
      U32 childrenCountIgnore;
      S32 x, y;

      windowManagerInfo.info.x11.lock_func();

      // Find parent window of managed window to get proper coordinates
      XQueryTree(windowManagerInfo.info.x11.display, windowManagerInfo.info.x11.wmwindow,
              &rootIgnore, &parent, &childrenIgnore, &childrenCountIgnore);

      // Free any children returned
      if (childrenIgnore)
         XFree(childrenIgnore);

      // Get Window attributes
      XGetWindowAttributes(windowManagerInfo.info.x11.display, parent, &xAttributes);

      // Now find absolute values
      XTranslateCoordinates(windowManagerInfo.info.x11.display, parent,
            xAttributes.root, 0, 0, &x, &y, &childIgnore);

      windowManagerInfo.info.x11.unlock_func();
      return getX ? x : y;
   }
#endif

#ifdef TNL_OS_WIN32
   RECT rect;
   GetWindowRect(windowManagerInfo.window, &rect);
   return getX ? rect.left : rect.top;
#endif

   // Otherwise just return 0
   return 0;

#endif /* SDL_VERSION_ATLEAST(2,0,0) */
}


S32 VideoSystem::getWindowPositionX()
{
   return getWindowPositionCoord(true);
}


S32 VideoSystem::getWindowPositionY()
{
   return getWindowPositionCoord(false);
}


extern void setDefaultBlendFunction();
extern Vector<ClientGame *> gClientGames;

// Actually put us in windowed or full screen mode.  Pass true the first time this is used, false subsequently.
// This has the unfortunate side-effect of triggering a mouse move event.
void VideoSystem::actualizeScreenMode(GameSettings *settings, bool changingInterfaces, bool currentUIUsesEditorScreenMode)
{
   DisplayMode displayMode = settings->getIniSettings()->displayMode;

   gScreenInfo.resetGameCanvasSize();     // Set GameCanvasSize vars back to their default values
   gScreenInfo.setActualized();


   // If old display mode is windowed or current is windowed but we change interfaces,
   // save the window position
   if(settings->getIniSettings()->oldDisplayMode == DISPLAY_MODE_WINDOWED ||
         (changingInterfaces && displayMode == DISPLAY_MODE_WINDOWED))
   {
      settings->getIniSettings()->winXPos = getWindowPositionX();
      settings->getIniSettings()->winYPos = getWindowPositionY();

      gINI.SetValueI("Settings", "WindowXPos", settings->getIniSettings()->winXPos, true);
      gINI.SetValueI("Settings", "WindowYPos", settings->getIniSettings()->winYPos, true);
   }

   // When we're in the editor, let's take advantage of the entire screen unstretched
   // We might want to disallow this when we're in split screen mode?
   if(currentUIUsesEditorScreenMode &&
         (displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED))
   {
      // Smaller values give bigger magnification; makes small things easier to see on full screen
      F32 magFactor = 0.85f;

      // For screens smaller than normal, we need to readjust magFactor to make sure we get the full canvas height crammed onto
      // the screen; otherwise our dock will break.  Since this mode is only used in the editor, we don't really care about
      // screen width; tall skinny screens will work just fine.
      magFactor = max(magFactor, (F32)gScreenInfo.getGameCanvasHeight() / (F32)gScreenInfo.getPhysicalScreenHeight());

      gScreenInfo.setGameCanvasSize(S32(gScreenInfo.getPhysicalScreenWidth() * magFactor), S32(gScreenInfo.getPhysicalScreenHeight() * magFactor));

      displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   }


   // Set up video/window flags amd parameters and get ready to change the window
   S32 sdlWindowWidth, sdlWindowHeight;
   F64 orthoLeft, orthoRight, orthoTop, orthoBottom;

   getWindowParameters(settings, displayMode, sdlWindowWidth, sdlWindowHeight, orthoLeft, orthoRight, orthoTop, orthoBottom);


#if SDL_VERSION_ATLEAST(2,0,0)
   // Change video modes based on selected display mode
   // Note:  going into fullscreen you have to do in order:
   //  - SDL_SetWindowSize()
   //  - SDL_SetWindowFullscreen()
   //
   // However, coming out of fullscreen mode you must do the reverse
   switch (displayMode)
   {
   case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
      SDL_SetWindowSize(gScreenInfo.sdlWindow, sdlWindowWidth, sdlWindowHeight);
      SDL_SetWindowFullscreen(gScreenInfo.sdlWindow, SDL_TRUE);

      break;

   case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
      SDL_SetWindowSize(gScreenInfo.sdlWindow, sdlWindowWidth, sdlWindowHeight);
      SDL_SetWindowFullscreen(gScreenInfo.sdlWindow, SDL_TRUE);

      break;

   case DISPLAY_MODE_WINDOWED:
   default:
      // Reverse order, leave fullscreen before setting size
      SDL_SetWindowFullscreen(gScreenInfo.sdlWindow, SDL_FALSE);
      SDL_SetWindowSize(gScreenInfo.sdlWindow, sdlWindowWidth, sdlWindowHeight);
      break;
   }

#else
   // Set up sdl video flags according to display mode
   S32 sdlVideoFlags = SDL_OPENGL;

   switch (displayMode)
   {
      case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
         sdlVideoFlags |= settings->getIniSettings()->useFakeFullscreen ? SDL_NOFRAME : SDL_FULLSCREEN;
         break;

      case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
         sdlVideoFlags |= settings->getIniSettings()->useFakeFullscreen ? SDL_NOFRAME : SDL_FULLSCREEN;
         break;

      case DISPLAY_MODE_WINDOWED:
      default:  //  DISPLAY_MODE_WINDOWED
         sdlVideoFlags |= SDL_RESIZABLE;
         break;
   }

   // Finally change the video mode
   if(SDL_SetVideoMode(sdlWindowWidth, sdlWindowHeight, 0, sdlVideoFlags) == NULL)
      logprintf(LogConsumer::LogFatalError, "Setting display mode failed: %s", SDL_GetError());
#endif


   // Now save the new window dimensions in ScreenInfo
   gScreenInfo.setWindowSize(sdlWindowWidth, sdlWindowHeight);

   glClearColor( 0, 0, 0, 0 );

   glViewport(0, 0, sdlWindowWidth, sdlWindowHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   // The best understanding I can get for glOrtho is that these are the coordinates you want to appear at the four corners of the
   // physical screen. If you want a "black border" down one side of the screen, you need to make left negative, so that 0 would
   // appear some distance in from the left edge of the physical screen.  The same applies to the other coordinates as well.
   glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, 0, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Do the scissoring
   if(displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
   {
      glScissor(gScreenInfo.getHorizPhysicalMargin(),    // x
                gScreenInfo.getVertPhysicalMargin(),     // y
                gScreenInfo.getDrawAreaWidth(),          // width
                gScreenInfo.getDrawAreaHeight());        // height
   }
   else
   {
      // Enabling scissor appears to fix crashing problem switching screen mode
      // in linux and "Mobile 945GME Express Integrated Graphics Controller",
      // probably due to lines and points was not being clipped,
      // causing some lines to wrap around the screen, or by writing other
      // parts of RAM that can crash Bitfighter, graphics driver, or the entire computer.
      // This is probably a bug in the Linux Intel graphics driver.
      glScissor(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());
   }

   glEnable(GL_SCISSOR_TEST);    // Turn on clipping

   setDefaultBlendFunction();
   glLineWidth(gDefaultLineWidth);

   // Enable Line smoothing everywhere!  Make sure to disable temporarily for filled polygons and such
   glEnable(GL_LINE_SMOOTH);
   //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glEnable(GL_BLEND);

   gConsole.onScreenModeChanged();

   // Now set the window position
   if(displayMode == DISPLAY_MODE_WINDOWED)
   {
      if(settings->getIniSettings()->winXPos != 0 || settings->getIniSettings()->winYPos != 0)  // sometimes it happens to be (0,0) hiding the top title bar preventing ability to move the window, in this case we are not moving it unless it is not (0,0). Note that ini config file will default to (0,0).
         setWindowPosition(settings->getIniSettings()->winXPos, settings->getIniSettings()->winYPos);
   }
   else
      setWindowPosition(0, 0);

   // Notify all active UIs that the screen has changed mode.  This will likely need some work to not do something
   // horrible in split-screen mode.
   for(S32 i = 0; i < gClientGames.size(); i++)
      if(gClientGames[i]->getUIManager()->getCurrentUI())
         gClientGames[i]->getUIManager()->getCurrentUI()->onDisplayModeChange();

   // Re-initialize our fonts because OpenGL textures can be lost upon screen change
   FontManager::cleanup();
   FontManager::initialize(settings);
}



void VideoSystem::getWindowParameters(GameSettings *settings, DisplayMode displayMode, 
                                      S32 &sdlWindowWidth, S32 &sdlWindowHeight, F64 &orthoLeft, F64 &orthoRight, F64 &orthoTop, F64 &orthoBottom)
{
   // Set up variables according to display mode
   switch(displayMode)
   {
      case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
         sdlWindowWidth  = gScreenInfo.getPhysicalScreenWidth();
         sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
         orthoLeft   = 0;
         orthoRight  = gScreenInfo.getGameCanvasWidth();
         orthoBottom = gScreenInfo.getGameCanvasHeight();
         orthoTop    = 0;
         break;

      case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
         sdlWindowWidth  = gScreenInfo.getPhysicalScreenWidth();
         sdlWindowHeight = gScreenInfo.getPhysicalScreenHeight();
         orthoLeft   = -1 * gScreenInfo.getHorizDrawMargin();
         orthoRight  = gScreenInfo.getGameCanvasWidth() + gScreenInfo.getHorizDrawMargin();
         orthoBottom = gScreenInfo.getGameCanvasHeight() + gScreenInfo.getVertDrawMargin();
         orthoTop    = -1 * gScreenInfo.getVertDrawMargin();
         break;

      case DISPLAY_MODE_WINDOWED:
      default:  //  Fall through OK
         sdlWindowWidth  = (S32) floor((F32)gScreenInfo.getGameCanvasWidth()  * settings->getIniSettings()->winSizeFact + 0.5f);
         sdlWindowHeight = (S32) floor((F32)gScreenInfo.getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);
         orthoLeft   = 0;
         orthoRight  = gScreenInfo.getGameCanvasWidth();
         orthoBottom = gScreenInfo.getGameCanvasHeight();
         orthoTop    = 0;
         break;
   }
}

} /* namespace Zap */
