/*
 * Copyright (c) 2010-2011 Alexander Ames
 * Alexander.Ames@gmail.com
 * See Copyright Notice at the end of this file
 */

// API Summary:
//
// LuaWrapper is a library designed to help bridge the gab between Lua and
// C++. It is designed to be small (a single header file), simple, fast,
// and typesafe. It has no external dependencies, and does not need to be
// precompiled; the header can simply be dropped into a project and used
// immediately. It even supports class inheritance to a certain degree. Objects
// can be created in either Lua or C++, and passed back and forth.
//
// The main functions of interest are the following:
//  luaW_is<T>
//  luaW_to<T>
//  luaW_check<T>
//  luaW_push<T>
//  luaW_register<T>
//  luaW_extend<T, U>
//  luaW_hold<T>
//  luaW_release<T>
//  luaW_clean<T>
//
// These functions allow you to manipulate arbitrary classes just like you
// would the primitive types (e.g. numbers or strings). If you are familiar
// with the normal Lua API the behavior of these functions should be very
// intuative.
//
// For more information see the README and the comments below

#ifndef LUA_WRAPPER_H_
#define LUA_WRAPPER_H_

extern "C"
{
#include "../lua/lua-vec/src/lua.h"
#include "../lua/lua-vec/src/lauxlib.h"
}

#include <vector>
#include <map>
#include <cstring>
#include <typeinfo>  // XXX remove me when removing all the logprintf's with 'typeid'

#define LUAW_BUILDER

#define luaW_getregistry(L, s) \
     lua_getfield(L, LUA_REGISTRYINDEX, s)

#define luaW_setregistry(L, s) \
     lua_setfield(L, LUA_REGISTRYINDEX, s)

#define LUAW_POSTCTOR_KEY "__postctor"
#define LUAW_EXTENDS_KEY "__extends"
#define LUAW_STORAGE_KEY "__storage"
#define LUAW_COUNT_KEY "__counts"
#define LUAW_HOLDS_KEY "__holds"
#define LUAW_WRAPPER_KEY "LuaWrapper"


// Forward declaration
template <class T> class LuaProxy;


// These are the default allocator and deallocator. If you would prefer an
// alternative option, you may select a different function when registering
// your class.
template <typename T>
T* luaW_defaultallocator(lua_State*)
{
    return new T();
}
template <typename T>
void luaW_defaultdeallocator(lua_State*, T* obj)
{
    delete obj;
}

// The identifier function is responsible for pushing a value unique to each
// object on to the stack. Most of the time, this can simply be the address
// of the pointer, but sometimes that is not adaquate. For example, if you
// are using shared_ptr you would need to push the address of the object the
// shared_ptr represents, rather than the address of the shared_ptr itself.
template <typename T>
void luaW_defaultidentifier(lua_State* L, T* obj)
{
    logprintf("CCCCCCCCCCCCC  %p %s", obj,  typeid(T).name());
    lua_pushlightuserdata(L, obj);
}


// As above, but only to be called with proxied objects
template <typename T>
void luaW_proxiedidentifier(lua_State* L, T* obj)
{
    LuaProxy<T> *proxy = obj->getLuaProxy();
    if(!proxy)
       proxy = new LuaProxy<T>(obj);

    lua_pushlightuserdata(L, obj->getLuaProxy());
}


// This class is what is used by LuaWrapper to contain the userdata. data
// stores a pointer to the object itself, and cast is used to cast toward the
// base class if there is one and it is necessary. Rather than use RTTI and
// typid to compare types, I use the clever trick of using the cast to compare
// types. Because there is at most one cast per type, I can use it to identify
// when and object is the type I want. This is only used internally.
struct luaW_Userdata
{
    luaW_Userdata(void* vptr = NULL, luaW_Userdata (*udcast)(const luaW_Userdata&) = NULL)
        : data(vptr), cast(udcast) {}
    void* data;
    luaW_Userdata (*cast)(const luaW_Userdata&);
};

// This class cannot actually to be instantiated. It is used only hold the
// table name and other information.
template <typename T>
class LuaWrapper
{
public:
    static const char* classname;
    static void (*identifier)(lua_State*, T*);
    static T* (*allocator)(lua_State*);
    static void (*deallocator)(lua_State*, T*);
    static luaW_Userdata (*cast)(const luaW_Userdata&);
private:
    LuaWrapper();
};
template <typename T> const char* LuaWrapper<T>::classname;
template <typename T> void (*LuaWrapper<T>::identifier)(lua_State*, T*);
template <typename T> T* (*LuaWrapper<T>::allocator)(lua_State*);
template <typename T> void (*LuaWrapper<T>::deallocator)(lua_State*, T*);
template <typename T> luaW_Userdata (*LuaWrapper<T>::cast)(const luaW_Userdata&);

// Cast from an object of type T to an object of type U. This template
// function is instantiated by calling luaW_extend<T, U>(L). This is only used
// internally.
template <typename T, typename U>
luaW_Userdata luaW_cast(const luaW_Userdata& obj)
{
    return luaW_Userdata(static_cast<U*>(static_cast<T*>(obj.data)), LuaWrapper<U>::cast);
}

// Analogous to lua_is(boolean|string|*)
//
// Returns 1 if the value at the given acceptable index is of type T (or if
// strict is false, convertable to type T) and 0 otherwise.
template <typename T>
bool luaW_is(lua_State *L, int index, bool strict = false)
{
    bool equal = false;// lua_isnil(L, index);
logprintf("luaW_is for T=%s",  LuaWrapper<T>::classname);
// Here we have stack: userdata, 'getRad'
    if (!equal && lua_isuserdata(L, index) && lua_getmetatable(L, index))
    {
        // ... ud ... udmt
        luaL_getmetatable(L, LuaWrapper<T>::classname); // ... ud ... udmt Tmt
//        Zap::LuaObject::dumpStack(L, "ZYX");
//        Zap::LuaObject::dumpTable(L, -1);  // looks like udmt to me
//        Zap::LuaObject::dumpTable(L, -2);     // empty table

        equal = lua_rawequal(L, -1, -2);     // Compare udmt and Tmt
        if (!equal && !strict)
        {
            lua_getfield(L, -2, LUAW_EXTENDS_KEY); // ... ud ... udmt Tmt udmt.extends

       // Table @ -1 is item method table
            //Zap::LuaObject::dumpTable(L, -3);

            for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
            {
               //Zap::LuaObject::dumpStack(L);
 //                          Zap::LuaObject::dumpTable(L, -1);      // empty table (v)
 //Zap::LuaObject::dumpTable(L, -4);     // robot method table (Tmt)

                // ... ud ... udmt Tmt udmt.extends k v
                equal = lua_rawequal(L, -1, -4);      // Compare v and Tmt
                if (equal)
                {
                    lua_pop(L, 2); // ... ud ... udmt Tmt udmt.extends
                    break;
                }
//                Zap::LuaObject::dumpStack(L);
            }
            lua_pop(L, 1); // ... ud ... udmt Tmt

        }
        lua_pop(L, 2); // ... ud ...
    }
    return equal;
}

// Analogous to lua_to(boolean|string|*)
//
// Converts the given acceptable index to a T*. That value must be of (or
// convertable to) type T; otherwise, returns NULL.
template <typename T>
T* luaW_to(lua_State* L, int index, bool strict = false)

{
    if (luaW_is<T>(L, index, strict))
    {
        luaW_Userdata* pud = (luaW_Userdata*)lua_touserdata(L, index);
        luaW_Userdata ud;

        logprintf("luaW_to: %p, %p (T = %s)", LuaWrapper<T>::cast, pud->cast, LuaWrapper<T>::classname);

        while (!strict && LuaWrapper<T>::cast != pud->cast)
        {
            ud = pud->cast(*pud);
            pud = &ud;
        }
          LuaProxy<T> *proxy = (LuaProxy<T> *)pud->data;
        if(!proxy->isDefunct())
           return proxy->getProxiedObject();
    }
    return NULL;
}


// As above, but returns the proxy instead of the object itself
template <typename T>
LuaProxy<T>* luaW_toProxy(lua_State* L, int index, bool strict = false)
{
    if (luaW_is<T>(L, index, strict))
    {
        luaW_Userdata* pud = (luaW_Userdata*)lua_touserdata(L, index);
        luaW_Userdata ud;
        while (!strict && LuaWrapper<T>::cast != pud->cast)
        {
            ud = pud->cast(*pud);
            pud = &ud;
        }
        return (LuaProxy<T>*)pud->data;
    }
    return NULL;
}


// Analogous to luaL_check(boolean|string|*)
//
// Converts the given acceptable index to a T*. That value must be of (or
// convertable to) type T; otherwise, an error is raised.
template <typename T>
T* luaW_check(lua_State* L, int index, bool strict = false)
{
   if(strcmp(LuaWrapper<T>::classname, "Item") == 0)
      int x = 1;
    T* obj = NULL;
    if (luaW_is<T>(L, index, strict))
    {
        luaW_Userdata* pud = (luaW_Userdata*)lua_touserdata(L, index);
        luaW_Userdata ud;
        logprintf("luaW_check (T=%s), %p , %p", LuaWrapper<T>::classname, LuaWrapper<T>::cast, pud->cast);
        while (!strict && LuaWrapper<T>::cast != pud->cast)
        {
            ud = pud->cast(*pud);
            pud = &ud;
       logprintf("luaW_check (T=%s), %p , %p", LuaWrapper<T>::classname, LuaWrapper<T>::cast, pud->cast);

        }
        logprintf("Going");
        LuaProxy<T> *proxy = (LuaProxy<T>*)pud->data;
        logprintf("Back");
        if(!proxy->isDefunct())
         obj = proxy->getProxiedObject();
    }
    else
    {
        luaL_typerror(L, index, LuaWrapper<T>::classname);
    }

    return obj;
}


// Forward declaration
template <typename T>
bool luaW_hold(lua_State* L, T* obj);


// Analogous to lua_push(boolean|string|*)
//
// Pushes a userdata of type T onto the stack. If this object already exists in
// the Lua environment, it will assign the existing store to it. Otherwise, a
// new storage table will be created for it.
template <typename T>
void luaW_push(lua_State* L, T* obj)
{
    if (obj)
    {
        LuaProxy<T> *proxy = obj->getLuaProxy();
        if(!proxy)
           proxy = new LuaProxy<T>(obj);

        luaW_Userdata* ud = (luaW_Userdata*)lua_newuserdata(L, sizeof(luaW_Userdata)); // ... obj
        ud->data = proxy;

        ud->cast = LuaWrapper<T>::cast;

        logprintf("luaW_push %p  (T=%s)", ud->cast, LuaWrapper<T>::classname); 

        luaL_getmetatable(L, LuaWrapper<T>::classname); // ... obj mt
        lua_setmetatable(L, -2); // ... obj
        luaW_getregistry(L, LUAW_WRAPPER_KEY); // ... obj LuaWrapper
        lua_getfield(L, -1, LUAW_COUNT_KEY); // ... obj LuaWrapper LuaWrapper.counts
        LuaWrapper<T>::identifier(L, obj); // ... obj LuaWrapper LuaWrapper.counts id
        lua_gettable(L, -2); // ... obj LuaWrapper LuaWrapper.counts count
        int count = (int) lua_tointeger(L, -1);

           logprintf("XXXXX Pushing obj %p as %s  [[ count = %d ]]", obj, typeid(T).name(), count);

        LuaWrapper<T>::identifier(L, obj); // ... obj LuaWrapper LuaWrapper.counts count id
        lua_pushinteger(L, count+1); // ... obj LuaWrapper LuaWrapper.counts count id count+1
        lua_settable(L, -4); // ... obj LuaWrapper LuaWrapper.counts count
        lua_pop(L, 3); // ... obj

        luaW_hold<T>(L, obj);     // Tell luaW to collect the proxy when it's done with it
    }
    else
    {
        lua_pushnil(L);
    }
}


// Instructs LuaWrapper that it owns the userdata, and can manage its memory.
// When all references to the object are removed, Lua is free to garbage
// collect it and delete the object.
//
// Returns true if luaW_hold took hold of the object, and false if it was
// already held
template <typename T>
bool luaW_hold(lua_State* L, T* obj)
{
   logprintf("XXXXX Holding %s for obj %p", typeid(T).name(), obj);
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // ... LuaWrapper

    lua_getfield(L, -1, LUAW_HOLDS_KEY); // ... LuaWrapper LuaWrapper.holds
    LuaWrapper<T>::identifier(L, obj); // ... LuaWrapper LuaWrapper.holds id
    lua_rawget(L, -2); // ... LuaWrapper LuaWrapper.holds hold
    bool held = lua_toboolean(L, -1);
    // If it's not held, hold it
    if (!held)
    {
        // Apply hold boolean
        lua_pop(L, 1); // ... LuaWrapper LuaWrapper.holds
        LuaWrapper<T>::identifier(L, obj); // ... LuaWrapper LuaWrapper.holds id
        lua_pushboolean(L, true); // ... LuaWrapper LuaWrapper.holds id true
        lua_rawset(L, -3); // ... LuaWrapper LuaWrapper.holds

        // Check count, if there's at least one, add a storage table
        lua_pop(L, 1); // ... LuaWrapper
        lua_getfield(L, -1, LUAW_COUNT_KEY); // ... LuaWrapper LuaWrapper.counts
        LuaWrapper<T>::identifier(L, obj); // ... LuaWrapper LuaWrapper.counts id
        lua_rawget(L, -2); // ... LuaWrapper LuaWrapper.counts count
        if (lua_tointeger(L, -1) > 0)
        {
            // Find and attach the storage table
            lua_pop(L, 2);
            lua_getfield(L, -1, LUAW_STORAGE_KEY); // ... LuaWrapper LuaWrapper.storage
            LuaWrapper<T>::identifier(L, obj); // ... LuaWrapper LuaWrapper.storage id
            lua_rawget(L, -2); // ... LuaWrapper LuaWrapper.storage store

            // Add the storage table if there isn't one already
            if (lua_isnoneornil(L, -1))
            {
                lua_pop(L, 1); // ... LuaWrapper LuaWrapper.storage
                LuaWrapper<T>::identifier(L, obj); // ... LuaWrapper LuaWrapper.storage id
                lua_newtable(L); // ... LuaWrapper LuaWrapper.storage id store

                lua_newtable(L); // ... LuaWrapper LuaWrapper.storage id store mt storemt
                luaL_getmetatable(L, LuaWrapper<T>::classname); // ... LuaWrapper LuaWrapper.storage id store storemt mt
                lua_setfield(L, -2, "__index"); // ... LuaWrapper LuaWrapper.storage id store storemt
                lua_setmetatable(L, -2); // ... LuaWrapper LuaWrapper.storage id store

                lua_rawset(L, -3); // ... LuaWrapper LuaWrapper.storage
                lua_pop(L, 2); // ...
            }
        }
        return true;
    }
    lua_pop(L, 3); // ...
    return false;
}


// Releases LuaWrapper's hold on an object. This allows the user to remove
// all references to an object in Lua and ensure that Lua will not attempt to
// garbage collect it.
//
// This function takes the index of the identifier for an object rather than
// the object itself. This is because needs to be able to run after the object
// has already been deallocated.
template <typename T>
void luaW_release(lua_State* L, int index)
{
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // ... id ... LuaWrapper
    lua_getfield(L, -1, LUAW_HOLDS_KEY); // ... id ... LuaWrapper LuaWrapper.holds
    lua_pushvalue(L, (index>0) ? index : index-2); // ... id ... LuaWrapper LuaWrapper.holds id
    lua_pushnil(L); // ... id ... LuaWrapper LuaWrapper.holds id nil
    lua_settable(L, -3); // ... id ... LuaWrapper LuaWrapper.holds
    lua_pop(L, 2); // ... id ...
}

// When luaW_clean is called on an object, values stored on it's Lua store
// become no longer accessible.
//
// This function takes the index of the identifier for an object rather than
// the object itself. This is because needs to be able to run after the object
// has already been deallocated.
template <typename T>
void luaW_clean(lua_State* L, int index)
{
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // ... id ... LuaWrapper
    lua_getfield(L, -1, LUAW_STORAGE_KEY); // ... id ... LuaWrapper LuaWrapper.storage
    lua_pushvalue(L, (index>0) ? index : index-2); // ... id ... LuaWrapper LuaWrapper.storage id
    lua_pushnil(L); // ... id ... LuaWrapper LuaWrapper.storage id nil
    lua_settable(L, -3);  // ... id ... LuaWrapper LuaWrapper.store
    lua_pop(L, 2); // ... id ...
}

// This function is called from Lua, not C++
//
// Calls the lua post-constructor (LUAW_POSTCTOR_KEY or "__postctor") on a
// userdata. Assumes the userdata is on top of the stack, and numargs arguments
// are below it. This runs the LUAW_POSTCTOR_KEY function on T's metatable,
// using the object as the first argument and whatever else is below it as
// the rest of the arguments This exists to allow types to adjust values in
// thier storage table, which can not be created until after the constructor is
// called.
template <typename T>
void luaW_postconstructor(lua_State* L, int numargs)
{
    // ... args ud
    lua_getfield(L, -1, LUAW_POSTCTOR_KEY); // ... args ud ud.__postctor
    if (lua_type(L, -1) == LUA_TFUNCTION)
    {
        lua_pushvalue(L, -2); // ... args ud ud.__postctor ud
        lua_insert(L, -3); // ... ud args ud ud.__postctor
        lua_insert(L, -3); // ... ud.__postctor ud args ud
        lua_insert(L, -3); // ... ud ud.__postctor ud args
        lua_call(L, numargs+1, 0); // ... ud
    }
    else
    {
        lua_pop(L, 1); // ... ud
    }
}

// This function is generally called from Lua, not C++
//
// Creates an object of type T using the constructor and subsequently calls the
// post-constructor on it.
template <typename T>
inline int luaW_new(lua_State* L, int args)
{
    T* obj = LuaWrapper<T>::allocator(L);
    luaW_push<T>(L, obj);
    luaW_hold<T>(L, obj);
    luaW_postconstructor<T>(L, args);
    return 1;
}

template <typename T>
int luaW_new(lua_State* L)
{
    return luaW_new<T>(L, lua_gettop(L));
}

#ifdef LUAW_BUILDER

// This function is called from Lua, not C++
//
// This is an alternative way to construct objects. Instead of using new and a
// constructor, you can use a builder instead. A builder is called like this:
//
// f = Foo.build
// {
//     X = 10;
//     Y = 20;
// }
//
// This will then create a new Foo object, and then call f:X(10) and f:Y(20)
// on that object. The lua defined constructor is not called at any point. The
// keys in this table are used as function names on the metatable.
//
// This is sort of experimental, just to see if it ends up being useful.
template <typename T>
void luaW_builder(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TTABLE)
    {
        // {} ud
        for (lua_pushnil(L); lua_next(L, 1); lua_pop(L, 1))
        {
            // {} ud k v
            lua_pushvalue(L, -2); // {} ud k v k
            lua_gettable(L, -4); // {} ud k v ud[k]
            lua_pushvalue(L, -4); // {} ud k v ud[k] ud
            lua_pushvalue(L, -3); // {} ud k v ud[k] ud v
            lua_call(L, 2, 0); // {} ud k v
        }
        // {} ud
    }
}

// This function is generally called from Lua, not C++
//
// Creates an object of type T and initializes it using its builder to
// initialize it. Calls post constructor with 0 arguments in case special
// initialization is needed to set up special tables that can not be added
// during construction
template <typename T>
int luaW_build(lua_State* L)
{
    T* obj = LuaWrapper<T>::allocator(L);
    luaW_push<T>(L, obj);
    luaW_hold<T>(L, obj);
    luaW_postconstructor<T>(L, 0);
    luaW_builder<T>(L);
    return 1;
}

#endif

// This function is called from Lua, not C++
//
// The default metamethod to call when indexing into lua userdata representing
// an object of type T. This will first check the userdata's environment table
// and if it's not found there it will check the metatable. This is done so
// individual userdata can be treated as a table, and can hold thier own
// values.
template <typename T>
int luaW__index(lua_State* L)
{
    // obj key
    T* obj = luaW_to<T>(L, 1);
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // obj key LuaWrapper
    lua_getfield(L, -1, LUAW_STORAGE_KEY); // obj key LuaWrapper LuaWrapper.storage
    LuaWrapper<T>::identifier(L, obj); // obj key LuaWrapper LuaWrapper.storage id
    lua_rawget(L, -2); // obj key LuaWrapper LuaWrapper.storage store
    if (!lua_isnoneornil(L, -1))
    {
        lua_pushvalue(L, -4); // obj key LuaWrapper LuaWrapper.storage store key
        lua_gettable(L, -2); // obj key LuaWrapper LuaWrapper.storage store store[k]
    }
    else
    {
        lua_pop(L, 3); // obj key
        lua_getmetatable(L, -2); // obj key mt
        lua_pushvalue(L, -2); // obj key mt k
        lua_gettable(L, -2); // obj key mt mt[k]
    }
    return 1;
}

// This function is called from Lua, not C++
//
// The default metamethod to call when creating a new index on lua userdata
// representing an object of type T. This will index into the the userdata's
// environment table that it keeps for personal storage. This is done so
// individual userdata can be treated as a table, and can hold thier own
// values.
template <typename T>
int luaW__newindex(lua_State* L)
{
    // obj key value
    T* obj = luaW_to<T>(L, 1);
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // obj key value LuaWrapper
    lua_getfield(L, -1, LUAW_STORAGE_KEY); // obj key value LuaWrapper LuaWrapper.storage
    LuaWrapper<T>::identifier(L, obj); // obj key value LuaWrapper LuaWrapper.storage id
    lua_rawget(L, -2); // obj key value LuaWrapper LuaWrapper.storage store
    if (!lua_isnoneornil(L, -1))
    {
        lua_pushvalue(L, -5); // obj key value LuaWrapper LuaWrapper.storage store key
        lua_pushvalue(L, -5); // obj key value LuaWrapper LuaWrapper.storage store key value
        lua_rawset(L, -3); // obj key value LuaWrapper LuaWrapper.storage store
    }
    return 0;
}

// This function is called from Lua, not C++
//
// The __gc metamethod handles cleaning up userdata. The userdata's reference
// count is decremented and if this is the final reference to the userdata its
// environment table is nil'd and pointer deleted with the destructor callback.
template <typename T>
int luaW__gc(lua_State* L)
{
    // obj
    LuaProxy<T>* proxy = luaW_toProxy<T>(L, 1);
    T* obj = proxy->getProxiedObject();

    // If obj is NULL here, it may have been deleted from the C++ side already

    LuaWrapper<T>::identifier(L, obj); // obj id
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // obj id LuaWrapper
    lua_getfield(L, -1, LUAW_COUNT_KEY); // obj id LuaWrapper LuaWrapper.counts
    LuaWrapper<T>::identifier(L, obj); // obj id LuaWrapper LuaWrapper.counts id
    lua_gettable(L, -2); // obj id LuaWrapper LuaWrapper.counts count
    int count = lua_tointeger(L, -1);
    lua_pushvalue(L, 2); // obj id LuaWrapper LuaWrapper.counts count id
    lua_pushinteger(L, --count); // obj id LuaWrapper LuaWrapper.counts count id count-1
    lua_settable(L, -4); // obj id LuaWrapper LuaWrapper.counts count

    logprintf("XXXXX Garbage collecting obj %p of type %s (count = %d)", obj, typeid(T).name(), count);

    if (obj && 0 == count)
    {
        lua_getfield(L, 3, LUAW_HOLDS_KEY); // obj id LuaWrapper LuaWrapper.counts LuaWrapper.holds
        lua_pushvalue(L, 2); // obj id LuaWrapper LuaWrapper.counts LuaWrapper.holds id
        lua_gettable(L, -2); // obj id LuaWrapper LuaWrapper.counts LuaWrapper.holds hold
        //if (lua_toboolean(L, -1) && LuaWrapper<T>::deallocator && !proxy->isDefunct())
        //{
        //    LuaWrapper<T>::deallocator(L, obj);
        //}
        luaW_release<T>(L, 2);
        luaW_clean<T>(L, 2);
    }
    return 0;
}

// Run luaW_register to create a table and metatable for your class. This
// creates a table with the name you specify filled with the function from the
// table argument in addition to the functions new and build. This is generally
// for things you think of as static methods in C++. The metatable becomes a
// metatable for each object if your class. These can be thought of as member
// functions or methods.
//
// You may also supply constructors and destructors for classes that do not
// have a default constructor or that require special set up or tear down. You
// may specify NULL as the constructor, which means that you will not be able
// to call the new function on your class table. You will need to manually push
// objects from C++. By default, the default constructor is used to create
// objects and a simple call to delete is used to destroy them.
//
// By default LuaWrapper uses the address of C++ object to identify unique
// objects. In some cases this is not desired, such as in the case of
// shared_ptrs. Two shared_ptrs may themselves have unique locations in memory
// but still represent the same object. For cases like that, you may specify an
// identifier function which is responsible for pushing a key representing your
// object on to the stack.
template <typename T>
void luaW_register(lua_State* L, const char* classname, const luaL_reg* table, const luaL_reg* metatable, T* (*allocator)(lua_State*) = luaW_defaultallocator<T>, void (*deallocator)(lua_State*, T*) = luaW_defaultdeallocator<T>, void (*identifier)(lua_State*, T*) = luaW_defaultidentifier<T>)
{
   logprintf("XXXXX luaW_register: %s", typeid(T).name());
    LuaWrapper<T>::classname = classname;
    LuaWrapper<T>::identifier = identifier;
    LuaWrapper<T>::allocator = allocator;
    LuaWrapper<T>::deallocator = deallocator;

    const luaL_reg defaulttable[] =
    {
        { "new", luaW_new<T> },
#ifdef LUAW_BUILDER
        { "build", luaW_build<T> },
#endif
        { NULL, NULL }
    };
    const luaL_reg defaultmetatable[] = { { "__index", luaW__index<T> }, { "__newindex", luaW__newindex<T> }, { "__gc", luaW__gc<T> }, { NULL, NULL } };
    const luaL_reg emptytable[] = { { NULL, NULL } };

    table = table ? table : emptytable;
    metatable = metatable ? metatable : emptytable;

    // Ensure that the LuaWrapper table is set up
    luaW_getregistry(L, LUAW_WRAPPER_KEY); // LuaWrapper
    if (lua_isnil(L, -1))
    {
        lua_newtable(L); // nil {}
        luaW_setregistry(L, LUAW_WRAPPER_KEY); // nil
        luaW_getregistry(L, LUAW_WRAPPER_KEY); // nil LuaWrapper
        lua_newtable(L); // nil LuaWrapper {}
        lua_setfield(L, -2, LUAW_COUNT_KEY); // nil LuaWrapper
        lua_newtable(L); // nil LuaWrapper {}
        lua_setfield(L, -2, LUAW_STORAGE_KEY); // nil LuaWrapper
        lua_newtable(L); // nil LuaWrapper {}
        lua_setfield(L, -2, LUAW_HOLDS_KEY); // nil LuaWrapper
        lua_pop(L, 1); // nil
    }
    lua_pop(L, 1); //

    // Open table
    if (allocator)
    {
        luaL_register(L, LuaWrapper<T>::classname, defaulttable); // T
        luaL_register(L, NULL, table); // T
    }
    else
    {
        luaL_register(L, LuaWrapper<T>::classname, table); // T
    }

    // Open metatable, set up extends table
    luaL_newmetatable(L, LuaWrapper<T>::classname); // T mt
    lua_newtable(L); // T mt {}
    lua_setfield(L, -2, LUAW_EXTENDS_KEY); // T mt
    luaL_register(L, NULL, defaultmetatable); // T mt
    luaL_register(L, NULL, metatable); // T mt
    lua_setfield(L, -2, "metatable"); // T
}

// luaW_extend is used to declare that class T inherits from class U. All
// functions in the base class will be available to the derived class (except
// when they share a function name, in which case the derived class's function
// wins). This also allows luaW_to<T> to cast your object apropriately, as
// casts straight through a void pointer do not work.
template <typename T, typename U>
void luaW_extend(lua_State* L)
{
    if(!LuaWrapper<T>::classname)
        luaL_error(L, "attempting to call extend on a type that has not been registered");

    if(!LuaWrapper<U>::classname)
        luaL_error(L, "attempting to extend %s by a type that has not been registered", LuaWrapper<T>::classname);

    LuaWrapper<T>::cast = luaW_cast<T, U>;

    luaL_getmetatable(L, LuaWrapper<T>::classname); // mt
    luaL_getmetatable(L, LuaWrapper<U>::classname); // mt emt

    // Point T's metatable __index at U's metatable for inheritance
    lua_newtable(L); // mt emt {}
    lua_pushvalue(L, -2); // mt emt {} emt
    lua_setfield(L, -2, "__index"); // mt emt {}
    lua_setmetatable(L, -3); // mt emt

    // Make a list of al types that inherit from U, for type checking
    lua_getfield(L, -2, LUAW_EXTENDS_KEY); // mt emt mt.extends
    lua_pushvalue(L, -2); // mt emt mt.extends emt
    lua_setfield(L, -2, LuaWrapper<U>::classname); // mt emt mt.extends
    lua_getfield(L, -2, LUAW_EXTENDS_KEY); // mt emt mt.extends emt.extends
    for (lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1))
    {
        // mt emt mt.extends emt.extends k v
        lua_pushvalue(L, -2); // mt emt mt.extends emt.extends k v k
        lua_pushvalue(L, -2); // mt emt mt.extends emt.extends k v k
        lua_rawset(L, -6); // mt emt mt.extends emt.extends k v
    }

    lua_pop(L, 4); // mt emt
}

#undef luaW_getregistry
#undef luaW_setregistry



// Class to facilitate the semi-autonomous self-registration of LuaW classes.
// To use this system, classes must implement the following:
//    class name:  const char *luaClassName
//    method list: const luaL_reg Item::luaMethods[] = { }
// Then, somewhere in the class definition (.cpp file), add the line:
//    REGISTER_LUA_CLASS(className);
// or
//    REGISTER_LUA_SUBCLASS(className, parentClass);
// When you actually create your Lua instance (L), call LuaW_Registrar::registerClasses(L)
// to make your methods available.
// And with that, your class will be registered with LuaWrapper.
//
// A quick overview of how the registration system works is REGISTER_LUA_CLASS and REGISTER_LUA_SUBCLASS 
// are actually dummy declarations that during initialization, build lists of registration functions 
// called registrationFunctions and extensionFunctions.  When you actually create your L object and 
// want to register your methods with it, calling registerClass() uses those lists to call the 
// appropriate LuaW registration/extension methods.  
class LuaW_Registrar
{
private:
   typedef void (*luaW_regFunc)(lua_State *);

   struct classParent { 
      const char *name;   
      const char *parent; 

      bool isTopLevel() { return parent == NULL; }
   };

   typedef const char* functionName;
   typedef std::pair<functionName, luaW_regFunc> keyValue;
   typedef std::map <functionName, luaW_regFunc> functionMap;    // Map of function name and functions

   // List of registration functions
   static functionMap &getRegistrationFunctions()
   {
      static functionMap registrationFunctions;
      return registrationFunctions;
   }

   // List of extension functions
   static functionMap &getExtensionFunctions()
   {
      static functionMap extensionFunctions;
      return extensionFunctions;
   }

   // Unordered list of classes
   static std::vector<classParent> &getPreorderedClassList()
   {
      static std::vector<classParent> preorderedClassList;
      return preorderedClassList;
   }

   // Return vector of classes where parents of each class are listed before their children.  List is built as-needed.
   static std::vector<functionName> &getOrderedClassList()
   {
      static std::vector<functionName> orderedClassList;

      unsigned int startingSize, currentSize;
      startingSize = currentSize = getPreorderedClassList().size();

      // Pass 1: First grab all top level objects, we do this only once.  Go backwards.
      for(int i = (int)getPreorderedClassList().size() - 1; i >= 0; i--)   
      {
         // If this is a top-level object, add to ordered list and remove from pre-ordered one
         if(getPreorderedClassList()[i].isTopLevel())
         {
            orderedClassList.push_back(getPreorderedClassList()[i].name);
            getPreorderedClassList().erase(getPreorderedClassList().begin() + i);
         }
      }

      currentSize = getPreorderedClassList().size();     // Remaining items after all top-level items have been processed

      // Pass 2: Go through the remaining objects -- these should all have parents that are in orderedClassList
      unsigned int iteration = 0;
      while(currentSize > 0)
      {
         iteration++;

         for(int i = (int)getPreorderedClassList().size() - 1; i >= 0; i--)
         {
            bool foundParent = false;

            // Search ordered list to see if parent is already there
            for(unsigned int j = 0; j < orderedClassList.size(); j++)
               if(strcmp(orderedClassList[j], getPreorderedClassList()[i].parent) == 0)
                  foundParent = true;

            // If parent is already found, add to ordered list and remove from pre-ordered list, as before
            if(foundParent)
            {
               orderedClassList.push_back(getPreorderedClassList()[i].name);
               getPreorderedClassList().erase(getPreorderedClassList().begin() + i);
            }
         }

         // For safety if objects have no found parents and we've iterated too many times,
         // just add them to the end of the list.  This block should nevever run.
         TNLAssert(iteration <= startingSize, "Item appears to have invalid parent!");
         if(iteration > startingSize)
         {
            for(int i = (int)getPreorderedClassList().size() - 1; i > -1; i--)
            {
               orderedClassList.push_back(getPreorderedClassList()[i].name);
               getPreorderedClassList().erase(getPreorderedClassList().begin() + i);
            }
         }

         currentSize = getPreorderedClassList().size();     // Items still remaining in the list
      }

      // We allow roughly, roughly monkey boy
      TNLAssert(orderedClassList.size() == startingSize, "Ordered list is different size than pre-ordered list!");

      return orderedClassList;
   }

protected:
   template<class T>
   static void registerClass(lua_State *L)
   {
      luaW_register<T>(L, T::luaClassName, NULL, T::luaMethods);
      lua_pop(L, 1);                            // Remove metatable from stack
   }

   template<class T>
   void static registerClass()
   {
      classParent key = {T::luaClassName, NULL};
      getPreorderedClassList().push_back(key);

      keyValue pair1(T::luaClassName, &registerClass<T>);
      getRegistrationFunctions().insert(pair1);
   }

   template<class T, class U>
   static void registerClass()
   {
      classParent key = {T::luaClassName, U::luaClassName};
      getPreorderedClassList().push_back(key);

      keyValue pair1(T::luaClassName, &registerClass<T>);
      getRegistrationFunctions().insert(pair1);

      keyValue pair2(T::luaClassName, &luaW_extend<T, U>);
      getExtensionFunctions()   .insert(pair2);
   }

public:
   static void registerClasses(lua_State *L)
   {
      std::vector<const char*> orderedClassList = getOrderedClassList();

      // Register all our classes
      for(unsigned int i = 0; i < orderedClassList.size(); i++)
         getRegistrationFunctions()[orderedClassList[i]](L);

      // Extend those that need extending
      for(unsigned int i = 0; i < orderedClassList.size(); i++)
      {
         // Non sub-classes will not be in this list
         if(getExtensionFunctions()[orderedClassList[i]] == NULL)
            continue;

         getExtensionFunctions()[orderedClassList[i]](L);
      }
   }
};

// Helper classes that extend LuaW_Registrar.  These are intended for use
// only by one of the two macro definitions below.
template<class T>
class LuaW_Registrar1Arg : public LuaW_Registrar
{
public:
   LuaW_Registrar1Arg() { registerClass<T>(); }
};


template<class T, class U>
class LuaW_Registrar2Args : public LuaW_Registrar
{
public:
   LuaW_Registrar2Args() { registerClass<T, U>(); }
};


#define REGISTER_LUA_CLASS(cls) \
   static LuaW_Registrar1Arg<cls> luaclass_##cls

#define REGISTER_LUA_SUBCLASS(cls, parent) \
   static LuaW_Registrar2Args<cls, parent> luaclass_##cls





template <class T>
class LuaProxy
{
private:
    S32 mId;
    bool mDefunct;
    T *mProxiedObject;

public:
    // Default constructor
    LuaProxy() { TNLAssert(false, "Not used"); }

    // Typical constructor
    LuaProxy(T *obj)
    {
      mProxiedObject = obj;
      obj->setLuaProxy(this);
      mDefunct = false;

      logprintf("XXXXX Creating %s proxy for %p (proxy addr: %p)", typeid(T).name(), mProxiedObject, this);
    }

   // Destructor
   ~LuaProxy()
   {
      TNLAssert(false, "");
      logprintf("XXXXX Deleting LuaProxy for %s%p (proxy addr %p)", mDefunct ? "Defunct " : "", mProxiedObject, this);

      if(!mDefunct)
         mProxiedObject->mLuaProxy = NULL;
   }


   T *getProxiedObject()
   {
      return mProxiedObject;
   }


   void setDefunct(bool isDefunct)
   {
      mDefunct = isDefunct;
   }


   bool isDefunct()
   {
      return mDefunct;
   }
};





// This goes in the header of a "wrapped class"
#define  LUAW_DECLARE_CLASS(className) \
   LuaProxy<className> *mLuaProxy; \
   LuaProxy<className> *getLuaProxy() { return mLuaProxy; } \
   void setLuaProxy(LuaProxy<className> *obj) { mLuaProxy = obj; } \
   className (lua_State *L) { }  /* Unused but required constructor */ \
   void push(lua_State *L) { luaW_push(L, this); }


// This goes in the constructor of the "wrapped class"
#define LUAW_CONSTRUCTOR_INITIALIZATIONS \
   mLuaProxy = NULL


// And this in the destructor of the "wrapped class"
#define LUAW_DESTRUCTOR_CLEANUP \
   if(mLuaProxy) mLuaProxy->setDefunct(true)



// Runs a method on a proxied object.  Returns nil if the proxied object no longer exists, so Lua scripts may need to check for this.
// Wraps a standard method (one that takes L as a single parameter) within a proxy check. 
template <typename T, int (T::*methodName)(lua_State * )>
int luaW_doMethod(lua_State *L)
{
   T *w = luaW_check<T>(L, 1);
   if(w) 
   {
      lua_remove(L, 1);
      return (w->*methodName)(L);
   }

   lua_pushnil(L);
   return 1;
}

/*
 * Copyright (c) 2010-2011 Alexander Ames
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#endif // LUA_WRAPPER_H_
