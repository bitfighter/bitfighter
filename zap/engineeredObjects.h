//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _ENGINEEREDOBJECTS_H_
#define _ENGINEEREDOBJECTS_H_

#include "gameObject.h"
#include "item.h"
#include "barrier.h"

namespace Zap
{

extern void engClientCreateObject(GameConnection *connection, U32 object);

class EngineeredObject : public GameObject, public LuaItem
{
private:
   typedef GameObject Parent;

protected:
   F32 mHealth;
   //Color mTeamColor;   ok to delete
   SafePtr<Item> mResource;
   SafePtr<Ship> mOwner;
   Point mAnchorPoint;
   Point mAnchorNormal;
   bool mIsDestroyed;
   S32 mOriginalTeam;

   S32 mHealRate;       // Rate at which items will heal themselves, defaults to 0
   Timer mHealTimer;    // Timer for tracking mHealRate

   enum MaskBits
   {
      InitialMask = BIT(0),
      HealthMask = BIT(1),
      TeamMask = BIT(2),
      NextFreeMask = BIT(3),
   };

   virtual void setObjectMask() = 0;   // Must be overridden by child classes

public:
   EngineeredObject(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point());
   bool processArguments(S32 argc, const char **argv);

   void setResource(Item *resource);
   bool checkDeploymentPosition();
   void computeExtent();
   virtual void onDestroyed() { /* do nothing */ }  
   virtual void onDisabled() { /* do nothing */ } 
   virtual void onEnabled() { /* do nothing */ }  
   virtual bool isTurret() { return false; }
   bool isEnabled();    // True if still active, false otherwise

   void explode();
   bool isDestroyed() { return mIsDestroyed; }
   void setOwner(Ship *owner);    
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void damageObject(DamageInfo *damageInfo);
   bool collide(GameObject *hitObject) { return true; }
   F32 getHealth() { return mHealth; }
   void healObject(S32 time);

   // LuaItem interface
   // S32 getLoc(lua_State *L) { }   ==> Will be implemented by derived objects
   // S32 getRad(lua_State *L) { }   ==> Will be implemented by derived objects
   S32 getVel(lua_State *L) { return LuaObject::returnPoint(L, Point(0, 0)); }   

   // More Lua methods that are inherited by turrets and forcefield projectors
   S32 getTeamIndx(lua_State *L) { return returnInt(L, getTeam()); }
   S32 getHealth(lua_State *L) { return returnFloat(L, mHealth); }
   S32 isActive(lua_State *L) { return returnInt(L, isEnabled()); }

   GameObject *getGameObject() { return this; }

};


class ForceField : public GameObject
{
private:
   Point mStart, mEnd;
   Timer mDownTimer;
   bool mFieldUp;

public:
   enum Constants
   {
      InitialMask = BIT(0),
      StatusMask = BIT(1),

      FieldDownTime = 250,
   };

   ForceField(S32 team = -1, Point start = Point(), Point end = Point());
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void render();
   S32 getRenderSortValue() { return 0; }

   TNL_DECLARE_CLASS(ForceField);
};


class ForceFieldProjector : public EngineeredObject
{
private:
   typedef EngineeredObject Parent;
   SafePtr<ForceField> mField;
   void setObjectMask() { mObjectTypeMask = ForceFieldProjectorType | CommandMapVisType; }

public:
   static const S32 defaultRespawnTime = 0;

   ForceFieldProjector(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point());  // Constructor

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void onAddedToGame(Game *theGame);
   void idle(GameObject::IdleCallPath path);

   void render();
   void onEnabled();
   void onDisabled();


   TNL_DECLARE_CLASS(ForceFieldProjector);

   ///// Lua Interface

   ForceFieldProjector(lua_State *L);             //  Lua constructor

   static const char className[];

   static Lunar<ForceFieldProjector>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, ForceFieldProjectorType); }
   void push(lua_State *L) {  Lunar<ForceFieldProjector>::push(L, this); }

   // LuaItem methods
   enum {
      radius = 7,
   };

   S32 getRad(lua_State *L) { return returnInt(L, radius); }
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, mAnchorPoint + mAnchorNormal * radius ); } 
};


class Turret : public EngineeredObject
{
private:
   typedef EngineeredObject Parent;
   Timer mFireTimer;
   F32 mCurrentAngle;
   void setObjectMask() { mObjectTypeMask = TurretType | CommandMapVisType; }

public:
   static const S32 defaultRespawnTime = 0;

   enum {
      TurretAimOffset = 15,               // I think this is some factor to account for the fact that turrets do not shoot from their center
                                          // Also serves as radius of circle of turret's body, where the turret starts
      TurretPerceptionDistance = 800,     // Area to search for potential targets...
      TurretTurnRate = 4,                 // How fast can turrets turn to aim?
      // Turret projectile characteristics (including bullet range) set in gameWeapons.cpp

      AimMask = EngineeredObject::NextFreeMask,
   };

   Turret(S32 team = -1, Point anchorPoint = Point(), Point anchorNormal = Point(1, 0));     // Constructor

   bool getCollisionPoly(Vector<Point> &polyPoints);
   void render();
   void idle(IdleCallPath path);
   void onAddedToGame(Game *theGame);
   bool isTurret() { return true; }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(Turret);

   ///// Lua Interface

   Turret(lua_State *L);             //  Lua constructor
   static const char className[];
   static Lunar<Turret>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, TurretType); }
   void push(lua_State *L) { Lunar<Turret>::push(L, this); }

   // LuaItem methods
   S32 getRad(lua_State *L) { return returnInt(L, TurretAimOffset); }
   S32 getLoc(lua_State *L) { return LuaObject::returnPoint(L, mAnchorPoint + mAnchorNormal * (TurretAimOffset)); } 
};


};

#endif
