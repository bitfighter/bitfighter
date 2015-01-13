//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _NEXUSGAME_H_
#define _NEXUSGAME_H_

#include "gameType.h"
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

   S32 mNexusClosedTime;      // Time Nexus remains closed, in milliseconds
   S32 mNexusOpenTime;        // Time Nexus remains open, in milliseconds
   S32 mNexusChangeAtTime;    // When the next Nexus status change will occur  --> needs to be able go be negative

   bool mNexusIsOpen;         // Is the nexus open?

   struct YardSaleWaypoint
   {
      Timer timeLeft;
      Point pos;
   };

   bool nexusShouldChange();  // Is Nexus overdue for a change?

   Vector<YardSaleWaypoint> mYardSaleWaypoints;
   Vector<SafePtr<NexusZone> > mNexus;


   Vector<string> makeParameterMenuKeys() const;

   void idle_client(U32 deltaT);     // Idle for clients
   void idle_server(U32 deltaT);     // Idle for server

protected:

public:
   NexusGameType();           // Constructor
   virtual ~NexusGameType();

   bool processArguments(S32 argc, const char **argv, Level *level);
   string toLevelCode() const;

   S32 getNexusTimeLeftMs() const;     // Get time until the nexus changes state in MilliSeconds

   bool isSpawnWithLoadoutGame();

   void onOvertimeStarted();

   void shipTouchFlag(Ship *ship, FlagItem *flag);

   bool isCarryingItems(Ship *ship);
   void itemDropped(Ship *ship, MoveItem *item, DismountMode dismountMode);

   void openNexus(S32 timeNexusOpened);
   void closeNexus(S32 timeNexusClosed);
   void setNexusState(bool open);

   void setNewOpenTime(S32 timeInSeconds);
   void setNewClosedTime(S32 timeInSeconds);

#ifndef ZAP_DEDICATED
   const Vector<string> *getGameParameterMenuKeys() const;
   boost::shared_ptr<MenuItem> getMenuItem(const string &key) const;
   bool saveMenuItem(const MenuItem *menuItem, const string &key);
#endif

   void addNexus(NexusZone *theObject);
   void shipTouchNexus(Ship *ship, NexusZone *nexus);
   void onGhostAvailable(GhostConnection *connection);
   void idle(BfObject::IdleCallPath path, U32 deltaT);

   void releaseFlag(const Point &pos, const Point &vel = Point(0,0), S32 count = 1);

#ifndef ZAP_DEDICATED
   void renderInterfaceOverlay(S32 canvasWidth, S32 canvasHeight) const;
   S32 renderTimeLeftSpecial(S32 right, S32 bottom, bool render) const;
#endif

   void controlObjectForClientKilled(ClientInfo *theClient, BfObject *clientObject, BfObject *killerObject);
   bool spawnShip(ClientInfo *clientInfo);

   bool isNexusOpen() const;

   GameTypeId getGameTypeId() const;
   const char *getShortName() const;
   const char **getInstructionString() const;
   HelpItem getGameStartInlineHelpItem() const;

   // Info about this game type:
   bool isFlagGame() const;
   bool canBeTeamGame() const;
   bool canBeIndividualGame() const;


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
   virtual ~NexusFlagItem();                                                                                // Destructor

   void renderItem(const Point &pos) const;
   void renderItemAlpha(const Point &pos, F32 alpha) const;

   void dismount(DismountMode dismountMode);

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
   explicit NexusZone(lua_State *L = NULL);     // Combined Lua / C++ constructor
   virtual ~NexusZone();                        // Destructor

   NexusZone *clone() const;

   bool processArguments(S32 argc, const char **argv, Level *level);

   void onAddedToGame(Game *theGame);
   void idle(BfObject::IdleCallPath path);

   void render() const;
   void renderDock(const Color &color) const;

   const Vector<Point> *getCollisionPoly() const;
   bool collide(BfObject *hitObject);

   // These services are used, but provided solely by parent
   //U32 packUpdate(GhostConnection *connection, U32 mask, BitStream *stream);
   //void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_RPC(s2cFlagsReturned, ());    // Alert the Nexus object that flags have been returned to it

   TNL_DECLARE_CLASS(NexusZone);


   /////
   // Editor methods
   const char *getEditorHelpString() const;
   const char *getPrettyNamePlural() const;
   const char *getOnDockName() const;
   const char *getOnScreenName() const;

   string toLevelCode() const;

   bool hasTeam();     
   bool canBeHostile();
   bool canBeNeutral();
   
   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false) const;

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(NexusZone);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_isOpen(lua_State *L);
   S32 lua_setOpen(lua_State *L);
   S32 lua_setOpenTime(lua_State *L);
   S32 lua_setClosedTime(lua_State *L);
};

};

#endif  // _NEXUSGAME_H_

