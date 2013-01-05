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

#ifndef _NEXUSGAME_H_
#define _NEXUSGAME_H_

#include "gameType.h"
#include "item.h"
#include "polygon.h"
#include "LuaWrapper.h"
#include "Zone.h"

namespace Zap
{

class Ship;
class NexusFlagItem;
class NexusZone;

class NexusGameType : public GameType
{
private:
   typedef GameType Parent;

   S32 mNexusClosedTime;      // Time Nexus remains closed, in seconds
   S32 mNexusOpenTime;        // Time Nexus remains open, in seconds
   S32 mNexusChangeAtTime;    // When the next Nexus status change will occur  --> needs to be able go be negative

   struct YardSaleWaypoint
   {
      Timer timeLeft;
      Point pos;
   };

   bool nexusShouldChange();  // Is Nexus overdue for a change?

   Vector<YardSaleWaypoint> mYardSaleWaypoints;
   Vector<SafePtr<NexusZone> > mNexus;

   void idle_client(U32 deltaT);     // Idle for clients
   void idle_server(U32 deltaT);     // Idle for server

protected:
   void setTimeRemaining(U32 timeLeft, bool isUnlimited);                           // Runs on server
   void setTimeRemaining(U32 timeLeft, bool isUnlimited, S32 renderingOffset);      // Runs on client

public:
   NexusGameType();           // Constructor
   bool processArguments(S32 argc, const char **argv, Game *game);
   string toString() const;

   bool mNexusIsOpen;         // Is the nexus open?
   S32 getNexusTimeLeft();    // Get time until the nexus changes state


   bool isSpawnWithLoadoutGame();

   void shipTouchFlag(Ship *ship, FlagItem *flag);

   bool isCarryingItems(Ship *ship);
   void itemDropped(Ship *ship, MoveItem *item, MountableItem::Dismount_Mode dismountMode);

   void openNexus(S32 timeNexusOpened);
   void closeNexus(S32 timeNexusClosed);
   void setNexusState(bool open);

   void setNewOpenTime(S32 timeInSeconds);
   void setNewClosedTime(S32 timeInSeconds);

#ifndef ZAP_DEDICATED
   Vector<string> getGameParameterMenuKeys();
   boost::shared_ptr<MenuItem> getMenuItem(const string &key);
   bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   void addNexus(NexusZone *theObject);
   void shipTouchNexus(Ship *ship, NexusZone *nexus);
   void onGhostAvailable(GhostConnection *connection);
   void idle(BfObject::IdleCallPath path, U32 deltaT);

   static void releaseFlag(Game *game, const Point &pos, const Point &startVel = Point(0,0), S32 count = 1);

#ifndef ZAP_DEDICATED
   void renderInterfaceOverlay(bool scoreboardVisible);
#endif

   void controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject);
   bool spawnShip(ClientInfo *clientInfo);

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char *getInstructionString() const;

   // Info about this game type:
   bool isFlagGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


   U32 getLowerRightCornerScoreboardOffsetFromBottom() const;

   S32 getEventScore(ScoringGroup scoreGroup, ScoringEvent scoreEvent, S32 data);

   enum {
      DefaultNexusReturnDelay = 60000,
      DefaultNexusCapDelay = 15000,
      YardSaleWaypointTime = 5000,
   };

   // Message types
   enum {
      NexusMsgScore,
      NexusMsgYardSale,
      NexusMsgGameOverWin,
      NexusMsgGameOverTie,
   };

   TNL_DECLARE_RPC(s2cAddYardSaleWaypoint, (F32 x, F32 y));
   TNL_DECLARE_RPC(s2cSetNexusTimer, (S32 nextChangeTime, bool isOpen));
   TNL_DECLARE_RPC(s2cSendNexusTimes, (S32 nexusClosedTime, S32 nexusOpenTime));
   TNL_DECLARE_RPC(s2cNexusMessage, (U32 msgIndex, StringTableEntry clientName, U32 flagCount, U32 score));

   TNL_DECLARE_CLASS(NexusGameType);
};


////////////////////////////////////////
////////////////////////////////////////

class NexusFlagItem : public FlagItem
{
   typedef FlagItem Parent;

private:
   U32 mFlagCount;                    // How many flags does this represet?  When flag is mounted, could represent several

   void dropFlags(U32 flags);

protected:
   enum MaskBits {
      FlagCountMask = Parent::FirstFreeMask << 0,
      FirstFreeMask = Parent::FirstFreeMask << 1,
   };

public:
   NexusFlagItem(Point pos = Point(), Point vel = Point(0,0), S32 count = 0, bool useDropDelay = false);    // Constructor
   ~NexusFlagItem();                                                                                        // Destructor

   void renderItem(const Point &pos);
   void renderItemAlpha(const Point &pos, F32 alpha);

   void dismount(Dismount_Mode dismountMode);

   bool isItemThatMakesYouVisibleWhileCloaked();

   void changeFlagCount(U32 change);
   U32 getFlagCount();

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   bool isAtHome();            // Nexus flags have no home, and are thus never there
   void sendHome();            // Nexus flags have no home, and can thus never be sent there

   TNL_DECLARE_CLASS(NexusFlagItem);
};


////////////////////////////////////////
////////////////////////////////////////

class NexusZone : public GameZone
{
   typedef GameZone Parent;

private:
   void processArguments_ArchaicZapFormat(S32 argc, const char **argv, F32 gridSize);    // Helper for processArguments

public:
   NexusZone(lua_State *L = NULL);     // Combined Lua / C++ constructor
   ~NexusZone();                       // Destructor

   NexusZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Game *game);

   void onAddedToGame(Game *theGame);
   void idle(BfObject::IdleCallPath path);

   void render();
   void renderDock();

   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);

   // These services are used, but provided solely by parent
   //U32 packUpdate(GhostConnection *connection, U32 mask, BitStream *stream);
   //void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_RPC(s2cFlagsReturned, ());    // Alert the Nexus object that flags have been returned to it

   TNL_DECLARE_CLASS(NexusZone);


   /////
   // Editor methods
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();

   string toString(F32 gridSiz) const;

   bool hasTeam();     
   bool canBeHostile();
   bool canBeNeutral();
   
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(NexusZone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 isOpen(lua_State *L);
   S32 setOpen(lua_State *L);
   S32 setOpenTime(lua_State *L);
   S32 setClosedTime(lua_State *L);
};

};

#endif  // _NEXUSGAME_H_

