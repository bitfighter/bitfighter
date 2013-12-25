//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _CONN_STATS_RENDERER_
#define _CONN_STATS_RENDERER_

#include "tnlTypes.h"

using namespace TNL;

namespace Zap
{

class GameConnection;

namespace UI
{

class ConnectionStatsRenderer
{
   bool mVisible;
   bool mGraphVisible;

   static const U32 ArraySize = 32;
   U32 mSendSize[ArraySize];
   U32 mRecvSize[ArraySize];
   U32 mCurrentIndex;
   U32 mTime;
public:
   ConnectionStatsRenderer();      // Constructor
   ~ConnectionStatsRenderer();

   void reset();
   void idle(U32 timeDelta, GameConnection *conn);
   void render(GameConnection *conn) const;
   void toggleVisibility();
};

} } // Nested namespace


#endif

