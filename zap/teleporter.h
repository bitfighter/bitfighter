//-----------------------------------------------------------------------------------
//
// bitFighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
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

#include "gameConnection.h"
#include "gameObject.h"
#include "point.h"    
#include "../tnl/tnlNetObject.h"

namespace Zap
{

class Teleporter : public GameObject
{
private:
   S32 mLastDest;    // Destination of last ship through
   Point mPos;
   Vector<Point> mDest;

public:
   bool doSplash;
   U32 timeout;
   U32 mTime;

   enum {
      InitMask     = BIT(0),
      TeleportMask = BIT(1),

      TeleporterRadius        = 75,       // Overall size of the teleporter
      TeleporterTriggerRadius = 50,
      TeleporterDelay = 1500,             // Time teleporter remains idle after it has been used
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,
   };

   Teleporter();

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(GameObject::IdleCallPath path);
   void render();

   void onAddedToGame(Game *theGame);

   bool processArguments(S32 argc, const char **argv);
   Teleporter findTeleporterAt(Point pos);      // Find a teleporter at pos

   TNL_DECLARE_CLASS(Teleporter);
};

};
