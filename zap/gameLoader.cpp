//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "gameLoader.h"
#include "stringUtils.h"

#include "tnl.h"
#include "tnlLog.h"

#include <fstream>
#include <sstream>

#include <stdio.h>

#  ifdef TNL_OS_WIN32
#     include "../other/dirent.h"        // Need local copy for Windows builds
#  else
#     include <dirent.h>        // Need standard copy for *NIXes
#  endif


using namespace TNL;


namespace Zap
{


using namespace std;



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

