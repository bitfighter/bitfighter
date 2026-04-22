//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MOVE_H_
#define _MOVE_H_

#include "shipItems.h"     // For ShipModuleCount, ShipWeaponCount

namespace TNL {
   class BitStream;
}

using namespace TNL;

namespace Zap
{

static const S32 MaxXtankWeaponSlots = 6;

// Can represent a move by a human player or a robot
class Move
{
public:
   Move();              // Constructor
   Move(F32 x, F32 y, F32 angle = 0);  // Constructor used in tests
   virtual ~Move();

   void initialize();

   F32 x, y;
   F32 angle;
   bool fire;
   bool modulePrimary[ShipModuleCount];    // Is given module primary component active?
   bool moduleSecondary[ShipModuleCount];  // Is given module secondary component active?
   U32 time;
   S8 bodyIndex;     // -1 = normal BF ship; 0..N = xtank body index (matches XtankBody::Type enum)
   S8 weaponSlot[MaxXtankWeaponSlots];  // active XtankWeapon::Type per weapon number slot (-1 = None)
   S8 weaponMount[MaxXtankWeaponSlots]; // XtankMountLocation per weapon slot (-1 = None)
   S8 engineType;    // XtankEngine::Type; valid when bodyIndex >= 0
   S8 treadType;     // XtankTread::Type; valid when bodyIndex >= 0
   S8 heatSinkCount; // heat sink count (1-6); valid when bodyIndex >= 0
   S8 armorType;     // XtankArmor::Type; valid when bodyIndex >= 0
   S8 suspensionType; // XtankSuspension index; valid when bodyIndex >= 0
   S8 bumperType;     // XtankBumper index; valid when bodyIndex >= 0
   U16 specials;      // XtankSpecial bitmask; valid when bodyIndex >= 0
   U8  armorSides[4]; // per-side armor points (front=0,back=1,left=2,right=3); valid when bodyIndex >= 0

   static const S32 MaxMoveTime = 127;

   bool isAnyModActive() const;
   bool isEqualMove(const Move *move) const;    // Compares this move to the previous one -- are they the same?
   void pack(BitStream *stream, Move *prev, bool packTime);
   void unpack(BitStream *stream, bool unpackTime);
   void prepare();                  // Packs and unpacks move to ensure effects of rounding are same on client and server
   string toString();
   void set(F32 x, F32 y, F32 angle = 0);

};

};

#endif
