#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_VECTOR_H_
#include "tnlVector.h"
#endif

#ifndef _TNL_BITSTREAM_H_
#include "tnlBitStream.h"
#endif

#ifndef _TNL_NETSTRINGTABLE_H_
#include "tnlNetStringTable.h"
#endif

#ifndef _TNL_STRING_H_
#include "tnlString.h"
#endif

using namespace TNL;

struct GameStatistics3
{
StringTableEntry gameType;
bool teamGame;
StringTableEntry levelName;
Vector<StringTableEntry> teams;
Vector<S32> teamScores;
Vector<RangedU32<0,0xFFFFFF> > color;
U16 timeInSecs;
Vector<StringTableEntry> playerNames;
Vector<Vector<U8> > playerIDs;
Vector<bool> isBot;
Vector<bool> lastOnTeam;
Vector<S32> playerScores;
Vector<U16> playerKills;
Vector<U16> playerDeaths;
Vector<U16> playerSuicides;
Vector<Vector<U16> > shots;
Vector<Vector<U16> > hits;
};

namespace Types
{
   extern void read(TNL::BitStream &s, GameStatistics3 *val);
   extern void write(TNL::BitStream &s, GameStatistics3 &val);
}

