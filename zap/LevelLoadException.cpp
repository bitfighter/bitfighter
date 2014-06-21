//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelLoadException.h"

using namespace std;


namespace Zap
{

// Constructor
LevelLoadException::LevelLoadException(string str) : msg(str)
{
   // Do nothing
}


// Destructor, required by gcc to avoid "looser throw" error
LevelLoadException::~LevelLoadException() throw ()
{
   // Do nothing
}


const char *LevelLoadException::what() const throw ()
{
   return msg.c_str();
}


};

