/*
 * Copyright (c) 2010-2011 Alexander Ames
 * Alexander.Ames@gmail.com
 * See Copyright Notice at the end of this file
 */

// API Summary:
//
// This is a set of utility functions that add to the functionalit of
// LuaWrapper. Over time I found myself repeating a few patterns, such as
// writing many trvial getter and setter functions, and wanting pass ownership
// of one object to another and have the Lua script properly handle that case.
//
// This file contains the additional functions that I've added but that do
// not really belong in the core API.

#ifndef LUAWRAPPERUTILS_HPP_
#define LUAWRAPPERUTILS_HPP_

#include "LuaWrapper.hpp"

inline int luaU_correctindex(lua_State* L, int index, int correction)
{
    return index < 0 ? index - correction : index;
}

///////////////////////////////////////////////////////////////////////////////
//
// A set if enum helper functions and macros
// These are not actually typesafe past just checking if the value is an int
//

template <typename T>
void luaU_pushenum(lua_State* L, int index, const char* key, T value)
{
    lua_pushnumber(L, value);
    lua_setfield(L, luaU_correctindex(L, index, 1), key);
}

template <typename T>
T luaU_toenum(lua_State* L, int index)
{
    return (T)lua_tointeger(L, index);
}

template <typename T>
T luaU_checkenum(lua_State* L, int index)
{
    return (T)luaL_checkinteger(L, index);
}

template <typename T>
T* luaU_checkornil(lua_State* L, int index, bool strict = false)
{
    if (lua_isnil(L, index))
        return NULL;
    else
        return luaW_check<T>(L, index, strict);
}

///////////////////////////////////////////////////////////////////////////////
//
// A set of templated luaL_check and lua_push functions for use in the getters
// and setters below
//

template <typename U> U luaU_check(lua_State* L, int index);
template <> inline bool          luaU_check<>(lua_State* L, int index) { return lua_toboolean(L, index); }
template <> inline const char*   luaU_check<>(lua_State* L, int index) { return luaL_checkstring(L, index); }
template <> inline unsigned int  luaU_check<>(lua_State* L, int index) { return luaL_checkinteger(L, index); }
template <> inline int           luaU_check<>(lua_State* L, int index) { return luaL_checkinteger(L, index); }
template <> inline unsigned char luaU_check<>(lua_State* L, int index) { return luaL_checkinteger(L, index); }
template <> inline char          luaU_check<>(lua_State* L, int index) { return luaL_checkinteger(L, index); }
template <> inline float         luaU_check<>(lua_State* L, int index) { return luaL_checknumber(L, index); }
template <> inline double        luaU_check<>(lua_State* L, int index) { return luaL_checknumber(L, index); }

template <typename U> U luaU_to(lua_State* L, int index);
template <> inline bool          luaU_to<>(lua_State* L, int index) { return lua_toboolean(L, index); }
template <> inline const char*   luaU_to<>(lua_State* L, int index) { return lua_tostring(L, index); }
template <> inline unsigned int  luaU_to<>(lua_State* L, int index) { return lua_tointeger(L, index); }
template <> inline int           luaU_to<>(lua_State* L, int index) { return lua_tointeger(L, index); }
template <> inline unsigned char luaU_to<>(lua_State* L, int index) { return lua_tointeger(L, index); }
template <> inline char          luaU_to<>(lua_State* L, int index) { return lua_tointeger(L, index); }
template <> inline float         luaU_to<>(lua_State* L, int index) { return lua_tonumber(L, index); }
template <> inline double        luaU_to<>(lua_State* L, int index) { return lua_tonumber(L, index); }

template <typename U> void luaU_push(lua_State* L, const U& val);
template <> inline void luaU_push<>(lua_State* L, const bool&          val) { lua_pushboolean(L, val); }
template <> inline void luaU_push<>(lua_State* L, const int&           val) { lua_pushinteger(L, val); }
template <> inline void luaU_push<>(lua_State* L, const unsigned int&  val) { lua_pushinteger(L, val); }
template <> inline void luaU_push<>(lua_State* L, const char&          val) { lua_pushinteger(L, val); }
template <> inline void luaU_push<>(lua_State* L, const unsigned char& val) { lua_pushinteger(L, val); }
template <> inline void luaU_push<>(lua_State* L, const char* const&   val) { lua_pushstring(L, val); }
template <> inline void luaU_push<>(lua_State* L, const float&         val) { lua_pushnumber(L, val); }
template <> inline void luaU_push<>(lua_State* L, const double&        val) { lua_pushnumber(L, val); }

///////////////////////////////////////////////////////////////////////////////
//
// These are just some functions I've always felt should exist
//

template <typename U>
inline U luaU_getfield(lua_State* L, int index, const char* field)
{
    lua_getfield(L, index, field);
    U val = luaU_to<U>(L, -1);
    lua_pop(L, 1);
    return val;
}

template <typename U>
inline void luaU_setfield(lua_State* L, int index, const char* field, U val)
{
    luaU_push<U>(L, val);
    lua_setfield(L, luaU_correctindex(L, index, 1), field);
}

///////////////////////////////////////////////////////////////////////////////
//
// A set of trivial getter and setter templates. These templates are designed
// to call trivial getters or setters.
//
// There are four forms:
//  1. Getting or setting a public member variable that is of a primitive type
//  2. Getting or setting a public member variable that is a pointer to an
//     object
//  3. Getting or setting a private member variable that is of a primitive type
//     through a getter or setter
//  3. Getting or setting a private member variable that is is a pointer to an
//     object through a getter or setter
//
// The interface to all of them is the same however. In addition to plain
// getter and setter functions, there is a getset which does both - if an
// argument is supplied it attempts to set the value, and in either case it
// returns the value. In your lua table declaration in C++ rather than write
// individiual wrappers for each getter and setter you may do the following:
//
// static luaL_reg Foo_Metatable[] =
// {
//     { "GetBar", luaU_get<Foo, bool, &Widget::GetBar> },
//     { "SetBar", luaU_set<Foo, bool, &Widget::SetBar> },
//     { "Bar", luaU_getset<Foo, bool, &Widget::GetBar, &Widget::SetBar> },
// }
//
// Getters and setters must have the following signatures:
//    void T::Setter(U value);
//    void T::Setter(U* value);
//    void T::Setter(const U& value);
//    U Getter() const;
//    U* Getter() const;
//
// In this example, the variable Foo::bar is private and so it is accessed
// through getter and setter functions. If Foo::bar were public, it could be
// accessed directly, like so:
//
// static luaL_reg Foo_Metatable[] =
// {
//     { "GetBar", luaU_get<Foo, bool, &Widget::bar> },
//     { "SetBar", luaU_set<Foo, bool, &Widget::bar> },
//     { "Bar", luaU_getset<Foo, bool, &Widget::bar> },
// }
//
// In a Lua script, you can now use foo:GetBar(), foo:SetBar() and foo:Bar()
//

template <typename T, typename U, U T::*Member>
int luaU_get(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    luaU_push<U>(L, obj->*Member);
    return 1;
}

template <typename T, typename U, U* T::*Member>
int luaU_get(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    luaW_push<U>(L, obj->*Member);
    return 1;
}

template <typename T, typename U, U (T::*Getter)() const>
int luaU_get(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    luaU_push(L, (obj->*Getter)());
    return 1;
}

template <typename T, typename U, const U& (T::*Getter)() const>
int luaU_get(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    luaU_push(L, (obj->*Getter)());
    return 1;
}

template <typename T, typename U, U* (T::*Getter)() const>
int luaU_get(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    luaW_push<U>(L, (obj->*Getter)());
    return 1;
}

template <typename T, typename U, U T::*Member>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
        obj->*Member = luaU_check<U>(L, 2);
    return 0;
}

template <typename T, typename U, U* T::*Member>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
        obj->*Member = luaU_checkornil<U>(L, 2);
    return 0;
}

template <typename T, typename U, void (T::*Setter)(U)>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
        (obj->*Setter)(luaU_check<U>(L, 2));
    return 0;
}

template <typename T, typename U, void (T::*Setter)(const U&)>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
        (obj->*Setter)(luaU_check<U>(L, 2));
    return 0;
}

template <typename T, typename U, void (T::*Setter)(U*)>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
        (obj->*Setter)(luaU_checkornil<U>(L, 2));
    return 0;
}

template <typename T, typename U, U T::*Member>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        obj->*Member = luaU_check<U>(L, 2);
        return 0;
    }
    else
    {
        luaU_push<U>(L, obj->*Member);
        return 1;
    }
}

template <typename T, typename U, U* T::*Member>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        obj->*Member = luaU_checkornil<U>(L, 2);
        return 0;
    }
    else
    {
        luaW_push<U>(L, obj->*Member);
        return 1;
    }
}

template <typename T, typename U, U (T::*Getter)() const, void (T::*Setter)(U)>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        (obj->*Setter)(luaU_check<U>(L, 2));
        return 0;
    }
    else
    {
        luaU_push(L, (obj->*Getter)());
        return 1;
    }
}

template <typename T, typename U, U (T::*Getter)() const, void (T::*Setter)(const U&)>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        (obj->*Setter)(luaU_check<U>(L, 2));
        return 0;
    }
    else
    {
        luaU_push(L, (obj->*Getter)());
        return 1;
    }
}

template <typename T, typename U, const U& (T::*Getter)() const, void (T::*Setter)(const U&)>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        (obj->*Setter)(luaU_check<U>(L, 2));
        return 0;
    }
    else
    {
        luaU_push(L, (obj->*Getter)());
        return 1;
    }
}

template <typename T, typename U, U* (T::*Getter)() const, void (T::*Setter)(U*)>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        (obj->*Setter)(luaU_checkornil<U>(L, 2));
        return 0;
    }
    else
    {
        luaW_push(L, (obj->*Getter)());
        return 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Calls the copy constructor for an object of type T.
// Arguments may be passed in, in case they're needed for the postconstructor
//
// e.g.
//
// C++:
// luaL_reg Foo_Metatable[] =
// {
//     { "clone", luaU_clone<Foo> },
//     { NULL, NULL }
// };
//
// Lua:
// foo = Foo.new()
// foo2 = foo:clone()
//
template <typename T>
int luaU_clone(lua_State* L)
{
    // obj ...
    T* obj = new T(*luaW_check<T>(L, 1));
    lua_remove(L, 1); // ...
    int numargs = lua_gettop(L);
    luaW_push<T>(L, obj); // ... clone
    luaW_hold<T>(L, obj);
    luaW_postconstructor<T>(L, numargs);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// Takes the object of type T at the top of the stack and stores it in on a
// table with the name storagetable, on the table at the specified index.
//
// You may manually call luaW_hold and luaW_release to handle pointer
// ownership, but it is often easier to simply store a Lua userdata on a table
// that is owned by its parent. This ensures that your object will not be
// prematurely freed, and that it can only be destoryed after its parent.
//
// e.g.
//
// Foo* parent = luaW_check<Foo>(L, 1);
// Foo* child = luaW_check<Foo>(L, 2);
// if (parent && child)
// {
//     luaU_store<Foo>(L, 1, "Children");
//     parent->AddChild(child);
// }
//
template <typename T>
void luaU_store(lua_State* L, int index, const char* storagetable, const char* key = NULL)
{
    // ... store ... obj
    lua_getfield(L, index, storagetable); // ... store ... obj store.storagetable
    if (key)
        lua_pushstring(L, key); // ... store ... obj store.storagetable key
    else
        LuaWrapper<T>::identifier(L, luaW_to<T>(L, -2)); // ... store ... obj store.storagetable key
    lua_pushvalue(L, -3); // ... store ... obj store.storagetable key obj
    lua_settable(L, -3); // ... store ... obj store.storagetable
    lua_pop(L, 1); // ... store ... obj
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

#endif // LUAWRAPPERUTILS_HPP_