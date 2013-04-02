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

#ifndef _HttpRequest_H_
#define _HttpRequest_H_

#include <tnl.h>
#include <tnlUDP.h>
#include <map>
#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

class HttpRequest {
public:
   static const int BufferSize = 4096;
   static const int OK = 200;
   static const string UnreservedCharacters;
   static const string GetMethod;
   static const string PostMethod;

   explicit HttpRequest(string url);
   explicit HttpRequest(char* url);

   static string urlEncode(const string& str);

   bool send();
   string getResponseBody();
   int getResponseCode();
   void setMethod(const string&);
   void setData(const string& key, const string& value);

private:
   void parseResponse();

   string mUrl;
   string mRequest;
   string mResponse;
   string mResponseHead;
   string mResponseBody;
   string mMethod;
   int mResponseCode;
   TNL::Address* mAddress;
   TNL::Socket* mSocket;
   map<string, string> mData;
};

}

#endif