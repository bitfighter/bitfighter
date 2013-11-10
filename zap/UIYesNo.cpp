//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Renders a simple message box, waits for the user to hit Y or N, and runs a registered function

#include "UIYesNo.h"

#include <stdio.h>

namespace Zap
{

// Constructor
YesNoUserInterface::YesNoUserInterface(ClientGame *game) : Parent(game)
{
   mYesFunction = NULL;
   mNoFunction = NULL;
}


// Destructor
YesNoUserInterface::~YesNoUserInterface()
{
   // Do nothing
}


void YesNoUserInterface::registerYesFunction(void(*ptr)(ClientGame *game))
{
   mYesFunction = ptr;
}


void YesNoUserInterface::registerNoFunction(void(*ptr)(ClientGame *game))
{
   mNoFunction = ptr;
}


void YesNoUserInterface::reset()
{
   Parent::reset();

   mYesFunction = NULL;
   mNoFunction = NULL;
}


bool YesNoUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;
/*
   if(inputCode == KEY_Y)
   {
      if(mYesFunction)
         mYesFunction(getGame());
      else
         quit();
   }
   else if(inputCode == KEY_N)
   {
      if(mNoFunction)
         mNoFunction(getGame());
      else
         quit();
   }
   else if(inputCode == KEY_ESCAPE)
      quit();
   else
      return false;
*/
   // A key was handled
   return false;
}


};


