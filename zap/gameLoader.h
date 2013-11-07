//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAMELOADER_H_
#define _GAMELOADER_H_

#include "tnlVector.h"
#include "tnlTypes.h"
#include <string>


using namespace TNL;
using namespace std;

namespace Zap
{



////////////////////////////////////////
////////////////////////////////////////

// Thrown when the loader encounters a line that seems broken for some reason
struct LevelLoadException : public std::exception
{
   string msg;

   explicit LevelLoadException (string str);         // Constructor
   virtual ~LevelLoadException () throw();  // Destructor, required by gcc to avoid "looser throw" error
   const char* what() const throw();
};


};

#endif


