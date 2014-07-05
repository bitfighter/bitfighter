//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _ENGINEERHELPER_H_
#define _ENGINEERHELPER_H_

#include "helperMenu.h"    // Parent
#include "Engineerable.h"  // For EngineerBuildObjects enum

using namespace TNL;

namespace Zap
{

class Ship;

class EngineerHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   const char *getCancelMessage() const;
   S32 mSelectedIndex;

   const S32 mEngineerItemsDisplayWidth;
   S32 mEngineerButtonsWidth;

   S32 getWidthOfItems() const;
   void exitHelper();

public:
   explicit EngineerHelper();    // Constructor
   virtual ~EngineerHelper();    // Destructor

   HelperMenuType getType();

   void setSelectedEngineeredObject(U32 objectType);

   void onActivated();
   bool processInputCode(InputCode inputCode);   
   void render() const;                
   void renderDeploymentMarker(const Ship *ship) const;

   bool isChatDisabled() const;
   S32 getAnimationTime() const;
   static InputCode getInputCodeForOption(EngineerBuildObjects obj, bool keyBut);  // For testing
   bool isMenuBeingDisplayed() const;     // Used internally, public for testing
};

};

#endif


