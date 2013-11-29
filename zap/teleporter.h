//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TELEPORTER_H_
#define _TELEPORTER_H_

#include "SimpleLine.h"    // For SimpleLine def
#include "Engineerable.h"

#include "Point.h"
#include "Timer.h"

#include "tnlNetObject.h"

namespace Zap
{

class Teleporter;

// Manage destinations for a teleporter
struct DestManager 
{
   private:
      Vector<Point> mDests;
      Teleporter *mOwner;

   public:
      void setOwner(Teleporter *owner);

      S32 getDestCount() const;
      Point getDest(S32 index) const;
      S32 getRandomDest() const;

      void addDest(const Point &dest);
      void setDest(S32 index, const Point &dest);
      void delDest(S32 index);

      void resize(S32 count);
      void read(S32 index, BitStream *stream);     // Read a single dest
      void read(BitStream *stream);                // Read a whole list of dests

      void clear();
      const Vector<Point> *getDestList() const;
};


////////////////////////////////////////
////////////////////////////////////////

class Teleporter : public SimpleLine, public Engineerable
{
   typedef SimpleLine Parent;

public:
   enum {
      InitMask      = Parent::FirstFreeMask << 0,
      TeleportMask  = Parent::FirstFreeMask << 1,
      HealthMask    = Parent::FirstFreeMask << 2,
      DestroyedMask = Parent::FirstFreeMask << 3,
      FirstFreeMask = Parent::FirstFreeMask << 4,

      TeleporterCooldown = 1500,             // Time teleporter remains idle after it has been used
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,

      TeleporterExplosionTime = 1000,
   };

   static const S32 TELEPORTER_RADIUS = 75;  // Overall size of the teleporter -- this is in fact a radius!

private:
   static const F32 DamageReductionFactor;

   S32 mLastDest;    // Destination of last ship through

   bool mHasExploded;
   F32 mHealth;
   Vector<Point> mOutlinePoints;

   Timer mExplosionTimer;
   Timer mTeleportCooldown;

   bool mFinalExplosionTriggered;

   DestManager mDestManager;

   SafePtr<Ship> mEngineeringShip;

   void initialize(const Point &pos, const Point &dest, Ship *engineeringShip);
   void doSetGeom(lua_State *L);    // Helper
   void computeExtent();
   void generateOutlinePoints();

public:
   explicit Teleporter(lua_State *L = NULL);                                  // Combined default C++/Lua constructor
   Teleporter(const Point &pos, const Point &dest, Ship *engineeringShip);    // Constructor used by engineer
   virtual ~Teleporter();                                                     // Destructor

   Teleporter *clone() const;

   U32 mTime;

   U32 mTeleporterCooldown;

   Point getOrigin() const;      // For clarity
   // Destination management
   S32 getDestCount() const;
   Point getDest(S32 index) const;
   void addDest(const Point &dest);
   void delDest(S32 index);

   void clearDests();

   void doSetGeom(const Vector<Point> &points);    // Public so tests can have access


   void newObjectFromDock(F32 gridSize);

   static bool checkDeploymentPosition(const Point &position, const GridDatabase *gb, const Ship *ship);

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode() const;
   Rect calcExtents();

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   F32 getHealth() const;
   bool isDestroyed();

   void damageObject(DamageInfo *theInfo);
   void onDestroyed();
   void doTeleport();
   bool collide(BfObject *otherObject);

   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   const Vector<Point> *getCollisionPoly() const;
   const Vector<Point> *getOutline() const;
   const Vector<Point> *getEditorHitPoly() const;

   void idle(BfObject::IdleCallPath path);
   void render();

#ifndef ZAP_DEDICATED
   void doExplosion();
#endif

   void onAddedToGame(Game *theGame);

   TNL_DECLARE_CLASS(Teleporter);

   TNL_DECLARE_RPC(s2cAddDestination, (Point));   
   TNL_DECLARE_RPC(s2cClearDestinations, ());   


   ///// Editor Methods
   Color getEditorRenderColor();

   void renderEditorItem();

   void onAttrsChanging();
   void onGeomChanging();
   void onGeomChanged();

   void onConstructed();

   bool hasAnyDests() const;
   void setEndpoint(const Point &point);
   const Vector<Point> *getDestList() const;

   // Some properties about the item that will be needed in the editor
   const char *getOnScreenName();
   const char *getOnDockName();
   const char *getPrettyNamePlural();
   const char *getEditorHelpString();

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();


   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(Teleporter);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_addDest(lua_State *L);
   S32 lua_delDest(lua_State *L);
   S32 lua_clearDests(lua_State *L);
   S32 lua_getDest(lua_State *L);
   S32 lua_getDestCount(lua_State *L);
   S32 lua_setEngineered(lua_State *L);
   S32 lua_getEngineered(lua_State *L);

   // Overrides
   S32 lua_setGeom(lua_State *L);
   S32 lua_getGeom(lua_State *L);
};


};

#endif
