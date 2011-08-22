/*
 * Input.h
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */
#ifndef INPUT_H_
#define INPUT_H_

#include "tnlTypes.h"
#include "keyCode.h"

#include "SDL/SDL.h"

using namespace TNL;

namespace Zap {

class Event {
private:
   static void setMousePos(S32 x, S32 y);
   static void updateJoyAxesDirections(U32 axisMask, S16 value);

public:
   Event();
   virtual ~Event();

   static void onEvent(SDL_Event* event);

   static void onInputFocus();
   static void onInputBlur();
   static void onKeyDown(SDLKey key, SDLMod mod, U16 unicode);
   static void onKeyUp(SDLKey key, SDLMod mod, U16 unicode);
   static void onMouseFocus();
   static void onMouseBlur();
   static void onMouseMoved(S32 x, S32 y);
   static void onMouseWheel(bool Up, bool Down);  //Not implemented
   static void onMouseButtonDown(S32 x, S32 y, KeyCode keyCode);
   static void onMouseButtonUp(S32 x, S32 y, KeyCode keyCode);
   static void onJoyAxis(U8 which, U8 axis, S16 value);
   static void onJoyButtonDown(U8 which, U8 button);
   static void onJoyButtonUp(U8 which, U8 button);
   static void onJoyHat(U8 which, U8 hat, U8 value);
   static void onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel);
   static void onMinimize();
   static void onRestore();
   static void onResize(S32 w,S32 h);
   static void onExpose();
   static void onExit();
   static void onUser(U8 type, S32 code, void* data1, void* data2);
};

}
#endif /* INPUT_H_ */
