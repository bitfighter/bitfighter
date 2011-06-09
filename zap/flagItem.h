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

#ifndef _FLAGITEM_H_
#define _FLAGITEM_H_

#include "gameItems.h"     // For FlagSpawn def
#include "ship.h"

namespace Zap
{
////////////////////////////////////////
////////////////////////////////////////

class FlagItem : public EditorItem
{
protected:
   typedef EditorItem Parent;         // <=== needs to be protected as this is accessed by child in form of Parent::Parent

private:
   Point mInitialPos;                 // Where flag was "born"
   bool mIsAtHome;

protected:
   U32 mFlagCount;                    // How many flags does this represet?

public:
   FlagItem(Point pos = Point());                                    // C++ constructor
   FlagItem(Point pos, bool collidable, float radius, float mass);   // Alternate C++ constructor

   void initialize();      // Set inital values of things

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   virtual string toString();

   virtual void onAddedToGame(Game *theGame);
   virtual void renderItem(Point pos);

   void mountToShip(Ship *theShip);

   virtual void sendHome();

   virtual void onMountDestroyed();
   virtual bool collide(GameObject *hitObject);

   bool isAtHome() { return mIsAtHome; }

   Timer mTimer;                       // Used for games like HTF where time a flag is held is important

   virtual U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   virtual void unpackUpdate(GhostConnection *connection, BitStream *stream);
   virtual void idle(GameObject::IdleCallPath path);


   TNL_DECLARE_CLASS(FlagItem);

   ///// Editor stuff

   void renderDock();
   F32 getEditorRadius(F32 currentScale);

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() { return "Flag item, used by a variety of game types."; }  
   const char *getPrettyNamePlural() { return "Flags"; }
   const char *getOnDockName() { return "Flag"; }
   const char *getOnScreenName() { return "Flag"; }
   bool hasTeam() { return true; }
   bool canBeHostile() { return true; }
   bool canBeNeutral() { return true; }

   ///// Lua Interface

   FlagItem(lua_State *L) { /* Do nothing */ };    //  Lua constructor

   static const char className[];
   static Lunar<FlagItem>::RegType methods[];

   S32 getClassID(lua_State *L) { return returnInt(L, FlagType); }
   
   S32 getTeamIndx(lua_State *L) { return returnInt(L, mTeam + 1); }          // Index of owning team
   S32 isInInitLoc(lua_State *L) { return returnBool(L, isAtHome()); }        // Is flag in it's initial location?

   void push(lua_State *L) { Lunar<FlagItem>::push(L, this); }

};


};

#endif


