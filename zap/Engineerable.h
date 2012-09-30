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

#ifndef ENGINEERABLE_H_
#define ENGINEERABLE_H_

#include "Point.h"
#include "tnlNetBase.h"

namespace Zap
{

// Forward declaraions
class MountableItem;
class GridDatabase;

// Things you can build with Engineer
enum EngineerBuildObjects
{
   EngineeredTurret,
   EngineeredForceField,
   EngineeredTeleporterEntrance,
   EngineeredTeleporterExit,
   EngineeredItemCount
};

// These are events used with engineering an object over the client/server protocols
enum EngineerResponseEvents {
   EngineerEventTurretBuilt,
   EngineerEventForceFieldBuilt,
   EngineerEventTeleporterEntranceBuilt,
   EngineerEventTeleporterExitBuilt,
   EngineerEventCount
};

// This class is sort of an interface for engineer-specific members and methods
class Engineerable
{
protected:
   bool mEngineered;
   SafePtr<MountableItem> mResource;

public:
   Engineerable();           // Constructor
   virtual ~Engineerable();  // Destructor

   void setEngineered(bool isEngineered);
   bool isEngineered(); // Was this engineered by a player?

   void setResource(MountableItem *resource);
   void releaseResource(const Point &releasePos, GridDatabase *database);

   virtual void computeExtent() = 0;  // The object must have extents recomputed before being added
   virtual void onConstructed() = 0;  // Call this once the object has been added to the game
};


}


#endif /* ENGINEERABLE_H_ */
