//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _MASTER_TYPES_H_
#define _MASTER_TYPES_H_

#include "tnlTypes.h"

#include <utility>      // For pair


namespace Zap
{

// Returned by master server during server query
class ServerAddr
{
public:
	IPAddress ipAddress {};
	S32 id = 0;
	StringTableEntry serverName {};

	ServerAddr(const IPAddress &ipAddress)         : ipAddress(ipAddress) {}
	ServerAddr(const IPAddress &ipAddress, S32 id) : ipAddress(ipAddress), id(id) {}
	ServerAddr(const IPAddress &ipAddress, S32 id,
		       const StringTableEntry& serverName) : ipAddress(ipAddress), id(id), serverName(serverName) {}
};

}


#endif