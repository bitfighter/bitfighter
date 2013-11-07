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


