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

#ifndef COREGAME_H_
#define COREGAME_H_

#include "gameType.h"
#include "item.h"


namespace Zap {

// Forward Declarations
class CoreItem;
class ClientInfo;

class CoreGameType : public GameType
{
   typedef GameType Parent;

private:
   Vector<SafePtr<CoreItem> > mCores;

public:
   static const S32 DestroyedCoreScore = 1;

   CoreGameType();            // Constructor
   virtual ~CoreGameType();   // Destructor

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode() const;

   bool isTeamCoreBeingAttacked(S32 teamIndex) const;

   // Runs on client
   void renderInterfaceOverlay(bool scoreboardVisible);

   void addCore(CoreItem *core, S32 team);
   void removeCore(CoreItem *core);

   // What does aparticular scoring event score?
   void updateScore(ClientInfo *player, S32 team, ScoringEvent event, S32 data = 0);
   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);
   void score(ClientInfo *destroyer, S32 coreOwningTeam, S32 score);


#ifndef ZAP_DEDICATED
   Vector<string> getGameParameterMenuKeys();
   void renderScoreboardOrnament(S32 teamIndex, S32 xpos, S32 ypos) const;
#endif

   GameTypeId getGameTypeId() const;
   const char *getGameTypeName() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


   TNL_DECLARE_CLASS(CoreGameType);
};


////////////////////////////////////////
////////////////////////////////////////

static const S32 CORE_PANELS = 10;     // Note that changing this will require update of all clients, and a new CS_PROTOCOL_VERSION

struct PanelGeom 
{
   Point vert[CORE_PANELS];            // Panel 0 stretches from vert 0 to vert 1
   Point mid[CORE_PANELS];             // Midpoint of Panel 0 is mid[0]
   Point repair[CORE_PANELS];
   F32 angle;
   bool isValid;

   Point getStart(S32 i) { return vert[i % CORE_PANELS]; }
   Point getEnd(S32 i)   { return vert[(i + 1) % CORE_PANELS]; }
};


////////////////////////////////////////
////////////////////////////////////////

class EditorAttributeMenuUI;

class CoreItem : public Item
{

typedef Item Parent;

public:
   static const F32 PANEL_ANGLE;          // = FloatTau / (F32) CoreItem::CORE_PANELS;
   static const U32 CoreRadius = 100;

private:
   static const U32 CoreMinWidth = 20;
   static const U32 CoreDefaultStartingHealth = 40;     // In ship-damage equivalents; these will be divided amongst all panels
   static const U32 CoreHeartbeatStartInterval = 2000;  // In milliseconds
   static const U32 CoreHeartbeatMinInterval = 500;
   static const U32 CoreAttackedWarningDuration = 600;
   static const U32 ExplosionInterval = 600;
   static const U32 ExplosionCount = 3;

   U32 mCurrentExplosionNumber;
   PanelGeom mPanelGeom;

   static const F32 DamageReductionRatio;

   bool mHasExploded;
   bool mBeingAttacked;
   F32 mStartingHealth;          // Health stored in the level file, will be divided amongst panels
   F32 mStartingPanelHealth;     // Health divided up amongst panels
   void setHealth();             // Sets startingHealth value, panels will be scaled up or down as needed

   F32 mPanelHealth[CORE_PANELS];
   Timer mHeartbeatTimer;        // Client-side timer
   Timer mExplosionTimer;        // Client-side timer
   Timer mAttackedWarningTimer;  // Server-side timer

#ifndef ZAP_DEDICATED
   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for attribute editing; since it's static, don't bother with smart pointer
#endif
protected:
   enum MaskBits {
      PanelDamagedMask = Parent::FirstFreeMask << 0,  // each bit mask have own panel updates (PanelDamagedMask << n)
      PanelDamagedAllMask = ((1 << CORE_PANELS) - 1) * PanelDamagedMask,  // all bits of PanelDamagedMask
      FirstFreeMask   = Parent::FirstFreeMask << CORE_PANELS
   };

public:
   explicit CoreItem(lua_State *L = NULL);   // Combined Lua / C++ default constructor
   virtual ~CoreItem();                      // Destructor
   CoreItem *clone() const;

   static F32 getCoreAngle(U32 time);
   void renderItem(const Point &pos);

   const Vector<Point> *getCollisionPoly() const;
   bool getCollisionCircle(U32 state, Point &center, F32 &radius) const;
   bool collide(BfObject *otherObject);

   bool isBeingAttacked();

   void setStartingHealth(F32 health);
   F32 getTotalCurrentHealth();           // Returns total current health of all panels
   F32 getHealth();                       // Returns overall current health of item as a ratio between 0 and 1
   bool isPanelDamaged(S32 panelIndex);
   bool isPanelInRepairRange(const Point &origin, S32 panelIndex);

   Vector<Point> getRepairLocations(const Point &repairOrigin);
   PanelGeom *getPanelGeom();
   static void fillPanelGeom(const Point &pos, S32 time, PanelGeom &panelGeom);


   void onAddedToGame(Game *theGame);
#ifndef ZAP_DEDICATED
   void onGeomChanged();
#endif

   void damageObject(DamageInfo *theInfo);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);

#ifndef ZAP_DEDICATED
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void onItemExploded(Point pos);
   void doExplosion(const Point &pos);
   void doPanelDebris(S32 panelIndex);
#endif

   void idle(BfObject::IdleCallPath path);

   bool processArguments(S32 argc, const char **argv, Game *game);
   string toLevelCode(F32 gridSize) const;

   TNL_DECLARE_CLASS(CoreItem);

#ifndef ZAP_DEDICATED
   // These four methods are all that's needed to add an editable attribute to a class...
   EditorAttributeMenuUI *getAttributeMenu();
   void startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu
   void fillAttributesVectors(Vector<string> &keys, Vector<string> &values);
#endif

   ///// Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   F32 getEditorRadius(F32 currentScale);
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);    
   void renderDock();

   bool canBeHostile();
   bool canBeNeutral();

   ///// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(CoreItem);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_getCurrentHealth(lua_State *L);    // Current health = FullHealth - damage sustained
   S32 lua_getFullHealth(lua_State *L);       // Health with no damange
   S32 lua_setFullHealth(lua_State *L);     
};



} /* namespace Zap */
#endif /* COREGAME_H_ */
