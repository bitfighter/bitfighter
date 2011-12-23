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

static int gcvec_new (lua_State *L) {
  float x = (float)lua_tonumber(L, 1);
  float y = (float)lua_tonumber(L, 2);
  new_vec(L, x, y);
  return 1;
}

static int gcvec_dot (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, v1->x*v2->x + v1->y*v2->y);
  return 1;
}

/* U x V = Ux*Vy-Uy*Vx */
static int gcvec_cross (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  new_vec(L, v1->x * v2->y - v1->y * v2->x, 0.0f);
  return 1;
}

static int gcvec_length (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  lua_pushnumber(L, sqrtf(v->x*v->x + v->y*v->y));
  return 1;
}

// Bitfighter method
static int gcvec_lengthsquared (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  lua_pushnumber(L, v->x*v->x + v->y*v->y);
  return 1;
}

static int gcvec_normalize (lua_State *L) {
  const vec_t* v = checkvec(L, 1);
  float s = 1.0f / sqrtf(v->x*v->x + v->y*v->y);
  new_vec(L, v->x*s, v->y*s);
  return 1;
}

// Bitfighter method
static int gcvec_angleto (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, atan2(v2->y - v1->y, v2->x - v1->x));

  return 1;
}

// Bitfighter method
static int gcvec_distanceto (lua_State *L) {
  const vec_t* v1 = checkvec(L, 1);
  const vec_t* v2 = checkvec(L, 2);
  lua_pushnumber(L, sqrt((v2->x - v1->x)*(v2->x - v1->x) + (v2->y - v1->y)*(v2->y - v1->y)));

  return 1;
}

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
  lua_pushfstring( L, "vec(%f, %f)", v->x, v->y);
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

