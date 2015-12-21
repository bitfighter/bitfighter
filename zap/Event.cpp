//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Event.h"

#include "Console.h"
#include "UIManager.h"
#include "UIMenus.h"       // ==> Could be refactored out with some work
#include "IniFile.h"
#include "DisplayManager.h"
#include "Joystick.h"
#include "ClientGame.h"
#include "Cursor.h"

#include "SDL.h"

#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
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

   // If we are in the editor, we want to tell setMousePos that we are running in fullscreen stretched mode because it 
   // will convert the mouse coordinate assuming no black bars at the margins.  
   if(currentUI->usesEditorScreenMode() && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   DisplayManager::getScreenInfo()->setMousePos(x, y, reportedDisplayMode);
}


// Needs to be Aligned with JoystickAxesDirections... X-Macro?
static JoystickStaticDataStruct JoystickInputData[JoystickAxesDirectionCount] = {
   // Movement axes
   { JoystickMoveAxesLeft,   MoveAxesLeftMask,   STICK_1_LEFT  },
   { JoystickMoveAxesRight,  MoveAxesRightMask,  STICK_1_RIGHT },
   { JoystickMoveAxesUp,     MoveAxesUpMask,     STICK_1_UP    },
   { JoystickMoveAxesDown,   MoveAxesDownMask,   STICK_1_DOWN  },
   // Shooting axes
   { JoystickShootAxesLeft,  ShootAxesLeftMask,  STICK_2_LEFT  },
   { JoystickShootAxesRight, ShootAxesRightMask, STICK_2_RIGHT },
   { JoystickShootAxesUp,    ShootAxesUpMask,    STICK_2_UP    },
   { JoystickShootAxesDown,  ShootAxesDownMask,  STICK_2_DOWN  },
   // Triggers?
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

   const Vector<UserInterface *> *uis = currentUI->getUIManager()->getPrevUIs();
   for(S32 i = 0; i < uis->size(); i++)
      uis->get(i)->onKeyUp(inputCode);
}


bool Event::inputCodeDown(UserInterface *currentUI, InputCode inputCode)
{
   InputCodeManager::setState(inputCode, true);

   if(currentUI)
      return currentUI->onKeyDown(inputCode);

   return false;
}


void Event::onEvent(ClientGame *game, SDL_Event *event)
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

      case SDL_TEXTINPUT:
         if(mAllowTextInput)
            onTextInput(currentUI, event->text.text[0]);
         break;

      case SDL_MOUSEMOTION:
         onMouseMoved(currentUI, event->motion.x, event->motion.y, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_LEFT, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_RIGHT, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonDown(currentUI, event->button.x, event->button.y, MOUSE_MIDDLE, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;
         }
         break;

      case SDL_MOUSEWHEEL:
         if(event->wheel.y > 0)
            onMouseWheel(currentUI, true, false);
         else
            onMouseWheel(currentUI, false, true);

         break;

      case SDL_MOUSEBUTTONUP:
         switch(event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_LEFT, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_RIGHT, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonUp(currentUI, event->button.x, event->button.y, MOUSE_MIDDLE, iniSettings->mSettings.getVal<DisplayMode>("WindowMode"));
               break;
         }
         break;

      case SDL_CONTROLLERBUTTONDOWN:
         onControllerButtonDown(currentUI, event->cbutton.which, event->cbutton.button);
         break;

      case SDL_CONTROLLERBUTTONUP:
         onControllerButtonUp(currentUI, event->cbutton.which, event->cbutton.button);
         break;

      case SDL_CONTROLLERAXISMOTION:
         onControllerAxis(game, event->caxis.which, event->caxis.axis, event->caxis.value);
         break;

      // TODO Use these methods to trigger controller loading/unloading
      case SDL_CONTROLLERDEVICEADDED:
         onControllerAdded(event->cdevice.which);
         break;

      case SDL_CONTROLLERDEVICEREMOVED:
         onControllerRemoved(event->cdevice.which);
         break;

      case SDL_SYSWMEVENT:
         //Ignore
         break;

      case SDL_WINDOWEVENT:
         switch (event->window.event) {
            // This event should only be triggered in windowed mode.  SDL 2.0, however,
            // triggers this on any window change.  We have therefore flushed window events
            // in VideoSystem::actualizeScreenMode so this is only triggered by manual
            // resizing of a window
            case SDL_WINDOWEVENT_RESIZED:
               // Ignore window resize events if we are in fullscreen mode.  This actually does
               // occur when you ALT-TAB away and back to the window
               if(SDL_GetWindowFlags(DisplayManager::getScreenInfo()->sdlWindow) & SDL_WINDOW_FULLSCREEN)
                  break;

               onResize(game, event->window.data1, event->window.data2);
               break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
               // Released all keys when we lose focus.  No more stickies!
               InputCodeManager::resetStates();
               break;

            default:
               break;
            }
         break;

      default:
         onUser(event->user.type, event->user.code, event->user.data1, event->user.data2);
         break;
   }
}


void Event::onKeyDown(ClientGame *game, SDL_Event *event)
{
   // We first disallow key-to-text translation
   mAllowTextInput = false;

   SDL_Keycode key = event->key.keysym.sym;
   // Use InputCodeManager::getState() instead of checking the mod flag to prevent hyper annoying case
   // of user pressing and holding Alt, selecting another window, releasing Alt, returning to
   // Bitfighter window, and pressing enter, and having this block think we pressed alt-enter.
   // GetState() looks at the actual current state of the key, which is what we want.

   // ALT+ENTER --> toggles window mode/full screen
   if(key == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT))
   {
      const Point *pos = DisplayManager::getScreenInfo()->getMousePos();

      game->getUIManager()->getUI<OptionsMenuUserInterface>()->toggleDisplayMode();

      DisplayManager::getScreenInfo()->setCanvasMousePos((S32)pos->x, (S32)pos->y, game->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"));

      SDL_WarpMouseInWindow(DisplayManager::getScreenInfo()->sdlWindow, (S32)DisplayManager::getScreenInfo()->getWindowMousePos()->x, (S32)DisplayManager::getScreenInfo()->getWindowMousePos()->y);
   }
   // The rest
   else
   {
      InputCode inputCode = InputCodeManager::sdlKeyToInputCode(key);

      // If an input code is not handled by a UI, then we will allow text translation to pass through
      mAllowTextInput = !inputCodeDown(game->getUIManager()->getCurrentUI(), inputCode);
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


void Event::onControllerAxis(ClientGame *game, U8 deviceId, U8 axis, S16 value)
{
//   logprintf("SDL Axis number: %u, value: %d", axis, value);
   if(axis == SDL_CONTROLLER_AXIS_INVALID)
      return;

   S16 oldValue = Joystick::axesValues[axis];
   Joystick::axesValues[axis] = value;

   // Axis has changed direction
//   bool changedDirection = (value ^ oldValue) < 0;  // Uses signed bit for trickery

   switch((SDL_GameControllerAxis)axis)
   {
      // Left/Right movement axis
      case SDL_CONTROLLER_AXIS_LEFTX:
         updateJoyAxesDirections(game, MoveAxisLeftRightMask,  value);
         break;

      // Up/down movement axis
      case SDL_CONTROLLER_AXIS_LEFTY:
         updateJoyAxesDirections(game, MoveAxisUpDownMask,  value);
         break;

      // Left/Right shooting axis
      case SDL_CONTROLLER_AXIS_RIGHTX:
         updateJoyAxesDirections(game, ShootAxisLeftRightMask, value);
         break;

      // Up/down shooting axis
      case SDL_CONTROLLER_AXIS_RIGHTY:
         updateJoyAxesDirections(game, ShootAxisUpDownMask, value);
         break;

      // Left trigger
      case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
         // TODO
         break;

      // Right trigger
      case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
         // TODO
         break;
   }
}


void Event::onControllerButtonDown(UserInterface *currentUI, U8 deviceId, U8 button)
{
//   logprintf("SDL button down number: %u", button);
   inputCodeDown(currentUI, InputCodeManager::sdlControllerButtonToInputCode(button));
   Joystick::ButtonMask |= BIT(button);
}


void Event::onControllerButtonUp(UserInterface *currentUI, U8 deviceId, U8 button)
{
//   logprintf("SDL button up number: %u", button);
   inputCodeUp(currentUI, InputCodeManager::sdlControllerButtonToInputCode(button));
   Joystick::ButtonMask = Joystick::ButtonMask & ~BIT(button);
}


void Event::onControllerAdded(S32 deviceId)
{
//   const char *joystickName = SDL_GameControllerNameForIndex(deviceId);

//   logprintf("Found %s", joystickName);
}


void Event::onControllerRemoved(S32 deviceId)
{
//   const char *joystickName = SDL_GameControllerNameForIndex(deviceId);

//   logprintf("Removing %s", joystickName);
}


// TODO - make the change required here since we've moved to SDL 2
// This method should never be run in fullscreen mode, impossible with SDL 1.2, but probable 
// with SDL 2.0.  It is used to adjust window settings when resizing a windowed-window.
// This can be re-engineered when we move to SDL 2.0-only; we can then make use of the
// SDL_WINDOWEVENT_SIZE_CHANGED and merge this and VideoSystem::actualizeScreenMode
void Event::onResize(ClientGame *game, S32 width, S32 height)
{
   IniSettings *iniSettings = game->getSettings()->getIniSettings();

   S32 canvasHeight = DisplayManager::getScreenInfo()->getGameCanvasHeight();
   S32 canvasWidth = DisplayManager::getScreenInfo()->getGameCanvasWidth();

   // Constrain window to correct proportions...
   if((width - canvasWidth) > (height - canvasHeight))      // Wider than taller  (is this right? mixing virtual and physical pixels)
      iniSettings->winSizeFact = max((F32) height / (F32)canvasHeight, DisplayManager::getScreenInfo()->getMinScalingFactor());
   else
      iniSettings->winSizeFact = max((F32) width / (F32)canvasWidth, DisplayManager::getScreenInfo()->getMinScalingFactor());

   S32 newWidth  = (S32)floor(canvasWidth  * iniSettings->winSizeFact + 0.5f);   // virtual * (physical/virtual) = physical, fix rounding problem
   S32 newHeight = (S32)floor(canvasHeight * iniSettings->winSizeFact + 0.5f);

   SDL_SetWindowSize(DisplayManager::getScreenInfo()->sdlWindow, newWidth, newHeight);

   // Flush window events because SDL2 triggers another resize event with SDL_SetWindowSize
   SDL_FlushEvent(SDL_WINDOWEVENT);

   DisplayManager::getScreenInfo()->setWindowSize(newWidth, newHeight);
  
   glViewport(0, 0, DisplayManager::getScreenInfo()->getWindowWidth(), DisplayManager::getScreenInfo()->getWindowHeight());

   gConsole.onScreenResized();

   GameSettings::iniFile.SetValueF("Settings", "WindowScalingFactor", iniSettings->winSizeFact, true);

   glScissor(0, 0, DisplayManager::getScreenInfo()->getWindowWidth(), DisplayManager::getScreenInfo()->getWindowHeight());    // See comment on identical line in main.cpp
}


void Event::onUser(U8 type, S32 code, void* data1, void* data2)
{
   // Do nothing
}

}
