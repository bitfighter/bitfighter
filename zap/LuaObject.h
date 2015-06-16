//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LUA_OBJECT_H_
#define _LUA_OBJECT_H_


namespace Zap
{

class LuaObject
{

public:
   LuaObject();               // Constructor
   virtual ~LuaObject();      // Destructor

   void trackThisItem();
   void untrackThisItem();
   static void eraseAllPotentiallyUntrackedObjects();
   
};

};

#endif


