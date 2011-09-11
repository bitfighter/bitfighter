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
#include "ScreenShooter.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "ClientGame.h"

#include "SDL/SDL_opengl.h"

#include <cmath>

namespace Zap
{

Event::Event()
{
}

Event::~Event()
{
}

void Event::setMousePos(S32 x, S32 y)
{
   DisplayMode reportedDisplayMode = gIniSettings.displayMode;

   if(UserInterface::current->getMenuID() == EditorUI && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   gScreenInfo.setMousePos(x, y, reportedDisplayMode);
}

// Argument of axisMask is one of the 4 axes:
//    MoveAxisLeftRightMask, MoveAxisUpDownMask, ShootAxisLeftRightMask, ShootAxisUpDownMask
void Event::updateJoyAxesDirections(U32 axisMask, S16 value)
{
   // Get our current joystick-axis-direction and its opposite on the same axis
   U32 detectedAxesDirectionMask = 0;
   U32 oppositeDetectedAxesDirectionMask = 0;
   if (value < 0)
   {
      detectedAxesDirectionMask = axisMask & NegativeAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & PositiveAxesMask;
   }
   else
   {
      detectedAxesDirectionMask = axisMask & PositiveAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & NegativeAxesMask;
   }

   // Get the specific axes direction index (and opposite) from the mask we've detected
   // from enum JoystickAxesDirections
   U32 axesDirectionIndex = 0;
   U32 oppositeAxesDirectionIndex = 0;
   for (S32 i = 0; i < MaxAxesDirections; i++)
   {
      if(Joystick::JoystickInputData[i].axesMask & detectedAxesDirectionMask)
         axesDirectionIndex = i;
      if(Joystick::JoystickInputData[i].axesMask & oppositeDetectedAxesDirectionMask)
         oppositeAxesDirectionIndex = i;
   }


   // Update our global joystick input data, use a sensitivity threshold to take care of calibration issues
   // Also, normalize the input value to a floating point scale of 0 to 1
   F32 normalValue;
   if (value < -Joystick::SensitivityThreshold)
      normalValue = (F32)(-value - Joystick::SensitivityThreshold)/(F32)(S16_MAX - Joystick::SensitivityThreshold);
   else if (value > Joystick::SensitivityThreshold)
      normalValue = (F32)(value - Joystick::SensitivityThreshold)/(F32)(S16_MAX - Joystick::SensitivityThreshold);
   else
      normalValue = 0.0f;

   Joystick::JoystickInputData[axesDirectionIndex].value = normalValue;


   // Now set the opposite axis back to zero
   Joystick::JoystickInputData[oppositeAxesDirectionIndex].value = 0.0f;


   // Determine what to set the KeyCode key state, it is binary so set the threshold has half -> 0.5
   // Set the mask if it is above the digital threshold
   bool keyState = false;
   U32 currentKeyCodeMask = 0;

   for (S32 i = 0; i < MaxAxesDirections; i++)
      if (fabs(Joystick::JoystickInputData[i].value) > 0.5)
      {
         keyState = true;
         currentKeyCodeMask |= (1 << i);
      }


   // Only change KeyCode key state if the axis has changed.  Time to be tricky..
   U32 keyCodeDownDeltaMask = currentKeyCodeMask & ~Joystick::AxesKeyCodeMask;
   U32 keyCodeUpDeltaMask = ~currentKeyCodeMask & Joystick::AxesKeyCodeMask;

   for (S32 i = 0; i < MaxAxesDirections; i++)
   {
      // If the current axes direction is set in the keyCodeDownDeltaMask, set the key down
      if (Joystick::JoystickInputData[i].axesMask & keyCodeDownDeltaMask)
      {
         keyCodeDown(Joystick::JoystickInputData[i].keyCode, 0);
         continue;
      }

      // If the current axes direction is set in the keyCodeUpDeltaMask, set the key up
      if (Joystick::JoystickInputData[i].axesMask & keyCodeUpDeltaMask)
         keyCodeUp(Joystick::JoystickInputData[i].keyCode);
   }


   // Finally alter the global axes KeyCode mask to reflect the current keyState
   Joystick::AxesKeyCodeMask = currentKeyCodeMask;
}


void Event::keyCodeUp(KeyCode keyCode)
{
   setKeyState(keyCode, false);

   if(UserInterface::current)
      UserInterface::current->onKeyUp(keyCode);
}


void Event::keyCodeDown(KeyCode keyCode, char ascii)
{
   setKeyState(keyCode, true);

   if(UserInterface::current)
      UserInterface::current->onKeyDown(keyCode, ascii);
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
            onMouseMoved(event->motion.x, event->motion.y);
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_LEFT);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_RIGHT);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_MIDDLE);
               break;
         }
         break;

      case SDL_MOUSEBUTTONUP:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_LEFT);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_RIGHT);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_MIDDLE);
               break;
         }
         break;

      case SDL_JOYAXISMOTION:
         onJoyAxis(event->jaxis.which, event->jaxis.axis, event->jaxis.value);
         break;

         // TODO:  Do we need joyball and joyhat?
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


void Event::onInputBlur()     // <=== what does this do??
{
   // Do nothing
}


void Event::onKeyDown(SDLKey key, SDLMod mod, U16 unicode)
{
//   logprintf("Key: %d; Mod: %d", key, mod);

   // Global modifiers

   // ALT + ENTER --> toggles window mode/full screen
   if(key == SDLK_RETURN && (mod & KMOD_ALT))
      gClientGame->getUIManager()->getOptionsMenuUserInterface()->toggleDisplayMode();

   // CTRL + Q --> screenshot!
   else if(key == SDLK_q && (mod & KMOD_CTRL))
      ScreenShooter::saveScreenshot(gClientGame->getSettings()->getConfigDirs()->screenshotDir);

   // The rest
   else
   {
      KeyCode keyCode = sdlKeyToKeyCode(key);
      keyCodeDown(keyCode, keyToAscii(unicode, keyCode));
   }
}


void Event::onKeyUp(SDLKey key, SDLMod mod, U16 unicode)
{
   keyCodeUp(sdlKeyToKeyCode(key));
}

void Event::onMouseFocus()
{

}

void Event::onMouseBlur()
{

}

void Event::onMouseMoved(S32 x, S32 y)
{
   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}


void Event::onMouseWheel(bool Up, bool Down)
{

}

void Event::onMouseButtonDown(S32 x, S32 y, KeyCode keyCode)
{
   setMousePos(x, y);

   keyCodeDown(keyCode, 0);
}

void Event::onMouseButtonUp(S32 x, S32 y, KeyCode keyCode)
{
   setMousePos(x, y);

   keyCodeUp(keyCode);
}


void Event::onJoyAxis(U8 whichJoystick, U8 axis, S16 value)
{
//   logprintf("SDL Axis number: %u, value: %d", axis, value);

   if(axis < Joystick::rawAxisCount)
      Joystick::rawAxis[axis] = (F32)value / (F32)S16_MAX;

   // If we are using a predefined controller, get the appropriate axis
   if (gIniSettings.joystickType < ControllerTypeCount)
   {
      // Left/Right movement axis
      if(axis == Joystick::PredefinedJoystickList[gIniSettings.joystickType].moveAxesSdlIndex[0])
         updateJoyAxesDirections(MoveAxisLeftRightMask,  value);

      // Up/down movement axis
      if(axis == Joystick::PredefinedJoystickList[gIniSettings.joystickType].moveAxesSdlIndex[1])
         updateJoyAxesDirections(MoveAxisUpDownMask,  value);

      // Left/Right shooting axis
      if(axis == Joystick::PredefinedJoystickList[gIniSettings.joystickType].shootAxesSdlIndex[0])
         updateJoyAxesDirections(ShootAxisLeftRightMask, value);

      // Up/down shooting axis
      if(axis == Joystick::PredefinedJoystickList[gIniSettings.joystickType].shootAxesSdlIndex[1])
         updateJoyAxesDirections(ShootAxisUpDownMask, value);
   }
   // Else just use the first 4 axis for fun
   else {
      if (axis == 0)
         updateJoyAxesDirections(MoveAxisLeftRightMask,  value);
      if (axis == 1)
         updateJoyAxesDirections(MoveAxisUpDownMask,  value);
      if (axis == 2)
         updateJoyAxesDirections(ShootAxisLeftRightMask, value);
      if (axis == 3)
         updateJoyAxesDirections(ShootAxisUpDownMask, value);
   }
}

extern KeyCode joyButtonToKeyCode(int buttonIndex);

void Event::onJoyButtonDown(U8 which, U8 button)
{
   keyCodeDown(joyButtonToKeyCode(Joystick::remapJoystickButton(button)), 0);
}

void Event::onJoyButtonUp(U8 which, U8 button)
{
   keyCodeUp(joyButtonToKeyCode(Joystick::remapJoystickButton(button)));
}


extern KeyCode joyHatToKeyCode(int hatDirectionMask);

// See SDL_Joystick.h for the SDL_HAT_* mask definitions
void Event::onJoyHat(U8 which, U8 hat, U8 directionMask)
{
//   logprintf("SDL Hat number: %u, value: %u", hat, directionMask);

   KeyCode keyCode;
   U32 keyCodeDownDeltaMask = directionMask & ~Joystick::HatKeyCodeMask;
   U32 keyCodeUpDeltaMask = ~directionMask & Joystick::HatKeyCodeMask;

   for (S32 i = 0; i < MaxHatDirections; i++)
   {
      keyCode = joyHatToKeyCode(BIT(i));  // BIT(i) corresponds to a defined SDL_HAT_*  value

      // If the current hat direction is set in the keyCodeDownDeltaMask, set the key down
      if (keyCodeDownDeltaMask & BIT(i))
         keyCodeDown(keyCode, 0);

      // If the current hat direction is set in the keyCodeUpDeltaMask, set the key up
      if (keyCodeUpDeltaMask & BIT(i))
         keyCodeUp(keyCode);
   }

   // Finally alter the global hat KeyCode mask to reflect the current keyState
   Joystick::HatKeyCodeMask = directionMask;
}

void Event::onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel)
{
//   logprintf("SDL Ball number: %u, relative x: %d, relative y: %d", ball, xrel, yrel);
}

void Event::onMinimize()
{

}

void Event::onRestore()
{

}

// We don't need to worry about this event in fullscreen modes because it is never fired with SDL
void Event::onResize(S32 width, S32 height)
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
