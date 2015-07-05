//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUA_OBJECT_H_
#define _LUA_OBJECT_H_

#include <string>

struct lua_State;

using namespace std;

namespace Zap
{


class LuaObject
{

private:
   string mScriptId;

public:
   LuaObject();            // Constructor
   virtual ~LuaObject();   // Destructor

   void trackThisItem();
   void untrackThisItem();
   static void eraseAllPotentiallyUntrackedObjects(const string &scriptId);
   
};

};

#endif


