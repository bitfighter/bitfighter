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

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#endif

#ifdef BF_USE_LEGACY_GL
#  include "GLLegacyRenderer.h"
#else
#  include "GL2Renderer.h"
#endif

#include <cmath>

namespace Zap
{

// Init static vars
VideoSystem::videoSystem_st_t VideoSystem::currentState = init_st;


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

#ifdef __EMSCRIPTEN__
   // Route keyboard input to the page canvas. Emscripten's event target
   // lookup treats this as a CSS selector.
   SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif

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

#ifdef __EMSCRIPTEN__
   // WebGL is OpenGL ES 2.0. A desktop 2.1 context request does not map.
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif !defined(BF_USE_LEGACY_GL)
   // Tell SDL which OpenGL version we will use
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, BF_GL_MAJOR_VERSION);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, BF_GL_MINOR_VERSION);
#endif

   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );  // depth used in editor to display spybug visible area non-overlap
   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);

   // Get information about the current desktop video settings and initialize
   // our ScreenInfo class with with current width and height

   SDL_DisplayMode mode;

   SDL_GetCurrentDisplayMode(0, &mode);  // We only have one display..  for now

#ifdef __EMSCRIPTEN__
   // Some canvases report 0 (or the 300x150 default) before layout.
   if(mode.w < 640 || mode.h < 480)
   {
      mode.w = 1280;
      mode.h = 720;
   }
#endif

   DisplayManager::getScreenInfo()->init(mode.w, mode.h);

   S32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
#ifdef __EMSCRIPTEN__
   flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

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

   // No minimizing when ALT-TAB away
   SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

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

   // Initialize renderer
#ifdef BF_USE_LEGACY_GL
   GLLegacyRenderer::create();
#else
   GL2Renderer::create();
#endif

   // Set the window icon -- note that the icon must be a 32x32 bmp, and SDL will
   // downscale it to 16x16 with no interpolation.  Therefore, it's best to start
   // with a finely crafted 16x16 icon, then scale it up to 32x32 with no interpolation.
   // It will look crappy at 32x32, but good at 16x16, and that's all that really matters.

   // Save bmp as a 32 bit XRGB bmp file (Gimp can do it!)
   string iconPath = getInstalledDataDir() + getFileSeparator() + "bficon.bmp";
   SDL_Surface *icon = SDL_LoadBMP(iconPath.c_str());

   // OSX handles icons better with its native .icns file
#ifndef TNL_OS_MAC_OSX
   if(icon != NULL)
      SDL_SetWindowIcon(DisplayManager::getScreenInfo()->sdlWindow, icon);
#endif

   if(icon != NULL)
      SDL_FreeSurface(icon);

   // We will set the resolution, position, and flags in updateDisplayState()

   return true;
}

// Performs a clean shutdown of the video system
void VideoSystem::shutdown()
{
   Renderer::shutdown();
   SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VideoSystem::setWindowPosition(S32 left, S32 top)
{
   SDL_SetWindowPosition(DisplayManager::getScreenInfo()->sdlWindow, left, top);
}


void VideoSystem::saveWindowPosition(GameSettings *settings)
{
   S32 x, y;
   SDL_GetWindowPosition(DisplayManager::getScreenInfo()->sdlWindow, &x, &y);

#ifdef TNL_OS_LINUX
   // Sometimes X11 in Linux will save position with window decorations in
   // certain cases.  This creates an offset and the window will creep
   S32 top = 0, left = 0;
   SDL_GetWindowBordersSize(DisplayManager::getScreenInfo()->sdlWindow, &top, &left, NULL, NULL);

   // Subtract window decorations from the window coords
   x = x - left;
   y = y - top;
#endif

   settings->getIniSettings()->winXPos = x;
   settings->getIniSettings()->winYPos = y;

   GameSettings::iniFile.SetValueI("Settings", "WindowXPos", settings->getIniSettings()->winXPos, true);
   GameSettings::iniFile.SetValueI("Settings", "WindowYPos", settings->getIniSettings()->winYPos, true);
}


bool VideoSystem::isFullscreen()
{
   if(SDL_GetWindowFlags(DisplayManager::getScreenInfo()->sdlWindow) & SDL_WINDOW_FULLSCREEN)
      return true;

   return false;
}


void VideoSystem::saveUpdateWindowScale(GameSettings *settings)
{
   ScreenInfo *screen = DisplayManager::getScreenInfo();

   S32 windowWidth, windowHeight;
   SDL_GetWindowSize(screen->sdlWindow, &windowWidth, &windowHeight);

   S32 canvasHeight = screen->getGameCanvasHeight();
   S32 canvasWidth = screen->getGameCanvasWidth();

   // Find the window scaling factor
   F32 scale = 1;
   // Wider than taller
   if((windowWidth - canvasWidth) > (windowHeight - canvasHeight))
      scale = max((F32) windowHeight / (F32)canvasHeight, screen->getMinScalingFactor());
   else
      scale = max((F32) windowWidth / (F32)canvasWidth, screen->getMinScalingFactor());

   S32 newWidth  = (S32)floor(canvasWidth  * scale + 0.5f);   // virtual * (physical/virtual) = physical, fix rounding problem
   S32 newHeight = (S32)floor(canvasHeight * scale + 0.5f);

   // Save new scaling factor to settings and INI
   settings->getIniSettings()->winSizeFact = scale;
   GameSettings::iniFile.SetValueF("Settings", "WindowScalingFactor", settings->getIniSettings()->winSizeFact, true);

   // Save the new window dimensions in ScreenInfo
   screen->setWindowSize(newWidth, newHeight);
}

void VideoSystem::debugPrintState(VideoSystem::videoSystem_st_t currentState)
{
   logprintf("");
   switch(currentState)
   {
      case VideoSystem::windowed_st:
         logprintf("windowed_st");
         break;

      case VideoSystem::fullscreen_stretched_st:
         logprintf("fullscreen_stretched_st");
         break;

      case VideoSystem::fullscreen_unstretched_st:
         logprintf("fullscreen_unstretched_st");
         break;

      case VideoSystem::windowed_editor_st:
         logprintf("windowed_editor_st");
         break;

      case VideoSystem::fullscreen_editor_st:
         logprintf("fullscreen_editor_st");
         break;

      case VideoSystem::init_st:
         logprintf("unknown_st");
         break;

      default:
         // stay put
         break;
   }
}


#ifdef __EMSCRIPTEN__
// Browser fullscreen is requested only for an explicit toggle. Startup and
// resizes just re-fit the canvas the page already sized.
static bool emscriptenFullscreenReason(VideoSystem::StateReason reason)
{
   return reason == VideoSystem::StateReasonToggle
       || reason == VideoSystem::StateReasonModeDirectWindowed
       || reason == VideoSystem::StateReasonModeDirectFullscreenStretched
       || reason == VideoSystem::StateReasonModeDirectFullscreenUnstretched;
}

static bool emscriptenStateIsFullscreen(VideoSystem::videoSystem_st_t state)
{
   return state == VideoSystem::fullscreen_stretched_st
       || state == VideoSystem::fullscreen_unstretched_st
       || state == VideoSystem::fullscreen_editor_st;
}

// Keep the backbuffer at the CSS box times devicePixelRatio and letterbox the
// 1066x600 canvas inside it. The page CSS owns the box; we do not resize it
// to the game canvas.
static void applyEmscriptenCanvas(ScreenInfo *screen, VideoSystem::videoSystem_st_t state,
      VideoSystem::StateReason reason, DisplayMode &displayMode)
{
   if(emscriptenFullscreenReason(reason))
   {
      SDL_SetWindowFullscreen(screen->sdlWindow,
            emscriptenStateIsFullscreen(state) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
   }

   double cssW = 0;
   double cssH = 0;
   emscripten_get_element_css_size("#canvas", &cssW, &cssH);

   S32 width = (S32)floor(cssW + 0.5);
   S32 height = (S32)floor(cssH + 0.5);
   if(width < 1 || height < 1)
   {
      SDL_GetWindowSize(screen->sdlWindow, &width, &height);
      if(width < 1)
         width = 1;
      if(height < 1)
         height = 1;
   }

   // external_size: this updates the bitmap (CSS * pixel ratio) and the
   // window size SDL reports, and leaves the CSS box alone.
   S32 curW = 0;
   S32 curH = 0;
   SDL_GetWindowSize(screen->sdlWindow, &curW, &curH);
   if(curW != width || curH != height)
      SDL_SetWindowSize(screen->sdlWindow, width, height);

   screen->init(width, height);
   screen->setWindowSize(width, height);

   // Same ortho and scissor the desktop unstretched fullscreen path uses.
   screen->setOrtho(
         -1 * screen->getHorizDrawMargin(),
         screen->getGameCanvasWidth() + screen->getHorizDrawMargin(),
         screen->getGameCanvasHeight() + screen->getVertDrawMargin(),
         -1 * screen->getVertDrawMargin());
   screen->setScissor(
         screen->getHorizPhysicalMargin(),
         screen->getVertPhysicalMargin(),
         screen->getDrawAreaWidth(),
         screen->getDrawAreaHeight());

   if(state == VideoSystem::fullscreen_stretched_st)
      displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   else if(state == VideoSystem::fullscreen_unstretched_st)
      displayMode = DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
   else
      displayMode = DISPLAY_MODE_WINDOWED;

   logprintf("Web canvas %dx%d margins %d,%d", width, height,
         screen->getHorizPhysicalMargin(), screen->getVertPhysicalMargin());

   // The canvas CSS box can change without a window resize (scrollbar, fullscreen).
   // SDL only listens for the window event, so forward those here.
   EM_ASM({
      if (Module['bfCanvasWatch'])
         return;
      Module['bfCanvasWatch'] = true;
      var canvas = document.getElementById('canvas');
      if (!canvas || typeof ResizeObserver === 'undefined')
         return;
      var lastW = 0;
      var lastH = 0;
      new ResizeObserver(function() {
         var rect = canvas.getBoundingClientRect();
         var w = Math.round(rect.width);
         var h = Math.round(rect.height);
         if (w === lastW && h === lastH)
            return;
         lastW = w;
         lastH = h;
         window.dispatchEvent(new Event('resize'));
      }).observe(canvas);
   });
}
#endif


// This is a state-machine to manage all the different video states.  It is
// driven asynchronously
void VideoSystem::updateDisplayState(GameSettings *settings, StateReason reason)
{
   ScreenInfo *screen = DisplayManager::getScreenInfo();

   // INI is where we save the normal display mode, grab it and set it to the old mode
   DisplayMode oldDisplayMode = settings->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode");
   settings->getIniSettings()->oldDisplayMode = oldDisplayMode;

   // Set ScreenInfo back to default values
   screen->resetGameCanvasSize();
   screen->setActualized();

   // Other vars that should be updated in each state
   DisplayMode displayMode;
   S32 windowWidth, windowHeight;

   // START STATE MACHINE
   //
   // Perform state updates first (normally backwards from most FSMs)
   // Here we'll determine what state to go to next
   switch(currentState)
   {
      case init_st:
#ifdef __EMSCRIPTEN__
         // The page owns the canvas. A saved fullscreen mode must not request
         // the browser fullscreen API during startup (no user gesture).
         currentState = windowed_st;
         break;
#endif
         if(oldDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
            currentState = fullscreen_unstretched_st;

         else if(oldDisplayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED)
            currentState = fullscreen_stretched_st;

         else  // Default to windowed
            currentState = windowed_st;

         break;

      case windowed_st:
         if(reason == StateReasonToggle || reason == StateReasonModeDirectFullscreenStretched)
         {
            currentState = fullscreen_stretched_st;

            saveWindowPosition(settings);
         }

         else if(reason == StateReasonModeDirectFullscreenUnstretched)
         {
            currentState = fullscreen_unstretched_st;

            saveWindowPosition(settings);
         }

         else if(reason == StateReasonInterfaceChange)
         {
            currentState = windowed_editor_st;

            saveWindowPosition(settings);
         }

         // Stay here if resized
         else if(reason == StateReasonExternalResize)
         {
            // Get new window dimensions and scaling
            saveUpdateWindowScale(settings);
         }

         break;

      case fullscreen_stretched_st:
         if(reason == StateReasonToggle || reason == StateReasonModeDirectFullscreenUnstretched)
            currentState = fullscreen_unstretched_st;

         else if(reason == StateReasonModeDirectWindowed)
            currentState = windowed_st;

         else if(reason == StateReasonInterfaceChange)
            currentState = fullscreen_editor_st;

         // else resized.  stay here

         break;

      case fullscreen_unstretched_st:
         if(reason == StateReasonToggle || reason == StateReasonModeDirectWindowed)
            currentState = windowed_st;

         else if(reason == StateReasonModeDirectFullscreenStretched)
            currentState = fullscreen_stretched_st;

         else if(reason == StateReasonInterfaceChange)
            currentState = fullscreen_editor_st;

         // else resized.  stay here

         break;

      case windowed_editor_st:
         if(reason == StateReasonToggle)
         {
            currentState = fullscreen_editor_st;

            saveWindowPosition(settings);
         }

         else if(reason == StateReasonInterfaceChange)
         {
            currentState = windowed_st;

            saveWindowPosition(settings);
         }

         // Stay here if resized
         else if(reason == StateReasonExternalResize)
         {
            // Get new window dimensions and scaling
            saveUpdateWindowScale(settings);
         }

         break;

      case fullscreen_editor_st:
         if(reason == StateReasonToggle)
            currentState = windowed_editor_st;

         else if(reason == StateReasonInterfaceChange)
         {
            if(oldDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
               currentState = fullscreen_unstretched_st;

            // This basically means if we don't know, this is default
            else
               currentState = fullscreen_stretched_st;
         }

         // else resized.  stay here

         break;

      default:
         // stay put, should never get here
         break;
   }

//   debugPrintState(currentState);

   // Perform state actions
   // Now that we're in the state we want, alter the video params accordingly
#ifdef __EMSCRIPTEN__
   applyEmscriptenCanvas(screen, currentState, reason, displayMode);
#else
   switch(currentState)
   {
      // We should have moved on by now!
      case init_st:
         break;

      case windowed_st:
         displayMode = DISPLAY_MODE_WINDOWED;

         windowWidth = (S32) floor((F32)screen->getGameCanvasWidth() * settings->getIniSettings()->winSizeFact + 0.5f);
         windowHeight = (S32) floor((F32)screen->getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);

         // Set window position unless it is (0, 0) which can hide the title bar
         if(settings->getIniSettings()->winXPos != 0 || settings->getIniSettings()->winYPos != 0)
            setWindowPosition(settings->getIniSettings()->winXPos, settings->getIniSettings()->winYPos);

         // Leave fullscreen before setting size
         SDL_SetWindowFullscreen(screen->sdlWindow, SDL_DISABLE);
         SDL_SetWindowSize(screen->sdlWindow, windowWidth, windowHeight);

         // Save the new window dimensions in ScreenInfo
         screen->setWindowSize(windowWidth, windowHeight);

         screen->resetOrtho();
         screen->resetScissor();

         break;

      case fullscreen_stretched_st:
         displayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

         windowWidth = screen->getPhysicalScreenWidth();
         windowHeight = screen->getPhysicalScreenHeight();

         SDL_SetWindowSize(screen->sdlWindow, windowWidth, windowHeight);
         SDL_SetWindowFullscreen(screen->sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

         // Save the new window dimensions in ScreenInfo
         screen->setWindowSize(windowWidth, windowHeight);

         screen->resetOrtho();
         screen->resetScissor();

         break;

      case fullscreen_unstretched_st:
         displayMode = DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;

         windowWidth = screen->getPhysicalScreenWidth();
         windowHeight = screen->getPhysicalScreenHeight();

         SDL_SetWindowSize(screen->sdlWindow, windowWidth, windowHeight);
         SDL_SetWindowFullscreen(screen->sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

         // Save the new window dimensions in ScreenInfo
         screen->setWindowSize(windowWidth, windowHeight);

         // Set orthongonal projection to squeeze into window so as to keep normal
         // canvas scaling
         screen->setOrtho(
               -1 * screen->getHorizDrawMargin(),
               screen->getGameCanvasWidth() + screen->getHorizDrawMargin(),
               screen->getGameCanvasHeight() + screen->getVertDrawMargin(),
               -1 * screen->getVertDrawMargin()
               );

         // Set scissor to clip at orthogonal projection edges; helps with artifacts
         screen->setScissor(
               screen->getHorizPhysicalMargin(),
               screen->getVertPhysicalMargin(),
               screen->getDrawAreaWidth(),
               screen->getDrawAreaHeight()
               );

         break;

      case windowed_editor_st:
         // Keep old display mode for editor states
         displayMode = oldDisplayMode;

         windowWidth = (S32) floor((F32)screen->getGameCanvasWidth() * settings->getIniSettings()->winSizeFact + 0.5f);
         windowHeight = (S32) floor((F32)screen->getGameCanvasHeight() * settings->getIniSettings()->winSizeFact + 0.5f);

         // Set window position unless it is (0, 0) which can hide the title bar
         if(settings->getIniSettings()->winXPos != 0 || settings->getIniSettings()->winYPos != 0)
            setWindowPosition(settings->getIniSettings()->winXPos, settings->getIniSettings()->winYPos);

         SDL_SetWindowFullscreen(screen->sdlWindow, SDL_DISABLE);
         SDL_SetWindowSize(screen->sdlWindow, windowWidth, windowHeight);

         // Save the new window dimensions in ScreenInfo
         screen->setWindowSize(windowWidth, windowHeight);

         screen->resetOrtho();
         screen->resetScissor();

         break;

      case fullscreen_editor_st:
      {
         // Keep old display mode for editor states
         displayMode = oldDisplayMode;

         windowWidth = screen->getPhysicalScreenWidth();
         windowHeight = screen->getPhysicalScreenHeight();

         setWindowPosition(0, 0);

         SDL_SetWindowSize(screen->sdlWindow, windowWidth, windowHeight);
         SDL_SetWindowFullscreen(screen->sdlWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

         // Save the new window dimensions in ScreenInfo
         screen->setWindowSize(windowWidth, windowHeight);

         // Smaller values give bigger magnification; makes small things easier to see on full screen
         F32 magFactor = 0.85f;

         // For screens smaller than normal, we need to readjust magFactor to make sure we get the full canvas height crammed onto
         // the screen; otherwise our dock will break.  Since this mode is only used in the editor, we don't really care about
         // screen width; tall skinny screens will work just fine.
         magFactor = max(magFactor, (F32)screen->getGameCanvasHeight() / (F32)screen->getPhysicalScreenHeight());

         screen->setGameCanvasSize(
               S32(screen->getPhysicalScreenWidth() * magFactor),
               S32(screen->getPhysicalScreenHeight() * magFactor)
               );

         screen->resetOrtho();
         screen->resetScissor();
      }

         break;

      default:
         break;
   }
#endif

   //
   // END STATE MACHINE
   //

   // Save normal display mode
   settings->getIniSettings()->mSettings.setVal("WindowMode", displayMode);

   // Disable screensaver
   if(settings->getIniSettings()->disableScreenSaver)
      SDL_DisableScreenSaver();
   else
      SDL_EnableScreenSaver();

   // Set Vsync
   SDL_GL_SetSwapInterval(settings->getIniSettings()->mSettings.getVal<YesNo>("Vsync") ? 1 : 0);

   // Now update the viewport with any state change
   redrawViewport(settings);

   // Re-initialize our fonts because OpenGL textures can be lost upon screen change
   FontManager::reinitialize(settings);

   // This needs to happen after font re-initialization because I think fontstash interferes
   // with the oglconsole font somehow...
   gConsole.onScreenModeChanged();

   // Notify all active UIs that the screen has changed state.  May cause problems
   // in split-screen mode
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();
   for(S32 i = 0; i < clientGames->size(); i++)
      if(clientGames->get(i)->getUIManager()->getCurrentUI())
         clientGames->get(i)->getUIManager()->getCurrentUI()->onDisplayModeChange();
}


// Redraw's the OpenGL display; the actual SDL window size should have been set
void VideoSystem::redrawViewport(GameSettings *settings)
{
   // Get the window's size in screen coordinates.
   S32 windowWidth, windowHeight;
   SDL_GetWindowSize(DisplayManager::getScreenInfo()->sdlWindow, &windowWidth, &windowHeight);

   // Find out the drawable dimensions. If this is a high-DPI display, this may
   // be larger than the window.
   S32 drawWidth, drawHeight;
   SDL_GL_GetDrawableSize(DisplayManager::getScreenInfo()->sdlWindow, &drawWidth, &drawHeight);

   // Get renderer
   Renderer& r = Renderer::get();

   // If we're in HighDPI mode, set a flag in case we want to use later
   bool isHighDpi = (drawWidth > windowWidth) || (drawHeight > windowHeight);
   DisplayManager::getScreenInfo()->setHighDpi(isHighDpi);

   r.setClearColor( 0, 0, 0, 0 );

   // TODO High-DPI mode may change various OpenGL parameters below (and also
#ifdef __EMSCRIPTEN__
   r.setViewport(0, 0, drawWidth, drawHeight);
#else
   r.setViewport(0, 0, windowWidth, windowHeight);
#endif

   r.setMatrixMode(MatrixType::Projection);
   r.loadIdentity();

   // The best understanding I can get for glOrtho is that these are the coordinates you want to appear at the four corners of the
   // physical screen. If you want a "black border" down one side of the screen, you need to make left negative, so that 0 would
   // appear some distance in from the left edge of the physical screen.  The same applies to the other coordinates as well.
   OrthoData ortho = DisplayManager::getScreenInfo()->getOrtho();
   r.projectOrtho((F32)ortho.left, (F32)ortho.right, (F32)ortho.bottom, (F32)ortho.top, 0, 1);

   r.setMatrixMode(MatrixType::ModelView);
   r.loadIdentity();

   // Enabling scissor appears to fix crashing problem switching screen mode
   // in linux and "Mobile 945GME Express Integrated Graphics Controller",
   // probably due to lines and points was not being clipped,
   // causing some lines to wrap around the screen, or by writing other
   // parts of RAM that can crash Bitfighter, graphics driver, or the entire computer.
   // This is probably a bug in the Linux Intel graphics driver.
   // Scissor is in window pixels. GLRenderer scales it on Emscripten.
   ScissorData scissor = DisplayManager::getScreenInfo()->getScissor();
   r.setScissor(scissor.x, scissor.y, scissor.width, scissor.height);

   r.setLineWidth(gDefaultLineWidth);

   // Enable Line smoothing everywhere!  Make sure to disable temporarily for filled polygons and such
   if(settings->getIniSettings()->mSettings.getVal<YesNo>("LineSmoothing"))
      r.enableAntialiasing();
}


} /* namespace Zap */
