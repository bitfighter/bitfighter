//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

/**
 * @luaenum EngineerBuildObject(1,2)
 * The EngineerBuildObject enum represents different things you can build with the engineer module.
 */

//                Enum                    Enum in Lua            Nice name for docs
#define ENGINEER_BUILD_OBJECTS_TABLE \
   ENGR_OBJ(EngineeredTurret,             "Turret",              "Turret"               ) \
   ENGR_OBJ(EngineeredForceField,         "ForceFieldProjector", "Force Field Projector") \
   ENGR_OBJ(EngineeredTeleporterEntrance, "Teleporter",          "Teleporter"           ) \
   ENGR_OBJ(EngineeredTeleporterExit,     "TelerporterExit",     "Teleporter exit point") \

// Define an enum from the first values in EVENT_TABLE
enum EngineerBuildObjects {
#define ENGR_OBJ(a, b, c) a,
    ENGINEER_BUILD_OBJECTS_TABLE
#undef ENGR_OBJ
    EngineeredItemCount
};


// These are events used with engineering an object over the client/server protocols
enum EngineerResponseEvent {
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
