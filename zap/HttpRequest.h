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

#include "boost/shared_ptr.hpp"
#include <tnl.h>
#include <tnlNetBase.h>
#include <tnlUDP.h>
#include <map>
#include <list>
#include <string>

using boost::shared_ptr;
using namespace std;
using namespace TNL;

namespace Zap
{

struct HttpRequestFileInfo
{
   string fileName;
   string fieldName;
   const U8* data;
   U32 length;
};

class HttpRequestTest;
class HttpRequest
{
public:
   static const S32 BufferSize = 4096;
   static const S32 OK = 200;
   static const S32 Found = 302;
   static const string GetMethod;
   static const string PostMethod;
   static const S32 PollInterval = 20;
   static const string HttpRequestBoundary;

   static string urlEncodeChar(char c);
   static string urlEncode(const string& str);

   HttpRequest(string url = "/");
   virtual ~HttpRequest();

   void addFile(string field, string filename, const U8* data, U32 length);
   string buildRequest();
   string getResponseBody();
   S32 getResponseCode();
   string getResponseHead();
   void parseResponse(string response);
   void setData(const string& key, const string& value);
   void setMethod(const string&);
   void setTimeout(U32 timeout);
   void setUrl(const string& url);
   bool send();

   bool sendRequest(string request);
   string receiveResponse();

private:
   boost::shared_ptr<Address> mLocalAddress;
   boost::shared_ptr<Address> mRemoteAddress;
   boost::shared_ptr<Socket> mSocket;

   map<string, string> mData;
   list<HttpRequestFileInfo> mFiles;
   string mUrl;
   string mMethod;
   string mRequest;
   string mResponse;
   string mResponseHead;
   string mResponseBody;
   S32 mResponseCode;
   U32 mTimeout;

   friend class HttpRequestTest;
};

}

#endif /* HTTPREQUEST_H_ */
