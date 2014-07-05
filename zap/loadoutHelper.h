//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LOADOUTSELECT_H_
#define _LOADOUTSELECT_H_

#include "helperMenu.h"
#include "shipItems.h"
#include "WeaponInfo.h"

#include "tnlVector.h"

using namespace TNL;


namespace Zap
{

struct LoadoutItem
{
   InputCode key;            // Keyboard key used to select in loadout menu
   InputCode button;         // Controller button used to select in loadout menu
   U32 index;
   const char *text;       // Longer name used on loadout menu
   const char *help;       // An additional bit of help text, also displayed on loadout menu
   ShipModule requires;    // Item requires this module be part of loadout (used only for spy-bugs)

   // Constructors
   LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index);      // Shortcut for modules -- use info from ModuleInfos
   LoadoutItem(ClientGame *game, InputCode key, InputCode button, U32 index, const char *text, const char *help, ShipModule requires);
   virtual ~LoadoutItem();
};


////////////////////////////////////////
////////////////////////////////////////

class UIManager;

class LoadoutHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   ShipModule mModule[ShipModuleCount];   // Modules selected by user -- 2
   WeaponType mWeapon[ShipWeaponCount];   // Weapons selected by user -- 3

   S32 mCurrentIndex;
   S32 mLoadoutItemsDisplayWidth;
   S32 mLoadoutButtonsWidth;
   S32 mPresetItemsDisplayWidth;
   S32 mPresetButtonsWidth;

   Vector<OverlayMenuItem> mModuleMenuItems;
   Vector<OverlayMenuItem> mWeaponMenuItems;
   Vector<OverlayMenuItem> mPresetItems;

   bool mEngineerEnabled;
   bool mLoadoutChanged;      // Tracks if most recent loadout entry actually changed anything
   bool mShowingPresets;      // Are we in regular mode (false) or preset mode (true)?

   const char *getCancelMessage() const;
   InputCode getActivationKey();
   bool getActivationKeyClosesHelper();

   void activateTransitionFromLoadoutMenuToPresetMenu();
   void rebuildPresetItems();
   void reset();
   bool showingModules() const;
   void setShowingPresets(bool showingPresets);

public:
   explicit LoadoutHelper();                    // Constructor
   virtual ~LoadoutHelper();

   void pregameSetup(bool engineerEnabled);     // Set things up
   HelperMenu::HelperMenuType getType();

   void render() const;                
   void onActivated();  
   bool processInputCode(InputCode inputCode);   

   void activateHelp(UIManager *uiManager);  // Open help to an appropriate page
   void onWidgetClosed();

   // For testing:
   static InputCode getInputCodeForWeaponOption(WeaponType index, bool keyBut);
   static InputCode getInputCodeForModuleOption(ShipModule index, bool keyBut);
};

};

#endif


