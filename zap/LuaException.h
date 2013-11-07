//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUAEXCEPTION_H_
#define _LUAEXCEPTION_H_

#include <string>

// From http://stackoverflow.com/questions/134569/c-exception-throwing-stdstring

struct LuaException : public std::exception
{
   std::string msg;

   explicit LuaException(std::string str) : msg(str) { /* do nothing */ }    // Constructor
   virtual ~LuaException() throw()                   { /* do nothing */ }    // Destructor, required by gcc to avoid "looser throw" error
   const char* what() const throw()         { return msg.c_str(); }
};


#endif
