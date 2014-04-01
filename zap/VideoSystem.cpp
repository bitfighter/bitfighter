//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "VideoSystem.h"

#include "GameManager.h"
#include "ClientGame.h"
#include "IniFile.h"
#include "Console.h"
#include "DisplayManager.h"
#include "UI.h"
#include "version.h"
#include "FontManager.h"
#include "UIManager.h"

#include "stringUtils.h"

#include "tnlLog.h"

//#include "SDL.h"

#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
#  include "SDL_opengles.h"
//   // Needed for GLES compatibility
#  define glOrtho glOrthof
//#else
//#  include "SDL_opengl.h"
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


extern string getInstalledDataDir();

static string WINDOW_TITLE = "Bitfighter " + string(ZAP_GAME_RELEASE);

// Returns true if everything went ok, false otherwise
bool VideoSystem::init()
{
   // Make sure "SDL_Init(0)" was done before calling this function

   // First, initialize SDL's video subsystem
   if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
   {
      // Failed, exit
      logprintf(LogConsumer::LogFatalError, "SDL Video initialization failed: %s", SDL_GetError());
      return false;
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

   DisplayManager::getScreenInfo()->init(mode.w, mode.h);

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

#ifdef TNL_OS_MAC_OSX
   // This hint is to workaround this bug: https://bugzilla.libsdl.org/show_bug.cgi?id=1840
   SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif

   // SDL 2.0 lets us create the window first, only once
   DisplayManager::getScreenInfo()->sdlWindow = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
         DisplayManager::getScreenInfo()->getWindowWidth(), DisplayManager::getScreenInfo()->getWindowHeight(), flags);

   if(!DisplayManager::getScreenInfo()->sdlWindow)
   {
      logprintf(LogConsumer::LogFatalError, "SDL window creation failed: %s", SDL_GetError());
      return false;
   }

   // Create our OpenGL context; save it in case we ever need it
   SDL_GLContext context = SDL_GL_CreateContext(DisplayManager::getScreenInfo()->sdlWindow);
   DisplayManager::getScreenInfo()->sdlGlContext = &context;

   // We can set up vsync, too
   SDL_GL_SetSwapInterval(1);

#else

   const SDL_VideoInfo* info = SDL_GetVideoInfo();

   DisplayManager::getScreenInfo()->init(info->current_w, info->current_h);

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
      SDL_SetWindowIcon(DisplayManager::getScreenInfo()->sdlWindow, icon);
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

   return true;
}


#if !SDL_VERSION_ATLEAST(2,0,0)
static SDL_SysWMinfo windowManagerInfo;
#endif

void VideoSystem::setWindowPosition(S32 left, S32 top)
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_SetWindowPosition(DisplayManager::getScreenInfo()->sdlWindow, left, top);
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
   SDL_GetWindowPosition(DisplayManager::getScreenInfo()->sdlWindow, &x, &y);

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

// Actually put us in windowed or full screen mode.  Pass true the first time this is used, false subsequently.
// This has the unfortunate side-effect of triggering a mouse move event.
void VideoSystem::actualizeScreenMode(GameSettings *settings, bool changingInterfaces, bool currentUIUsesEditorScreenMode)
{
   DisplayMode displayMode = settings->getIniSettings()->mSettings.getVal<DisplayMode>(IniKey::WindowMode);

   DisplayManager::getScreenInfo()->resetGameCanvasSize();     // Set GameCanvasSize vars back to their default values
   DisplayManager::getScreenInfo()->setActualized();


   // If old display mode is windowed or current is windowed but we change interfaces,
   // save the window position
   if(settings->getIniSettings()->oldDisplayMode == DISPLAY_MODE_WINDOWED ||
         (changingInterfaces && displayMode == DISPLAY_MODE_WINDOWED))
   {
      settings->getIniSettings()->winXPos = getWindowPositionX();
      settings->getIniSettings()->winYPos = getWindowPositionY();

      GameSettings::iniFile.SetValueI("Settings", "WindowXPos", settings->getIniSettings()->winXPos, true);
      GameSettings::iniFile.SetValueI("Settings", "WindowYPos", settings->getIniSettings()->winYPos, true);
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
      magFactor = max(magFactor, (F32)DisplayManager::getScreenInfo()->getGameCanvasHeight() / (F32)DisplayManager::getScreenInfo()->getPhysicalScreenHeight());

      DisplayManager::getScreenInfo()->setGameCanvasSize(S32(DisplayManager::getScreenInfo()->getPhysicalScreenWidth() * magFactor), S32(DisplayManager::getScreenInfo()->getPhysicalScreenHeight() * magFactor));

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
      SDL_SetWindowSize(DisplayManager::getScreenInfo()->sdlWindow, sdlWindowWidth, sdlWindowHeight);
      SDL_SetWindowFullscreen(DisplayManager::getScreenInfo()->sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

      break;

   case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
      SDL_SetWindowSize(DisplayManager::getScreenInfo()->sdlWindow, sdlWindowWidth, sdlWindowHeight);
      SDL_SetWindowFullscreen(DisplayManager::getScreenInfo()->sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

      break;

   case DISPLAY_MODE_WINDOWED:
   default:
      // Reverse order, leave fullscreen before setting size
      SDL_SetWindowFullscreen(DisplayManager::getScreenInfo()->sdlWindow, 0);
      SDL_SetWindowSize(DisplayManager::getScreenInfo()->sdlWindow, sdlWindowWidth, sdlWindowHeight);
      break;
   }

   if(settings->getIniSettings()->mSettings.getVal<YesNo>(IniKey::DisableScreenSaver))
      SDL_DisableScreenSaver();
   else
      SDL_EnableScreenSaver();

   // Flush window events because SDL_SetWindowSize triggers a SDL_WINDOWEVENT_RESIZED 
   // event (which in turn triggers another SDL_SetWindowSize)
   SDL_FlushEvent(SDL_WINDOWEVENT);
#else
   // Set up sdl video flags according to display mode
   S32 sdlVideoFlags = SDL_OPENGL;

   switch (displayMode)
   {
      case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
         sdlVideoFlags |= settings->getIniSettings()->mSettings.getVal<YesNo>(IniKey::UseFakeFullscreen) ? SDL_NOFRAME : SDL_FULLSCREEN;
         break;

      case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
         sdlVideoFlags |= settings->getIniSettings()->mSettings.getVal<YesNo>(IniKey::UseFakeFullscreen)  ? SDL_NOFRAME : SDL_FULLSCREEN;
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
   DisplayManager::getScreenInfo()->setWindowSize(sdlWindowWidth, sdlWindowHeight);

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
      glScissor(DisplayManager::getScreenInfo()->getHorizPhysicalMargin(),    // x
                DisplayManager::getScreenInfo()->getVertPhysicalMargin(),     // y
                DisplayManager::getScreenInfo()->getDrawAreaWidth(),          // width
                DisplayManager::getScreenInfo()->getDrawAreaHeight());        // height
   }
   else
   {
      // Enabling scissor appears to fix crashing problem switching screen mode
      // in linux and "Mobile 945GME Express Integrated Graphics Controller",
      // probably due to lines and points was not being clipped,
      // causing some lines to wrap around the screen, or by writing other
      // parts of RAM that can crash Bitfighter, graphics driver, or the entire computer.
      // This is probably a bug in the Linux Intel graphics driver.
      glScissor(0, 0, DisplayManager::getScreenInfo()->getWindowWidth(), DisplayManager::getScreenInfo()->getWindowHeight());
   }

   glEnable(GL_SCISSOR_TEST);    // Turn on clipping

   setDefaultBlendFunction();
   glLineWidth(gDefaultLineWidth);

   // Enable Line smoothing everywhere!  Make sure to disable temporarily for filled polygons and such
   if(settings->getIniSettings()->mSettings.getVal<YesNo>(IniKey::LineSmoothing))
   {
      glEnable(GL_LINE_SMOOTH);
      //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   }

   glEnable(GL_BLEND);

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
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();
   for(S32 i = 0; i < clientGames->size(); i++)
      if(clientGames->get(i)->getUIManager()->getCurrentUI())
         clientGames->get(i)->getUIManager()->getCurrentUI()->onDisplayModeChange();

   // Re-initialize our fonts because OpenGL textures can be lost upon screen change
   FontManager::reinitialize(settings);

   // This needs to happen after font re-initialization because I think fontstash interferes
   // with the oglconsole font somehow...
   gConsole.onScreenModeChanged();
}


void VideoSystem::getWindowParameters(GameSettings *settings, DisplayMode displayMode, 
                                      S32 &sdlWindowWidth, S32 &sdlWindowHeight, F64 &orthoLeft, F64 &orthoRight, F64 &orthoTop, F64 &orthoBottom)
{
   // Set up variables according to display mode
   switch(displayMode)
   {
      case DISPLAY_MODE_FULL_SCREEN_STRETCHED:
         sdlWindowWidth  = DisplayManager::getScreenInfo()->getPhysicalScreenWidth();
         sdlWindowHeight = DisplayManager::getScreenInfo()->getPhysicalScreenHeight();
         orthoLeft   = 0;
         orthoRight  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
         orthoBottom = DisplayManager::getScreenInfo()->getGameCanvasHeight();
         orthoTop    = 0;
         break;

      case DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED:
         sdlWindowWidth  = DisplayManager::getScreenInfo()->getPhysicalScreenWidth();
         sdlWindowHeight = DisplayManager::getScreenInfo()->getPhysicalScreenHeight();
         orthoLeft   = -1 * DisplayManager::getScreenInfo()->getHorizDrawMargin();
         orthoRight  = DisplayManager::getScreenInfo()->getGameCanvasWidth() + DisplayManager::getScreenInfo()->getHorizDrawMargin();
         orthoBottom = DisplayManager::getScreenInfo()->getGameCanvasHeight() + DisplayManager::getScreenInfo()->getVertDrawMargin();
         orthoTop    = -1 * DisplayManager::getScreenInfo()->getVertDrawMargin();
         break;

      case DISPLAY_MODE_WINDOWED:
      default:  //  Fall through OK
         sdlWindowWidth  = (S32) floor((F32)DisplayManager::getScreenInfo()->getGameCanvasWidth()  * settings->getIniSettings()->winSizeFact + 0.5f);
         sdlWindowHeight = (S32) floor((F32)DisplayManager::getScreenInfo()->getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);
         orthoLeft   = 0;
         orthoRight  = DisplayManager::getScreenInfo()->getGameCanvasWidth();
         orthoBottom = DisplayManager::getScreenInfo()->getGameCanvasHeight();
         orthoTop    = 0;
         break;
   }
}

} /* namespace Zap */
