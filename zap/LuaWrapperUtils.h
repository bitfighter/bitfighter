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

///////////////////////////////////////////////////////////////////////////////
//
// A set if enum helper functions and macros
// These are not actually typesafe past just checking if the value is an int
//

template <typename T>
void luaU_setenum(lua_State* L, int index, const char* key, T value)
{
    lua_pushnumber(L, static_cast<int>(value));
    lua_setfield(L, luaW_correctindex(L, index, 1), key);
}

template <typename T>
T luaU_toenum(lua_State* L, int index)
{
    return static_cast<T>(lua_tointeger(L, index));
}

template <typename T>
void luaU_setenum(lua_State* L, int index, const char* key, T value)
{
    lua_pushnumber(L, static_cast<int>(value));
}

template <typename T>
void luaU_pushenum(lua_State* L, T value)
{
    lua_pushnumber(L, static_cast<int>(value));
}

template <typename T>
T luaU_checkenum(lua_State* L, int index)
{
    return static_cast<T>(luaL_checkinteger(L, index));
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
// It is often useful to override luaU_check, luaU_to and/or luaU_push to
// operate on your own simple types rather than register your type with
// LuaWrapper, especially with small objects.
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
inline U luaU_checkfield(lua_State* L, int index, const char* field)
{
    lua_getfield(L, index, field);
    U val = luaU_check<U>(L, -1);
    lua_pop(L, 1);
    return val;
}

template <typename U>
inline void luaU_setfield(lua_State* L, int index, const char* field, U val)
{
    luaU_push<U>(L, val);
    lua_setfield(L, luaW_correctindex(L, index, 1), field);
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
//     { NULL, NULL }
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

template <typename T, typename U, U* T::*Member, bool release = false>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
    {
        U* member = luaU_checkornil<U>(L, 2);
        obj->*Member = member;
        if (member && release)
            luaW_release<U>(L, member);
    }
    return 0;
}

template <typename T, typename U, const U* T::*Member, bool release = false>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
    {
        U* member = luaU_checkornil<U>(L, 2);
        obj->*Member = member;
        if (member && release)
            luaW_release<U>(L, member);
    }
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

template <typename T, typename U, void (T::*Setter)(U*), bool release = false>
int luaU_set(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj)
    {
        U* member = luaU_checkornil<U>(L, 2);
        (obj->*Setter)(member);
        if (member && release)
            luaW_release<U>(L, member);
    }
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

template <typename T, typename U, U* T::*Member, bool release = false>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        U* member = luaU_checkornil<U>(L, 2);
        obj->*Member = member;
        if (member && release)
            luaW_release<U>(L, member);
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

template <typename T, typename U, U* (T::*Getter)() const, void (T::*Setter)(U*), bool release = false>
int luaU_getset(lua_State* L)
{
    T* obj = luaW_check<T>(L, 1);
    if (obj && lua_gettop(L) >= 2)
    {
        U* member = luaU_checkornil<U>(L, 2);
        (obj->*Setter)(member);
        if (member && release)
            luaW_release<U>(L, member);
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
// luaU_func is a special macro that expands into a simple function wrapper.
// Unlike the getter setters above, you merely need to name the function you
// would like to wrap. 
//
// For example, 
//
// struct Foo
// {
//     int DoSomething(int, const char*);
// };
//
// static luaL_reg Foo_Metatable[] =
// {
//     { "DoSomething", luaU_func(&Foo::DoSomething) },
//     { NULL, NULL }
// }
//
// This macro will expand based on the function signature of Foo::DoSomething
// In this example, it would expand into the following wrapper:
//
//     luaU_push(luaW_check<T>(L, 1)->DoSomething(luaU_check<int>(L, 2), luaU_check<const char*>(L, 3)));
//     return 1;
//
// This macro and it's underlying templates are somewhat experimental and some
// refinements are probably needed.  There are cases where it does not
// currently work and I expect some changes can be made to refine its behavior.
//

#define luaU_func(...) &luaU_FuncWrapper<decltype(__VA_ARGS__),__VA_ARGS__>::call

template<int... ints> struct luaU_IntPack {};
template<int start, int count, int... tail> struct luaU_MakeIntRangeType { typedef typename luaU_MakeIntRangeType<start, count-1, start+count-1, tail...>::type type; };
template<int start, int... tail> struct luaU_MakeIntRangeType<start, 0, tail...> { typedef luaU_IntPack<tail...> type; };
template<int start, int count> inline typename luaU_MakeIntRangeType<start, count>::type luaU_makeIntRange() { return typename luaU_MakeIntRangeType<start, count>::type(); }
template<class MemFunPtrType, MemFunPtrType MemberFunc> struct luaU_FuncWrapper;

template<class T, class ReturnType, class... Args, ReturnType(T::*MemberFunc)(Args...)>
struct luaU_FuncWrapper<ReturnType(T::*)(Args...), MemberFunc>
{
public:
    static int call(lua_State* L)
    {
        return callImpl(L,luaU_makeIntRange<2,sizeof...(Args)>());
    }

private:
    template<int... indices>
    static int callImpl(lua_State* L, luaU_IntPack<indices...>)
    {
        luaU_push<ReturnType>(L, (luaW_check<T>(L, 1)->*MemberFunc)(luaU_check<Args>(L, indices)...));
        return 1;
    }
};

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