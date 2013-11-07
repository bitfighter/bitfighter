//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EVENT_KEY_DEFS_H
#define _EVENT_KEY_DEFS_H

#include "../zap/Event.h"
#include "SDL.h"


#if SDL_VERSION_ATLEAST(2,0,0)
#else
   typedef S32 SDL_Keycode;
#endif

// Define a big block of code that we can plunk into any test and get all these key events defined.  Gross yet fascinating.

#define DEFINE_KEYS_AND_EVENTS(clientSettings)                                                                             \
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
InputCode LOADOUT_KEY_BOOST  = LoadoutHelper::getInputCodeForModuleOption(ModuleBoost,    true);                           \
InputCode LOADOUT_KEY_SHIELD = LoadoutHelper::getInputCodeForModuleOption(ModuleShield,   true);                           \
InputCode LOADOUT_KEY_REPAIR = LoadoutHelper::getInputCodeForModuleOption(ModuleRepair,   true);                           \
InputCode LOADOUT_KEY_SENSOR = LoadoutHelper::getInputCodeForModuleOption(ModuleSensor,   true);                           \
InputCode LOADOUT_KEY_CLOAK  = LoadoutHelper::getInputCodeForModuleOption(ModuleCloak,    true);                           \
InputCode LOADOUT_KEY_ARMOR  = LoadoutHelper::getInputCodeForModuleOption(ModuleArmor,    true);                           \
InputCode LOADOUT_KEY_ENGR   = LoadoutHelper::getInputCodeForModuleOption(ModuleEngineer, true);                           \
                                                                                                                           \
InputCode LOADOUT_KEY_PHASER = LoadoutHelper::getInputCodeForWeaponOption(WeaponPhaser, true);                             \
InputCode LOADOUT_KEY_BOUNCE = LoadoutHelper::getInputCodeForWeaponOption(WeaponBounce, true);                             \
InputCode LOADOUT_KEY_TRIPLE = LoadoutHelper::getInputCodeForWeaponOption(WeaponTriple, true);                             \
InputCode LOADOUT_KEY_BURST  = LoadoutHelper::getInputCodeForWeaponOption(WeaponBurst,  true);                             \
InputCode LOADOUT_KEY_MINE   = LoadoutHelper::getInputCodeForWeaponOption(WeaponMine,   true);                             \
InputCode LOADOUT_KEY_SEEKER = LoadoutHelper::getInputCodeForWeaponOption(WeaponSeeker, true);                             \

#endif