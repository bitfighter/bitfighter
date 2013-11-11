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

public:
   explicit YesNoUserInterface(ClientGame *game);      // Constructor
   virtual ~YesNoUserInterface();

   void reset();
   bool onKeyDown(InputCode inputCode);
};

}

#endif


