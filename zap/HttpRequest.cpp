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

#include "HttpRequest.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace TNL;
namespace Zap
{

HttpRequest::HttpRequest(string url)
   : mUrl(url)
{
}

bool HttpRequest::send()
{
   // hostname is anything before the first slash
   TNL::U32 index = mUrl.find('/');
   string host = mUrl.substr(0, index);

   // location is anything that comes after
   string location = mUrl.substr(index, mUrl.length() - index);

   mAddress = new Address(TCPProtocol, Address::Any, 0);
   mSocket = new Socket(*mAddress);

   // TNL address strings are of the form transport:hostname:port
   string addressString = "ip:" + host + ":80";
   TNL::Address remoteAddress(addressString.c_str());

   // check that TNL understands the supplied address
   if(!remoteAddress.isValid())
   {
      return false;
   }

   // initiate the connection. this will block if DNS resolution is required
   TNL::NetError connectError = mSocket->connect(remoteAddress);

   // construct the request
   string requestTemplate = "GET %s HTTP/1.0\r\n\r\n";
   char formattedRequest[2048];
   snprintf(formattedRequest, 1024, requestTemplate.c_str(), location.c_str());
   mRequest = formattedRequest;

   // send request
   while(true)
   {
      Platform::sleep(50);
      NetError sendError;
      sendError = mSocket->send((unsigned char *) mRequest.c_str(), mRequest.size());

      if(sendError == WouldBlock)
      {
         continue;
      }
      else if(sendError == NoError)
      {
         break;
      }

      return false;
   }

   while(true)
   {
      Platform::sleep(50);
      TNL::NetError recvError;
      int bytesRead;
      char receiveBuffer[HttpRequest::BufferSize];
      recvError = mSocket->recv((unsigned char*) receiveBuffer, HttpRequest::BufferSize, &bytesRead);

      if(recvError == TNL::WouldBlock)
      {
         // need to wait
         continue;
      }

      mResponse.append(receiveBuffer, 0, bytesRead);

      if(bytesRead == 0)
      {
         parseResponse();
         return true;
      }
   }
}

string HttpRequest::getResponseBody()
{
   return mResponseBody;
}

int HttpRequest::getResponseCode()
{
   return mResponseCode;
}

void HttpRequest::parseResponse()
{
   int seperatorIndex = mResponse.find("\r\n\r\n");
   mResponseHead = mResponse.substr(0, seperatorIndex);

   int bodyIndex = seperatorIndex + 4;
   mResponseBody = mResponse.substr(bodyIndex, mResponse.length());

   int responseCodeStart = mResponseHead.find(" ") + 1;
   int responseCodeEnd = mResponseHead.find("\r\n", responseCodeStart);
   string responseCode = mResponseHead.substr(responseCodeStart, responseCodeEnd - responseCodeStart);
   mResponseCode = atoi(responseCode.c_str());
}

}
