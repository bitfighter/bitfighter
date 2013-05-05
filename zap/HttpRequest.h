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

#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

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
   static const S32 BufferSize = 4096;
   static const S32 OK = 200;
   static const S32 Found = 302;
   static const string GetMethod;
   static const string PostMethod;
   static const S32 PollInterval = 20;

   static string urlEncodeChar(char c);
   static string urlEncode(const string& str);

   HttpRequest(string url = "/", TNL::Socket* socket = NULL, TNL::Address* localAddress = NULL, TNL::Address* remoteAddress = NULL);

   string buildRequest();
   string getResponseBody();
   S32 getResponseCode();
   string getResponseHead();
   void parseResponse(string response);
   void setData(const string& key, const string& value);
   void setMethod(const string&);
   void setTimeout(U32 timeout);
   bool send();

   bool sendRequest(string request);
   string receiveResponse();

private:
   TNL::Address* mLocalAddress;
   TNL::Address* mRemoteAddress;
   map<string, string> mData;
   string mUrl;
   string mMethod;
   string mRequest;
   string mResponse;
   string mResponseHead;
   string mResponseBody;
   S32 mResponseCode;
   TNL::Socket* mSocket;
   U32 mTimeout;
};

}

#endif /* HTTPREQUEST_H_ */
