//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_YES_NO_H_
#define _UI_YES_NO_H_

#include "UIErrorMessage.h"

namespace Zap
{

class YesNoUserInterface : public AbstractMessageUserInterface
{
   typedef AbstractMessageUserInterface Parent;

private:
   void (*mYesFunction)(ClientGame *game);
   void (*mNoFunction)(ClientGame *game);

public:
   explicit YesNoUserInterface(ClientGame *game);      // Constructor
   virtual ~YesNoUserInterface();

   void reset();
   bool onKeyDown(InputCode inputCode);
   void registerYesFunction(void(*ptr)(ClientGame *));
   void registerNoFunction(void(*ptr)(ClientGame *));
};

}

#endif


