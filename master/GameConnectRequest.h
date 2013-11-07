//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAME_CONNECT_REQUEST_H_
#define _GAME_CONNECT_REQUEST_H_

#include "tnlTypes.h"
#include "tnlNetBase.h"

using namespace TNL;

namespace Master {

class MasterServerConnection;

class GameConnectRequest
{
public:
   SafePtr<MasterServerConnection> initiator;
   SafePtr<MasterServerConnection> host;

   U32 initiatorQueryId;
   U32 hostQueryId;
   U32 requestTime;
};


}

#endif