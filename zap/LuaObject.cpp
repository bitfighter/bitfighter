//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LuaObject.h"

#include "tnlLog.h"

#include <set>

using namespace std;
using namespace TNL;

namespace Zap
{

// Statics
static set<LuaObject *> mPotentiallyUntrackedObjects;

// LuaObject is a parent class for all objects that can be instantiated by Lua.  It currently serves as a repository
// for memory leak control when objects are constructed by not used in Lua.  See Issue 308: Lua memory leak.

// Constructor
LuaObject::LuaObject()
{
   // Do nothing
}


LuaObject::~LuaObject()
{
   untrackThisItem();   
}


void LuaObject::trackThisItem()
{
   mPotentiallyUntrackedObjects.insert(this);
}


void LuaObject::untrackThisItem()
{
   if(mPotentiallyUntrackedObjects.find(this) != mPotentiallyUntrackedObjects.end())
      mPotentiallyUntrackedObjects.erase(this);
}



void LuaObject::eraseAllPotentiallyUntrackedObjects()
{
   // Create a temporary copy of our set... as we delete untracked items, it will change the contents
   // of mPotentiallyUntrackedObjects, which would cause crashes if we were iterating over it directly
   set<LuaObject *> temp = mPotentiallyUntrackedObjects;


   logprintf("Cleaning up %d untracked objects!",mPotentiallyUntrackedObjects.size());//xyzzy

   for(set<LuaObject *>::iterator it = temp.begin(); it != temp.end(); it++)
      delete *it;

   mPotentiallyUntrackedObjects.clear();
}


};

