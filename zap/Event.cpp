/*
 * Input.cpp
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */

#include "Event.h"

#include "Console.h"
#include "config.h"
#include "UIMenus.h"
#include "IniFile.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "ClientGame.h"
#include "InputCode.h"     // For InputCodeManager def
#include "ScreenShooter.h"

#ifdef TNL_OS_MOBILE
#  include "SDL_opengles.h"
#else
#  include "SDL_opengl.h"
#endif

#include <cmath>

namespace Zap
{

bool Event::mAllowTextInput = false;


Event::Event()
{
   // Do nothing -- never called?
}


Event::~Event()
{
   // Do nothing
}


void Event::setMousePos(UserInterface *currentUI, S32 x, S32 y, DisplayMode reportedDisplayMode)
{
   // Handle special case of editor... would be better handled elsewhere?
   if(currentUI->getMenuID() == EditorUI && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   gScreenInfo.setMousePos(x, y, reportedDisplayMode);
}


// Needs to be Aligned with JoystickAxesDirections... X-Macro?
static JoystickStaticDataStruct JoystickInputData[JoystickAxesDirectionCount] = {
   { JoystickMoveAxesLeft,   MoveAxesLeftMask,   STICK_1_LEFT  },
   { JoystickMoveAxesRight,  MoveAxesRightMask,  STICK_1_RIGHT },
   { JoystickMoveAxesUp,     MoveAxesUpMask,     STICK_1_UP    },
   { JoystickMoveAxesDown,   MoveAxesDownMask,   STICK_1_DOWN  },
   { JoystickShootAxesLeft,  ShootAxesLeftMask,  STICK_2_LEFT  },
   { JoystickShootAxesRight, ShootAxesRightMask, STICK_2_RIGHT },
   { JoystickShootAxesUp,    ShootAxesUpMask,    STICK_2_UP    },
   { JoystickShootAxesDown,  ShootAxesDownMask,  STICK_2_DOWN  },
};


// Argument of axisMask is one of the 4 axes:
//    MoveAxisLeftRightMask, MoveAxisUpDownMask, ShootAxisLeftRightMask, ShootAxisUpDownMask
void Event::updateJoyAxesDirections(ClientGame *game, U32 axisMask, S16 value)
{
   // Get our current joystick-axis-direction and its opposite on the same axis
   U32 detectedAxesDirectionMask = 0;
   U32 oppositeDetectedAxesDirectionMask = 0;
   if (value < 0)
   {
      detectedAxesDirectionMask         = axisMask & NegativeAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & PositiveAxesMask;
   }
   else
   {
      detectedAxesDirectionMask         = axisMask & PositiveAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & NegativeAxesMask;
   }

   // Get the specific axes direction index (and opposite) from the mask we've detected
   // from enum JoystickAxesDirections
   U32 axesDirectionIndex = 0;
   U32 oppositeAxesDirectionIndex = 0;
   for (S32 i = 0; i < JoystickAxesDirectionCount; i++)
   {
      if(JoystickInputData[i].axesMask & detectedAxesDirectionMask)
         axesDirectionIndex = i;
      if(JoystickInputData[i].axesMask & oppositeDetectedAxesDirectionMask)
         oppositeAxesDirectionIndex = i;
   }

   // Update our global joystick input data, use a sensitivity threshold to take care of calibration issues
   // Also, normalize the input value to a floating point scale of 0 to 1
   F32 normalValue;
   S32 absValue = abs(value);
   if(axisMask & (ShootAxisUpDownMask | ShootAxisLeftRightMask))  // shooting have its own deadzone system (see ClientGame.cpp, joystickUpdateMove)
      normalValue = F32(absValue) / 32767.f;
   else if(absValue < Joystick::LowerSensitivityThreshold)
      normalValue = 0.0f;
   else if(absValue > Joystick::UpperSensitivityThreshold)
      normalValue = 1.0f;
   else
      normalValue = (F32)(absValue - Joystick::LowerSensitivityThreshold) / 
                    (F32)(Joystick::UpperSensitivityThreshold - Joystick::LowerSensitivityThreshold);

   game->mJoystickInputs[axesDirectionIndex] = normalValue;

   // Set the opposite axis back to zero
   game->mJoystickInputs[oppositeAxesDirectionIndex] = 0.0f;


   // Determine what to set the InputCode state, it is binary so set the threshold has half -> 0.5
   // Set the mask if it is above the digital threshold
   U32 currentInputCodeMask = 0;

   for (S32 i = 0; i < JoystickAxesDirectionCount; i++)
      if (fabs(game->mJoystickInputs[i]) > 0.5)
         currentInputCodeMask |= (1 << i);


   // Only change InputCode state if the axis has changed.  Time to be tricky..
   U32 inputCodeDownDeltaMask = currentInputCodeMask & ~Joystick::AxesInputCodeMask;
   U32 inputCodeUpDeltaMask = ~currentInputCodeMask & Joystick::AxesInputCodeMask;

   UserInterface *currentUI = game->getUIManager()->getCurrentUI();

   for(S32 i = 0; i < JoystickAxesDirectionCount; i++)
   {
      // If the current axes direction is set in the inputCodeDownDeltaMask, set the input code down
      if(JoystickInputData[i].axesMask & inputCodeDownDeltaMask)
      {
         inputCodeDown(currentUI, JoystickInputData[i].inputCode);
         continue;
      }

      // If the current axes direction is set in the inputCodeUpDeltaMask, set the input code up
      if(JoystickInputData[i].axesMask & inputCodeUpDeltaMask)
         inputCodeUp(currentUI, JoystickInputData[i].inputCode);
   }

   // Finally alter the global axes InputCode mask to reflect the current inputCodeState
   Joystick::AxesInputCodeMask = currentInputCodeMask;
}


void Event::inputCodeUp(UserInterface *currentUI, InputCode inputCode)
{
   InputCodeManager::setState(inputCode, false);

   if(currentUI)
      currentUI->onKeyUp(inputCode);
}


bool Event::inputCodeDown(UserInterface *currentUI, InputCode inputCode)
{
   InputCodeManager::setState(inputCode, true);

   if(currentUI)
      return currentUI->onKeyDown(inputCode);

   return false;
}


void Event::onEvent(ClientGame *game, SDL_Event* event)
{
   IniSettings *iniSettings = game->getSettings()->getIniSettings();
   UserInterface *currentUI = game->getUIManager()->getCurrentUI();

   switch (event->type)
   {
      case SDL_KEYDOWN:
         onKeyDown(game, event);
         break;

      case SDL_KEYUP:
         onKeyUp(currentUI, event);
         break;

#if SDL_VERSION_ATLEAST(2,0,0)
      case SDL_TEXTINPUT:
         if(mAllowTextInput)
            onTextInput(currentUI, event->text.text[0]);

         break;
#endif

      case SDL_MOUSEMOTION:
         onMouseMoved(currentUI, event->motion.x, event->motion.y, iniSettings->displayMode);
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_LEFT, iniSettings->displayMode);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_RIGHT, iniSettings->displayMode);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_MIDDLE, iniSettings->displayMode);
               break;
#if !SDL_VERSION_ATLEAST(2,0,0)
            case SDL_BUTTON_WHEELUP:
               onMouseWheel(currentUI, true, false);
               break;
            case SDL_BUTTON_WHEELDOWN:
               onMouseWheel(currentUI, false, true);
               break;
#endif
         }
         break;

#if SDL_VERSION_ATLEAST(2,0,0)
      case SDL_MOUSEWHEEL:
         if(event->wheel.y > 0)
            onMouseWheel(currentUI, true, false);
         else
            onMouseWheel(currentUI, false, true);

         break;
#endif

      case SDL_MOUSEBUTTONUP:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_LEFT, iniSettings->displayMode);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_RIGHT, iniSettings->displayMode);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_MIDDLE, iniSettings->displayMode);
               break;
         }
         break;

      case SDL_JOYAXISMOTION:
         onJoyAxis(game, event->jaxis.which, event->jaxis.axis, event->jaxis.value);
         break;

      case SDL_JOYBALLMOTION:
         onJoyBall(event->jball.which, event->jball.ball, event->jball.xrel, event->jball.yrel);
         break;

      case SDL_JOYHATMOTION:
         onJoyHat(currentUI, event->jhat.which, event->jhat.hat, event->jhat.value);
         break;

      case SDL_JOYBUTTONDOWN:
         onJoyButtonDown(currentUI, event->jbutton.which, event->jbutton.button);
         break;

      case SDL_JOYBUTTONUP:
         onJoyButtonUp(currentUI, event->jbutton.which, event->jbutton.button);
         break;

      case SDL_SYSWMEVENT:
         //Ignore
         break;
#if SDL_VERSION_ATLEAST(2,0,0)
      case SDL_WINDOWEVENT:
         switch (event->window.event) {
            case SDL_WINDOWEVENT_RESIZED:
               // Ignore window resize events if we are in fullscreen mode
               if(SDL_GetWindowFlags(gScreenInfo.sdlWindow) & SDL_WINDOW_FULLSCREEN)
                  break;

               onResize(game, event->window.data1, event->window.data2);
               break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
               // Released all modifier keys when we lose focus so they don't stick.  This
               // happens with SDL2 on Windows Vista/7 with Aero themes enabled
               SDL_SetModState(KMOD_NONE);

               // Released all keys when we lose focus.  No more stickies!
               InputCodeManager::resetStates();

               // Workaround for SDL2 issue where the cursor disappears completely if you
               // CMD + TAB to another application and come back again
#ifdef TNL_OS_MAC_OSX
               Cursor::disableCursor();
               Cursor::enableCursor();
#endif
               break;

#ifdef TNL_OS_MAC_OSX
            case SDL_WINDOWEVENT_FOCUS_GAINED:
               Cursor::disableCursor();
               Cursor::enableCursor();
               break;
#endif

            default:
               break;
            }
         break;

#else
      case SDL_VIDEORESIZE:
         onResize(game, event->resize.w, event->resize.h);
         break;
#endif
      default:
         onUser(event->user.type, event->user.code, event->user.data1, event->user.data2);
         break;
   }
}


void Event::onKeyDown(ClientGame *game, SDL_Event *event)
{
   // We first disallow key-to-text translation
   mAllowTextInput = false;

   SDLKey key = event->key.keysym.sym;
   // Use InputCodeManager::getState() instead of checking the mod flag to prevent hyper annoying case
   // of user pressing and holding Alt, selecting another window, releasing Alt, returning to
   // Bitfighter window, and pressing enter, and having this block think we pressed alt-enter.
   // GetState() looks at the actual current state of the key, which is what we want.

   // ALT+ENTER --> toggles window mode/full screen
   if(key == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT))
   {
      const Point *pos = gScreenInfo.getMousePos();

      game->getUIManager()->getOptionsMenuUserInterface()->toggleDisplayMode();

      gScreenInfo.setCanvasMousePos((S32)pos->x, (S32)pos->y, game->getSettings()->getIniSettings()->displayMode);

#if SDL_VERSION_ATLEAST(2,0,0)
      SDL_WarpMouseInWindow(gScreenInfo.sdlWindow, (S32)gScreenInfo.getWindowMousePos()->x, (S32)gScreenInfo.getWindowMousePos()->y);
#else
      SDL_WarpMouse(gScreenInfo.getWindowMousePos()->x, gScreenInfo.getWindowMousePos()->y);
#endif
   }
#ifndef BF_NO_SCREENSHOTS
   // CTRL+Q --> screenshot!
   else if(key == SDLK_q && InputCodeManager::getState(KEY_CTRL))
      ScreenShooter::saveScreenshot(game->getUIManager(), game->getSettings());
#endif
   // The rest
   else
   {
      InputCode inputCode = InputCodeManager::sdlKeyToInputCode(key);

      // If an input code is not handled by a UI, then we will allow text translation to pass through
      mAllowTextInput = !inputCodeDown(game->getUIManager()->getCurrentUI(), inputCode);

      // SDL 1.2 has the translated key along with the keysym, so we trigger text input from here
#if !SDL_VERSION_ATLEAST(2,0,0)
      if(mAllowTextInput)
         onTextInput(game->getUIManager()->getCurrentUI(), InputCodeManager::keyToAscii(event->key.keysym.unicode, inputCode));
#endif
   }
}


void Event::onKeyUp(UserInterface *currentUI, SDL_Event *event)
{
   inputCodeUp(currentUI, InputCodeManager::sdlKeyToInputCode(event->key.keysym.sym));
}


void Event::onTextInput(UserInterface *currentUI, char unicode)
{
   if(currentUI)
      currentUI->onTextInput(unicode);
}


void Event::onMouseMoved(UserInterface *currentUI, S32 x, S32 y, DisplayMode mode)
{
   setMousePos(currentUI, x, y, mode);

   if(currentUI)
      currentUI->onMouseMoved();
}


void Event::onMouseWheel(UserInterface *currentUI, bool up, bool down)
{
   if(up)
   {
      inputCodeDown(currentUI, MOUSE_WHEEL_UP);
      inputCodeUp(currentUI, MOUSE_WHEEL_UP);
   }

   if(down)
   {
      inputCodeDown(currentUI, MOUSE_WHEEL_DOWN);
      inputCodeUp(currentUI, MOUSE_WHEEL_DOWN);
   }
}


void Event::onMouseButtonDown(UserInterface *currentUI, S32 x, S32 y, InputCode inputCode, DisplayMode mode)
{
   setMousePos(currentUI, x, y, mode);

   inputCodeDown(currentUI, inputCode);
}


void Event::onMouseButtonUp(UserInterface *currentUI, S32 x, S32 y, InputCode inputCode, DisplayMode mode)
{
   setMousePos(currentUI,x, y, mode);

   inputCodeUp(currentUI, inputCode);
}


void Event::onJoyAxis(ClientGame *game, U8 whichJoystick, U8 axis, S16 value)
{
//   logprintf("SDL Axis number: %u, value: %d", axis, value);

   if(axis < Joystick::rawAxisCount)
      Joystick::rawAxis[axis] = (F32)value / (F32)S16_MAX;

   // Left/Right movement axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].moveAxesSdlIndex[0])
      updateJoyAxesDirections(game, MoveAxisLeftRightMask,  value);

   // Up/down movement axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].moveAxesSdlIndex[1])
      updateJoyAxesDirections(game, MoveAxisUpDownMask,  value);

   // Left/Right shooting axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].shootAxesSdlIndex[0])
      updateJoyAxesDirections(game, ShootAxisLeftRightMask, value);

   // Up/down shooting axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].shootAxesSdlIndex[1])
      updateJoyAxesDirections(game, ShootAxisUpDownMask, value);
}


void Event::onJoyButtonDown(UserInterface *currentUI, U8 which, U8 button)
{
//   logprintf("SDL button down number: %u", button);
   inputCodeDown(currentUI, InputCodeManager::joystickButtonToInputCode(Joystick::remapSdlButtonToJoystickButton(button)));
   Joystick::ButtonMask |= BIT(button);
}


void Event::onJoyButtonUp(UserInterface *currentUI, U8 which, U8 button)
{
//   logprintf("SDL button up number: %u", button);
   inputCodeUp(currentUI, InputCodeManager::joystickButtonToInputCode(Joystick::remapSdlButtonToJoystickButton(button)));
   Joystick::ButtonMask = Joystick::ButtonMask & ~BIT(button);
}


// See SDL_Joystick.h for the SDL_HAT_* mask definitions
void Event::onJoyHat(UserInterface *currentUI, U8 which, U8 hat, U8 directionMask)
{
//   logprintf("SDL Hat number: %u, value: %u", hat, directionMask);

   InputCode inputCode;
   U32 inputCodeDownDeltaMask = directionMask & ~Joystick::HatInputCodeMask;
   U32 inputCodeUpDeltaMask = ~directionMask & Joystick::HatInputCodeMask;

   for(S32 i = 0; i < MaxHatDirections; i++)
   {
      inputCode = InputCodeManager::joyHatToInputCode(BIT(i));  // BIT(i) corresponds to a defined SDL_HAT_*  value

      // If the current hat direction is set in the inputCodeDownDeltaMask, set the input code down
      if(inputCodeDownDeltaMask & BIT(i))
         inputCodeDown(currentUI, inputCode);

      // If the current hat direction is set in the inputCodeUpDeltaMask, set the input code up
      if(inputCodeUpDeltaMask & BIT(i))
         inputCodeUp(currentUI, inputCode);
   }

   // Finally alter the global hat InputCode mask to reflect the current input code State
   Joystick::HatInputCodeMask = directionMask;
}


void Event::onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel)
{
   // TODO:  Do we need joyball?
//   logprintf("SDL Ball number: %u, relative x: %d, relative y: %d", ball, xrel, yrel);
}


// We don't need to worry about this event in fullscreen modes because it is never fired with SDL
void Event::onResize(ClientGame *game, S32 width, S32 height)
{
   IniSettings *iniSettings = game->getSettings()->getIniSettings();

   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Constrain window to correct proportions...
   if((width - canvasWidth) > (height - canvasHeight))      // Wider than taller  (is this right? mixing virtual and physical pixels)
      iniSettings->winSizeFact = max((F32) height / (F32)canvasHeight, gScreenInfo.getMinScalingFactor());
   else
      iniSettings->winSizeFact = max((F32) width / (F32)canvasWidth, gScreenInfo.getMinScalingFactor());

   S32 newWidth  = (S32)floor(canvasWidth  * iniSettings->winSizeFact + 0.5f);   // virtual * (physical/virtual) = physical, fix rounding problem
   S32 newHeight = (S32)floor(canvasHeight * iniSettings->winSizeFact + 0.5f);

#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_SetWindowSize(gScreenInfo.sdlWindow, newWidth, newHeight);
#else
   S32 flags = SDL_OPENGL | SDL_RESIZABLE;
   SDL_SetVideoMode(newWidth, newHeight, 0, flags);
#endif

   gScreenInfo.setWindowSize(newWidth, newHeight);
  
   glViewport(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());

   gConsole.onScreenResized();

   gINI.SetValueF("Settings", "WindowScalingFactor", iniSettings->winSizeFact, true);

   glScissor(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());    // See comment on identical line in main.cpp
}


void Event::onUser(U8 type, S32 code, void* data1, void* data2)
{

}

}
