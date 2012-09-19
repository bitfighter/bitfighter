/*
** Alternative vec lib implementation with garbage collected userdata vectors
** See Copyright Notice in lua.h
**
** Note that this file has been modified for usage in Bitfighter, and optimized for 2-D work
*/


#include <stdlib.h>
#include <math.h>

#define lgcveclib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#define checkvec(L, n) (vec_t*)luaL_checkudata( L, n, "gcvec.vec" )

typedef struct
{
	float x, y;
} vec_t;

static void new_vec( lua_State* L, float x, float y)
{
  vec_t* v = (vec_t*)lua_newuserdata( L, sizeof(vec_t) );
  v->x = x;
  v->y = y;
  luaL_getmetatable(L, "gcvec.vec");
  lua_setmetatable(L, -2);
}


/**
  * @luavclass point
  * @brief     Class/library representing an x,y coordinate pair
  * @descr     Bitfighter uses a modified version of lua-vec as its embedded Lua interpreter.  Lua-vec extends the core language  
  *            and virtual machine with a new datatype, a two element 64-bit wide %point (2 x 32-bit float values).
  *            The new datatype is a first class type, like the regular number type is. This means that there is no penalty 
  *            for creating new points and there is no need for garbage collection. Points also don't allocate memory 
  *            dynamically when created because they're value types, not objects. Points are created with a special 
  *            creator function that converts up to two numbers to a %point. 
  *
  * After creation you can use standard arithmetic operators on them.  Points are immutable; once created, they cannot be changed.
  *
  * Note that while points are documented here as a class, they are really more of a library, modeled after other Lua libraries such
  * as the standard Table library.  As with other basic data types, 
  * points do not support . or : syntax.
  *
  * @code      
  *            local p = point.new(100, 200)
  *            local q = point.new(  5,  10)
  *            local r = point.new(100, 100)
  *            
  *            print(point.length(p))        -- 223.60679626465
  *            print(p + q)                  -- (105, 210)
  *            print(p * 2)                  -- (200, 400)
  *            print(p * q)                  -- (500, 2000)
  *            
  *            print(point.distanceTo(p, r)) -- 100
  * @endcode
  *
  */

/**
  * @luafunc %point point::new(x, y)
  * @brief   Create a new %point.
  * @param   x - The x coordinate for the new %point.
  * @param   y - The y coordinate for the new %point.
  */
static int gcvec_new (lua_State *L) {
  float x = (float)lua_tonumber(L, 1);
  float y = (float)lua_tonumber(L, 2);
  new_vec(L, x, y);
  return 1;
}

/**
  * @luafunc num point::dot(p, q)
  * @brief   Computes p dot q.
  * @descr   Computes p.x * q.x + p.y * q.y. 
  * @param   p - Point.
  * @param   q - Point.
  * @return  The computed dot product.
  */
static int gcvec_dot (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, v1->x*v2->x + v1->y*v2->y);
  return 1;
}

/** 
  * @luafunc num point::cross(p, q)
  * @brief   Computes p x q.
  * @descr   Computes p.x * q.y - p.y * q.x.
  * @param   p - Point.
  * @param   q - Point.
  * @return  The computed cross product.
  */
static int gcvec_cross (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, v1->x * v2->y - v1->y * v2->x);      /* U x V = Ux*Vy-Uy*Vx */
  return 1;
}

/**
  * @luafunc num point::length(p)
  * @brief   Computes length of p, or its distance from (0,0).
  * @descr   Distance is computed by sqrt(p.x * p.x + p.y * p.y).
  * @param   p - Point.
  * @returns Distance of from (0,0).
  */
static int gcvec_length (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  lua_pushnumber(L, sqrtf(v->x*v->x + v->y*v->y));
  return 1;
}

/**     Bitfighter method
  * @luafunc num point::lengthSquared(p)
  * @brief   Computes p's length squared, or the square of the distance of the %point from (0,0).
  * @desc    This function is less computationaly intensive than \em %length(), and can be used equally well for
  *          many purposes, such as comparing distances.  
  *
  * Computation is p.x * p.x + p.y * p.y.
  * @param   p - Point.
  * @return  Distance squared from (0,0).
  */
static int gcvec_lengthsquared (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  lua_pushnumber(L, v->x*v->x + v->y*v->y);
  return 1;
}

/**
  * @luafunc %point point::normalize(p)
  * @brief   Returns a %point with a length of 1.
  * @descr   Recomputes p.x and p.y in such a way that their relative lengths remain the same, but the
  *          total length = 1.
  * @param   p - Point.
  * @return  Normalized %point.
  */
static int gcvec_normalize (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  float s = 1.0f / sqrtf(v->x*v->x + v->y*v->y);
  new_vec(L, v->x*s, v->y*s);
  return 1;
}

/**     Bitfighter method
  * @luafunc num point::angleTo(p, q)
  * @brief   Computes angle from p to q.
  * @descr   Uses formula: atan2(q.y - p.y, q.x - p.x)
  * @param   p - Point.
  * @param   q - Point.
  * @return  Computed angle.
  */
static int gcvec_angleto (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, atan2(v2->y - v1->y, v2->x - v1->x));

  return 1;
}

/**     Bitfighter method
  * @luafunc num point::distanceTo(p, q)
  * @brief   Compute distance from p to q.
  * @param   p - Point.
  * @param   q - Point.
  * @return  Computed distance.
  */
static int gcvec_distanceto (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, sqrt((v2->x - v1->x)*(v2->x - v1->x) + (v2->y - v1->y)*(v2->y - v1->y)));

  return 1;
}

/**
  * @luafunc num point::distSquared(p, q)
  * @brief   Compute distance squared between p and q.
  * @param   p - Point.
  * @param   q - Point.
  * @return  Distance squared to p.
  */
static int gcvec_distsquared (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, (v2->x - v1->x)*(v2->x - v1->x) + (v2->y - v1->y)*(v2->y - v1->y));

  return 1;
}


static int gcvec_set_item( lua_State* L )
{
  vec_t* v = checkvec( L, 1 );
  int i = luaL_checkint( L, 2 );
  float val = (float)luaL_checknumber( L, 3 );
  luaL_argcheck( L, i >= 1 && i <= LUA_VEC_SIZE, 2, "index out of range" );
  (&v->x)[i-1] = val;
  return 0;
}

static int gcvec_get_item( lua_State* L )
{
  vec_t* v = checkvec( L, 1 );
  int i = luaL_checkint( L, 2 );
  luaL_argcheck( L, i >= 1 && i <= LUA_VEC_SIZE, 2, "index out of range" );
  lua_pushnumber( L, (&v->x)[i-1] );
  return 1;
}

static int gcvec_add( lua_State* L )
{
  vec_t* v1 = checkvec( L, 1 );
  vec_t* v2 = checkvec( L, 2 );
  new_vec( L, v1->x + v2->x, v1->y + v2->y);
  return 1;
}

static int gcvec_sub( lua_State* L )
{
  vec_t* v1 = checkvec( L, 1 );
  vec_t* v2 = checkvec( L, 2 );
  new_vec( L, v1->x - v2->x, v1->y - v2->y);
  return 1;
}

static int gcvec_mul( lua_State* L )
{
  vec_t* v1 = checkvec( L, 1 );
  if( lua_isuserdata(L, 2) )
  {
    // vector * vector
    vec_t* v2 = checkvec( L, 2 );
    new_vec( L, v1->x * v2->x, v1->y * v2->y);
  }
  else
  {
    // vector * scalar
    float s = (float)luaL_checknumber( L, 2 );
    new_vec( L, v1->x * s, v1->y * s);
  }
  return 1;
}

static int gcvec_div( lua_State* L )
{
  vec_t* v1 = checkvec( L, 1 );
  float s = (float)luaL_checknumber( L, 2 );
  luaL_argcheck( L, s != 0.0f, 2, "division by zero" );
  new_vec( L, v1->x / s, v1->y / s);
  return 1;
}

static int gcvec_negate( lua_State* L )
{
  vec_t* v = checkvec( L, 1 );
  new_vec( L, -v->x, -v->y);
  return 1;
}

static int gcvec_tostring( lua_State* L )
{
  vec_t* v = checkvec(L, 1);
  lua_pushfstring( L, "point(%f, %f)", v->x, v->y);
  return 1;
}


// Added new Bitfighter methods
static const luaL_Reg gcveclib_f[] = {
  {"new",           gcvec_new},
  {"dot",           gcvec_dot},
  {"cross",         gcvec_cross},
  {"length",        gcvec_length},
  {"lengthSquared", gcvec_lengthsquared},
  {"normalize",     gcvec_normalize}, 
  {"angleTo",       gcvec_angleto},
  {"distanceTo",    gcvec_distanceto},
  {"distSquared",   gcvec_distsquared},
  {NULL, NULL}
};

static const luaL_Reg gcveclib_m[] = {
  {"__newindex", gcvec_set_item},
  {"__index",    gcvec_get_item},
  {"__add",      gcvec_add},
  {"__sub",      gcvec_sub},
  {"__mul",      gcvec_mul},
  {"__div",      gcvec_div},
  {"__unm",      gcvec_negate},
  {"__tostring", gcvec_tostring},
  {NULL, NULL}
};


/*
** Open gcveclib
*/
LUALIB_API int luaopen_gcvec (lua_State *L) {
  // init mt
  luaL_newmetatable( L, "gcvec.vec" );
  lua_pushvalue( L, -1 );
  lua_setfield( L, -2, "__index" );	// metatable.__index = metatable
  luaL_register( L, NULL, gcveclib_m );

  luaL_register(L, LUA_GCVECLIBNAME, gcveclib_f);
  
  // numeric constants
  new_vec(L, 0, 0);
  lua_setfield(L, -2, "zero");
  new_vec(L, 1, 1);
  lua_setfield(L, -2, "one");
  return 1;
}

