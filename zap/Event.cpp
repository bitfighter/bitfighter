/*
 * Input.cpp
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */

#include "Event.h"

#include "oglconsole.h"
#include "config.h"
#include "UIMenus.h"
#include "UIDiagnostics.h"
#include "IniFile.h"
#include "screenShooter.h"

#include "SDL/SDL_opengl.h"

#include <cmath>

using namespace TNL;

namespace Zap
{

Event::Event()
{
}

Event::~Event()
{
}

void setMousePos(S32 x, S32 y)
{
   DisplayMode reportedDisplayMode = gIniSettings.displayMode;

   if(UserInterface::current->getMenuID() == EditorUI && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   gScreenInfo.setMousePos(x, y, reportedDisplayMode);
}

void Event::onEvent(SDL_Event* event)
{
   switch (event->type)
   {
      case SDL_KEYDOWN:
         onKeyDown(event->key.keysym.sym, event->key.keysym.mod, event->key.keysym.unicode);
         break;

      case SDL_KEYUP:
         onKeyUp(event->key.keysym.sym, event->key.keysym.mod, event->key.keysym.unicode);
         break;

      case SDL_MOUSEMOTION:
            onMouseMoved(event->motion.x, event->motion.y, event->motion.xrel, event->motion.yrel, (event->motion.state
                  & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0, (event->motion.state
                  & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0, (event->motion.state
                  & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseLeftButtonDown(event->button.x, event->button.y);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseRightButtonDown(event->button.x, event->button.y);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseMiddleButtonDown(event->button.x, event->button.y);
               break;
         }
         break;

      case SDL_MOUSEBUTTONUP:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseLeftButtonUp(event->button.x, event->button.y);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseRightButtonUp(event->button.x, event->button.y);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseMiddleButtonUp(event->button.x, event->button.y);
               break;
         }
         break;

      case SDL_JOYAXISMOTION:
         onJoyAxis(event->jaxis.which, event->jaxis.axis, event->jaxis.value);
         break;

      case SDL_JOYBALLMOTION:
         onJoyBall(event->jball.which, event->jball.ball, event->jball.xrel, event->jball.yrel);
         break;

      case SDL_JOYHATMOTION:
         onJoyHat(event->jhat.which, event->jhat.hat, event->jhat.value);
         break;

      case SDL_JOYBUTTONDOWN:
         onJoyButtonDown(event->jbutton.which, event->jbutton.button);
         break;

      case SDL_JOYBUTTONUP:
         onJoyButtonUp(event->jbutton.which, event->jbutton.button);
         break;

      case SDL_SYSWMEVENT:
         //Ignore
         break;

      case SDL_VIDEORESIZE:
         onResize(event->resize.w, event->resize.h);
         break;

      case SDL_VIDEOEXPOSE:
         onExpose();
         break;

      case SDL_ACTIVEEVENT:
         switch (event->active.state)
         {
            case SDL_APPMOUSEFOCUS:
               if (event->active.gain)
                  onMouseFocus();
               else
                  onMouseBlur();

               break;

            case SDL_APPINPUTFOCUS:
               if (event->active.gain)
                  onInputFocus();
               else
                  onInputBlur();

               break;

            case SDL_APPACTIVE:
               if (event->active.gain)
                  onRestore();
               else
                  onMinimize();

               break;
         }
         break;

      default:
         onUser(event->user.type, event->user.code, event->user.data1, event->user.data2);
         break;
   }
}

void Event::onInputFocus()
{

}

void Event::onInputBlur()
{

}

void Event::onKeyDown(SDLKey key, SDLMod mod, U16 unicode)
{
   // Global modifiers

   // ALT + ENTER --> toggles window mode/full screen
   if(key == SDLK_RETURN && (mod & KMOD_ALT))
      gOptionsMenuUserInterface.toggleDisplayMode();

   // CTRL + Q --> screenshot!
   else if(key == SDLK_q && (mod & KMOD_CTRL))
      gScreenshooter.phase = 1;

   // Turn on diagnostic overlay -- can be used anywhere
   else if(key == SDLK_F7)
      gDiagnosticInterface.activate();

   // The rest
   else
   {
      KeyCode keyCode = standardSDLKeyToKeyCode(key);
      setKeyState(keyCode, true);

      if(UserInterface::current)
         UserInterface::current->onKeyDown(keyCode, keyToAscii(unicode, keyCode));
   }
}

void Event::onKeyUp(SDLKey key, SDLMod mod, U16 unicode)
{
   KeyCode keyCode = standardSDLKeyToKeyCode(key);
   setKeyState(keyCode, false);

   if(UserInterface::current)
      UserInterface::current->onKeyUp(keyCode);
}

void Event::onMouseFocus()
{

}

void Event::onMouseBlur()
{

}

void Event::onMouseMoved(S32 x, S32 y, S32 relX, S32 relY, bool Left, bool Right, bool Middle)
{
   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}


void Event::onMouseWheel(bool Up, bool Down)
{

}

void Event::onMouseLeftButtonDown(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_LEFT, true);

   UserInterface::current->onKeyDown(MOUSE_LEFT, 0);
}

void Event::onMouseLeftButtonUp(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_LEFT, false);

   UserInterface::current->onKeyUp(MOUSE_LEFT);
}

void Event::onMouseRightButtonDown(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_RIGHT, true);

   UserInterface::current->onKeyDown(MOUSE_RIGHT, 0);
}

void Event::onMouseRightButtonUp(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_RIGHT, false);

   UserInterface::current->onKeyUp(MOUSE_RIGHT);
}

void Event::onMouseMiddleButtonDown(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_MIDDLE, true);

   UserInterface::current->onKeyDown(MOUSE_MIDDLE, 0);
}

void Event::onMouseMiddleButtonUp(S32 x, S32 y)
{
   setMousePos(x, y);

   if(!UserInterface::current)
      return;    // Bail if no current UI

   setKeyState(MOUSE_MIDDLE, false);

   UserInterface::current->onKeyUp(MOUSE_MIDDLE);
}

void Event::onJoyAxis(U8 which, U8 axis, S16 value)
{

}

void Event::onJoyButtonDown(U8 which, U8 button)
{

}

void Event::onJoyButtonUp(U8 which, U8 button)
{

}

void Event::onJoyHat(U8 which, U8 hat, U8 value)
{

}

void Event::onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel)
{

}

void Event::onMinimize()
{

}

void Event::onRestore()
{

}

void Event::onResize(S32 width, S32 height)
{
   // If we are entering fullscreen mode, then we don't want to mess around with proportions and all that.  Just save window size and get out.
   if(gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_STRETCHED || gIniSettings.displayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      gScreenInfo.setWindowSize(width, height);

   else
   {
      S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
      S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

      // Constrain window to correct proportions...
      if((width - canvasWidth) > (height - canvasHeight))      // Wider than taller  (is this right? mixing virtual and physical pixels)
         gIniSettings.winSizeFact = max((F32) height / (F32)canvasHeight, gScreenInfo.getMinScalingFactor());
      else
         gIniSettings.winSizeFact = max((F32) width / (F32)canvasWidth, gScreenInfo.getMinScalingFactor());

      S32 newWidth  = (S32)floor(canvasWidth  * gIniSettings.winSizeFact + 0.5f);   // virtual * (physical/virtual) = physical, fix rounding problem
      S32 newHeight = (S32)floor(canvasHeight * gIniSettings.winSizeFact + 0.5f);

      S32 flags = 0;
      flags = gScreenInfo.isHardwareSurface() ? SDL_OPENGL | SDL_HWSURFACE | SDL_RESIZABLE : SDL_OPENGL | SDL_RESIZABLE;
      SDL_SetVideoMode(newWidth, newHeight, 0, flags);
      gScreenInfo.setWindowSize(newWidth, newHeight);
   }

   glViewport(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());
   OGLCONSOLE_Reshape();

   gINI.SetValueF("Settings", "WindowScalingFactor", gIniSettings.winSizeFact, true);
}

void Event::onExpose()
{

}

void Event::onUser(U8 type, S32 code, void* data1, void* data2)
{

}

}
