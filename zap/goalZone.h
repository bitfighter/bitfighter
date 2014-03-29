//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GOALZONE_H_
#define _GOALZONE_H_

#include "Zone.h"          // Parent class
#include "Timer.h"

namespace Zap
{

class GoalZone : public GameZone
{
   typedef GameZone Parent;

private:
   static const S32 FlashDelay = 500;
   static const S32 FlashCount = 5;

   bool mHasFlag;     // Is there a flag parked in this zone?
   S32 mScore;        // How much is this zone worth?
   ClientInfo *mCapturer;  // Who captured this zone - used in Zone Control

   S32 mFlashCount;
   Timer mFlashTimer;

protected:
   enum MaskBits {
      InitialMask   = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1
   };

public:
   explicit GoalZone(lua_State *L = NULL);   // Combined Lua / C++ constructor
   virtual ~GoalZone();             // Destructor

   GoalZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);
   
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(BfObject::IdleCallPath path);
   void render();

   bool didRecentlyChangeTeam();

   void setTeam(S32 team);
   void setTeam(lua_State *L, S32 index);

   void onAddedToGame(Game *theGame);
   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);
   
   bool isFlashing();
   void setFlashCount(S32 i);

   S32 getScore();
   //bool hasFlag();
   void setHasFlag(bool hasFlag);
   
   ClientInfo *getCapturer();
   void setCapturer(ClientInfo *clientInfo);

   TNL_DECLARE_CLASS(GoalZone);

   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   bool hasTeam();      
   bool canBeHostile(); 
   bool canBeNeutral(); 


   string toLevelCode() const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false);
   void renderDock();

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(GoalZone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_hasFlag(lua_State *L);
};


};

#endif
