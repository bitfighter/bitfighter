//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "tnlMethodDispatch.h"
#include "tnlNetStringTable.h"
#include "tnlThread.h"

namespace Types
{
void read(TNL::BitStream &s, TNL::StringPtr *val)
{
   char buffer[256];
   s.readString(buffer);
   *val = buffer;
}

void write(TNL::BitStream &s, TNL::StringPtr &val)
{
   s.writeString(val.getString());
}

void read(TNL::BitStream &s, TNL::ByteBufferPtr *val)
{
   TNL::U32 size = s.readInt(ByteBufferSizeBitSize);
   *val = new TNL::ByteBuffer(size);
   s.read(size, (*val)->getBuffer());
}
void write(TNL::BitStream &s, TNL::ByteBufferPtr &val)
{
   s.writeInt(val->getBufferSize(), ByteBufferSizeBitSize);
   s.write(val->getBufferSize(), val->getBuffer());
}
void read(TNL::BitStream &s, TNL::IPAddress *val)
{
   s.read(&val->netNum);
   s.read(&val->port);
}

void write(TNL::BitStream &s, TNL::IPAddress &val)
{
   s.write(val.netNum);
   s.write(val.port);
}


};

namespace TNL
{


};
