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
// irendereditt under the terms of the GNU General Public License as published by
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

#ifndef _HUNTERSGAME_H_
#define _HUNTERSGAME_H_

#include "gameType.h"
#include "item.h"
#include "polygon.h"

namespace Zap
{

class Ship;
class HuntersFlagItem;
class HuntersNexusObject;

class HuntersGameType : public GameType
{
private:
   typedef GameType Parent;

   S32 mNexusClosedTime;      // Time Nexus remains closed
   S32 mNexusOpenTime;        // Time Nexus remains open
   Timer mNexusTimer;

   struct YardSaleWaypoint
   {
      Timer timeLeft;
      Point pos;
   };

   Vector<YardSaleWaypoint> mYardSaleWaypoints;
   Vector<SafePtr<HuntersNexusObject> > mNexus;

public:
   HuntersGameType();      // Constructor
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString() const;

   bool mNexusIsOpen;      // Is the nexus open?
   S32 getNexusTimeLeft() {return mNexusTimer.getCurrent(); }

   // Info about this game type:
   //bool isTeamGame() { return getGame()->getTeamCount() > 1; }
   bool isFlagGame() { return true; }         // Well, technically not, but we'll morph flags to our own uses as we load the level
   bool isTeamFlagGame() { return false; }    // Ditto... team info will be ignored... no need to show warning in editor

   bool isSpawnWithLoadoutGame() { return true; }

   void shipTouchFlag(Ship *ship, FlagItem *flag);

   bool isCarryingItems(Ship *ship);
   void itemDropped(Ship *ship, Item *item);

   const char **getGameParameterMenuKeys();
   boost::shared_ptr<MenuItem> getMenuItem(ClientGame *game, const char *key);
   bool saveMenuItem(const MenuItem *menuItem, const char *key);

   void addNexus(HuntersNexusObject *theObject);
   void shipTouchNexus(Ship *ship, HuntersNexusObject *nexus);
   void onGhostAvailable(GhostConnection *connection);
   void idle(GameObject::IdleCallPath path, U32 deltaT);
   void renderInterfaceOverlay(bool scoreboardVisible);

   void controlObjectForClientKilled(GameConnection *theClient, GameObject *clientObject, GameObject *killerObject);
   void spawnShip(GameConnection *theClient);
   GameTypes getGameType() { return NexusGame; }
   const char *getGameTypeString() const { return "Nexus"; }     // Official game name
   const char *getShortName() const { return "N"; }
   const char *getInstructionString() { return "Collect flags from opposing players and bring them to the Nexus!"; }
   bool canBeTeamGame() { return true; }
   bool canBeIndividualGame() { return true; }

   U32 getLowerRightCornerScoreboardOffsetFromBottom() const { return 88; }

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   enum {
      DefaultNexusReturnDelay = 60000,
      DefaultNexusCapDelay = 15000,
      YardSaleWaypointTime = 5000,
      YardSaleCount = 8,
   };

   // Message types
   enum {
      HuntersMsgScore,
      HuntersMsgYardSale,
      HuntersMsgGameOverWin,
      HuntersMsgGameOverTie,
   };

   TNL_DECLARE_RPC(s2cAddYardSaleWaypoint, (F32 x, F32 y));
   TNL_DECLARE_RPC(s2cSetNexusTimer, (U32 nexusTime, bool isOpen));
   TNL_DECLARE_RPC(s2cHuntersMessage, (U32 msgIndex, StringTableEntry clientName, U32 flagCount, U32 score));

   TNL_DECLARE_CLASS(HuntersGameType);
};


class HuntersFlagItem : public FlagItem
{
private:
   typedef FlagItem Parent;

   void dropFlags(U32 flags);

protected:
   enum MaskBits {
      FlagCountMask = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1,
   };

public:
   HuntersFlagItem(Point pos = Point(), Point vel = Point(0,0), bool useDropDelay = false);     // Constructor

   void renderItem(Point pos);

   void onMountDestroyed();
   void onItemDropped();

   bool isItemThatMakesYouVisibleWhileCloaked() { return false; }

   void changeFlagCount(U32 change) { mFlagCount = change; setMaskBits(FlagCountMask); }
   U32 getFlagCount() { return mFlagCount; }

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool isAtHome() { return false; }               // Nexus flags have no home, and are thus never there
   void sendHome() { /* Do nothing */ }            // Nexus flags have no home, and can thus never be sent there

   TNL_DECLARE_CLASS(HuntersFlagItem);
};


class HuntersNexusObject : public EditorPolygon
{
private:
   typedef GameObject Parent;

public:
   HuntersNexusObject();      // Constructor
   HuntersNexusObject *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);

   void onAddedToGame(Game *theGame);
   void idle(GameObject::IdleCallPath path);

   void render();
   void renderDock();

   S32 getRenderSortValue() { return -1; }

   bool getCollisionPoly(Vector<Point> &polyPoints) const;
   bool collide(GameObject *hitObject);

   U32 packUpdate(GhostConnection *connection, U32 mask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_RPC(s2cFlagsReturned, ());    // Alert the Nexus object that flags have been returned to it

   TNL_DECLARE_CLASS(HuntersNexusObject);


   /////
   // Editor methods
   const char *getEditorHelpString() { return "Area to bring flags in Hunter game.  Cannot be used in other games."; }
   const char *getPrettyNamePlural() { return "Nexii"; }
   const char *getOnDockName() { return "Nexus"; }
   const char *getOnScreenName() { return "Nexus"; }
   string toString(F32 gridSiz) const;
   
   void renderEditor(F32 currentScale);


   HuntersNexusObject(lua_State *L) { /* Do nothing */ };   //  Lua constructor
   static const char className[];                 // Class name as it appears to Lua scripts
   static Lunar<HuntersNexusObject>::RegType methods[];
   GameObject *getGameObject() { return this; }   // Return the underlying GameObject
   S32 getClassID(lua_State *L) { return returnInt(L, NexusTypeNumber); }
private:
   void push(lua_State *L) {  Lunar<HuntersNexusObject>::push(L, this); }
};

};

#endif  // _HUNTERSGAME_H_

