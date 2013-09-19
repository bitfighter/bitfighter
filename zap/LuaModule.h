
#ifndef _LUAMODULE_H_
#define _LUAMODULE_H_

#include "LuaBase.h"

namespace Zap
{

/**
 * This file provides a LuaWrapper-like macro for registering groups of
 * static functions (called "modules") in the lua environment. The
 * classes and macros here simply build a static data structure of modules
 * and their function profiles. The actual lua registration is done in
 * LuaScriptRunner::registerLooseFunctions.
 */

/**
 * Static storage for mapping module names to profile vectors.
 * Gives the advantage of guaranteed pre-main initialization.
 */
struct LuaModuleRegistrarBase
{
   static map<string, vector<LuaStaticFunctionProfile> > &getModuleProfiles()
   {
      static map<string, vector<LuaStaticFunctionProfile> > profiles;
      return profiles;
   }
};


/**
 * Type-specific extension of the above. Performs the registration in
 * its constructor.
 */
template<class T>
struct LuaModuleRegistrar : public LuaModuleRegistrarBase
{
   LuaModuleRegistrar() { T::registerFunctions(); }
};

#define LUA_STATIC_FUNARGS_ITEM(name, profiles, profileCount)                                                                  \
LuaStaticFunctionProfile name##_profile = { #name, {profiles, profileCount }, lua_##name }; profvec.push_back(name##_profile); \

/**
 * Generates a description class for the module, and declares a global static
 * LuaModuleRegistrar<T> instance, which causes the registration to occur
 * before main (when the instance is consructed).
 */
#define GENERATE_LUA_STATIC_METHODS_TABLE(modname, table_)                                                                \
struct modname##_Profiles : public LuaBase                                                                                \
{                                                                                                                         \
   using LuaBase::LuaArgType;                                                                                             \
   static void registerFunctions()                                                                                        \
   {                                                                                                                      \
      string name(#modname);                                                                                              \
      vector<LuaStaticFunctionProfile> profvec;                                                                           \
      table_(LUA_STATIC_FUNARGS_ITEM)                                                                                     \
      LuaModuleRegistrarBase::getModuleProfiles().insert(pair<string, vector<LuaStaticFunctionProfile> >(name, profvec)); \
   }                                                                                                                      \
};                                                                                                                        \
static LuaModuleRegistrar<modname##_Profiles> luaStaticFunctionProfileRegistrar_##modname;                                \


};

#endif