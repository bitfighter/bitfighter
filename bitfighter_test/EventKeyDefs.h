#ifndef _EVENT_KEY_DEFS_H
#define _EVENT_KEY_DEFS_H


// Define a big block of code that we can plunk into any test and get all these key events defined.  Gross yet fascinating.

#define DEFINE_SDL_EVENTS(clientSettings)                                                                                  \
SDL_Keycode keyDown = InputCodeManager::inputCodeToSDLKey(                                                                 \
            clientSettings->getInputCodeManager()->getBinding(InputCodeManager::BINDING_DOWN, InputModeKeyboard));         \
                                                                                                                           \
SDL_Event EventDownPressed;                                                                                                \
EventDownPressed.type = SDL_KEYDOWN;                                                                                       \
EventDownPressed.key.keysym.sym = keyDown;                                                                                 \
                                                                                                                           \
SDL_Event EventDownReleased;                                                                                               \
EventDownReleased.type = SDL_KEYUP;                                                                                        \
EventDownReleased.key.keysym.sym = keyDown;                                                                                \
                                                                                                                           \
InputCode KEY_MOD1 = clientSettings->getInputCodeManager()->getBinding(InputCodeManager::BINDING_MOD1, InputModeKeyboard); \

#endif