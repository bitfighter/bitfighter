//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LuaObject.h"

#include "tnlLog.h"
#include "LuaScriptRunner.h"

#include <map>
#include <set>

using namespace std;
using namespace TNL;

namespace Zap
{

// Statics
static map<string, set<LuaObject *> > mPotentiallyUntrackedObjects;

// LuaObject is a parent class for all objects that can be instantiated by Lua.  It currently serves as a repository
// for memory leak control when objects are constructed by not used in Lua.  See Issue 308: Lua memory leak.

// If scripts create objects, then add them to a game in short order, the list of untracked objects should stay short 
// (on the order of a single object), though I am using a set to track them, which is supposed to perform well even with 
// larger groups.  Finally, this solution is nice because scripts get cleaned up immediately after they are executed, so 
// lingering objects won't linger for long.
//
// An easy way to verify this is working is to create a script (levelgen, bot, or plugin), and simply add a line like this 
// somewhere that will get run at least once:
//
//    WallItem.new()
//
// That will create an object and forget about it, which formerly would have been a memory leak.  If you activate 
// LogLuaObjectLifecycle logging, or change the log line in eraseAllPotentiallyUntrackedObjects() below, you should see a 
// message about how many untracked objects were cleaned up.  Normally, this should be 0, but if you ran a line like the one 
// above, it will be 1 (or whatever number of times the above code was executed).
//
// Finally Lua commands executed from the console are not part of a script that terminates, so their lost objects linger 
// until Bitfighter exits.  Yes, it is possible to shoot yourself in the foot with the console.  Or with a gun.  Be careful.

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
   lua_State *L = LuaScriptRunner::getL();
   string scriptId = LuaScriptRunner::getScriptId(L);

   mScriptId = scriptId;

   TNLAssert(mPotentiallyUntrackedObjects[scriptId].find(this) == mPotentiallyUntrackedObjects[scriptId].end(), "Duplicate object!");

   mPotentiallyUntrackedObjects[scriptId].insert(this);
}


void LuaObject::untrackThisItem()
{
   // mScriptId could be empty if the object was created by a normal game process; the variable only gets set if
   // the object was created by Lua
   if(mScriptId.empty())
      return;

   if(mPotentiallyUntrackedObjects[mScriptId].find(this) != mPotentiallyUntrackedObjects[mScriptId].end())
      mPotentiallyUntrackedObjects[mScriptId].erase(this);
}


// This is only called once per script, in the LuaScriptRunner destructor
void LuaObject::eraseAllPotentiallyUntrackedObjects(const string &scriptId)
{
   // For a well-behaved script, there should be no untracked objects
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Cleaning up %d untracked objects for scriptId == '%s'", 
                                                 mPotentiallyUntrackedObjects[scriptId].size(), scriptId.c_str());

   // Create a temporary copy of our set... as we delete untracked items, it will change the contents
   // of mPotentiallyUntrackedObjects, which would cause crashes if we were iterating over it directly.
   // Hmmm... that doesn't really make sense, but if we iterate over mPotentiallyUntrackedObjects[scriptId] 
   // directly, we do get a crash, so I propose leaving it like this.
   set<LuaObject *> temp = mPotentiallyUntrackedObjects[scriptId];

   for(set<LuaObject *>::iterator it = temp.begin(); it != temp.end(); it++)
      delete *it;

   // We don't need to track objects for this script anymore, and this id will never be reused
   mPotentiallyUntrackedObjects.erase(scriptId);
}


};

