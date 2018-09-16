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
#include "VideoSystem.h"

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
            // Happens with any size change (to/from fullscreen, window scaling)
            case SDL_WINDOWEVENT_RESIZED:
               onResized(game, event->window.data1, event->window.data2);
               break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
               // Released all keys when we lose focus.  No more stickies!
               InputCodeManager::resetStates();
               break;

            case SDL_WINDOWEVENT_MOVED:
               // Only save position if not in fullscreen
               if(!VideoSystem::isFullscreen())
                  VideoSystem::saveWindowPostion(game->getSettings());

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

      VideoSystem::updateDisplayState(game->getSettings(), VideoSystem::StateReasonToggle);

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


struct ControllerAxisInputCode {
   U8 axis;
   // InputCode for each of the positive or negative directions
   InputCode negative;
   InputCode positive;
};


static ControllerAxisInputCode JoystickInputData[SDL_CONTROLLER_AXIS_MAX] = {
   // Movement axes
   { SDL_CONTROLLER_AXIS_LEFTX,        STICK_1_LEFT,  STICK_1_RIGHT },
   { SDL_CONTROLLER_AXIS_LEFTY,        STICK_1_UP,    STICK_1_DOWN  },
   // Shooting axes
   { SDL_CONTROLLER_AXIS_RIGHTX,       STICK_2_LEFT,  STICK_2_RIGHT },
   { SDL_CONTROLLER_AXIS_RIGHTY,       STICK_2_UP,    STICK_2_DOWN  },
   // Triggers - shouldn't be negative
   { SDL_CONTROLLER_AXIS_TRIGGERLEFT,  BUTTON_TRIGGER_LEFT,      BUTTON_TRIGGER_LEFT      },
   { SDL_CONTROLLER_AXIS_TRIGGERRIGHT, BUTTON_TRIGGER_RIGHT,      BUTTON_TRIGGER_RIGHT      },
};


void Event::onControllerAxis(ClientGame *game, U8 deviceId, U8 axis, S16 value)
{
   // Set our persistent array for raw values (used by diagnostics)
   Joystick::rawAxesValues[axis] = value;

   // Update our global joystick input data, use a sensitivity threshold to take
   // care of calibration issues.  Also, normalize the input value to a floating
   // point scale of -1.0 to 1.0
   F32 currentNormalizedValue;
   S32 magnitude = abs(value);

   // Lower than threshold, zero it out
   if(magnitude < Joystick::LowerSensitivityThreshold)
      currentNormalizedValue = 0.0f;

   // Higher than threshold.  Set full throttle!
   else if(magnitude > Joystick::UpperSensitivityThreshold)
   {
      if(value < 0)
         currentNormalizedValue = -1.0;
      else
         currentNormalizedValue = 1.0;
   }

   // Otherwise we're in the goldilocks zone
   else
   {
      currentNormalizedValue = (F32)(magnitude - Joystick::LowerSensitivityThreshold) /
            (F32)(Joystick::UpperSensitivityThreshold - Joystick::LowerSensitivityThreshold);

      if(value < 0)
         currentNormalizedValue = -currentNormalizedValue;
   }

   // Save the old and set the current normalized value into the game for move
   // processing
   F32 oldNormalizedValue = game->normalizedAxesValues[axis];
   game->normalizedAxesValues[axis] = currentNormalizedValue;

   // Determine what to set the InputCode state, it is binary so set the
   // threshold at half -> 0.5
   // Set the mask if it is above the digital threshold
   static const F32 InputCodeThreshold = 0.5;

   UserInterface *currentUI = game->getUIManager()->getCurrentUI();

   // This is ugly - I attempted a large karnaugh map with tri-state data to
   // simplify it and my head exploded.  This is the result

   // Old value was zero (below the threshold)
   if(oldNormalizedValue > -InputCodeThreshold && oldNormalizedValue < InputCodeThreshold)
   {
      // Current value is negative
      if(currentNormalizedValue < -InputCodeThreshold)
         inputCodeDown(currentUI, JoystickInputData[axis].negative);

      // Current value is positive
      else if(currentNormalizedValue > InputCodeThreshold)
         inputCodeDown(currentUI, JoystickInputData[axis].positive);
   }

   // Old value was negative
   else if(oldNormalizedValue < -InputCodeThreshold)
   {
      // Current value is zero or positive
      if(currentNormalizedValue >= -InputCodeThreshold)
         inputCodeUp(currentUI, JoystickInputData[axis].negative);

      // Current value is positive
      if(currentNormalizedValue > InputCodeThreshold)
         inputCodeDown(currentUI, JoystickInputData[axis].positive);
   }

   // Old value was positive
   else if(oldNormalizedValue > InputCodeThreshold)
   {
      // Current value is zero or negative
      if(currentNormalizedValue <= InputCodeThreshold)
         inputCodeUp(currentUI, JoystickInputData[axis].positive);

      // Current value is negative
      if(currentNormalizedValue < -InputCodeThreshold)
         inputCodeDown(currentUI, JoystickInputData[axis].negative);
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


void Event::onResized(ClientGame *game, S32 width, S32 height)
{
   // Ignore window resize events if we are in fullscreen mode.  This can occur
   // when you ALT-TAB away and back to the window
   if(VideoSystem::isFullscreen())
      return;

   VideoSystem::updateDisplayState(game->getSettings(), VideoSystem::StateReasonExternalResize);
}


void Event::onUser(U8 type, S32 code, void* data1, void* data2)
{
   // Do nothing
}

}
