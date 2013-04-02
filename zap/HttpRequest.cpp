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

const string HttpRequest::UnreservedCharacters =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_~";

const string HttpRequest::GetMethod = "GET";
const string HttpRequest::PostMethod = "POST";

HttpRequest::HttpRequest(string url)
   : mUrl(url), mMethod("GET")
{
}

HttpRequest::HttpRequest(char* url)
{
   mUrl = string(url);
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

// Note, according to http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010, sprintf and
// _snprintf are not quite the same, though the differences do not matter here.
#if _MSC_VER
#  define snprintf _snprintf
#endif

   
   // construct the request
   mRequest = "";

   // request line
   mRequest += mMethod + " " + location + " HTTP/1.0";

   // content type and data encoding for POST requests
   if(mMethod == PostMethod) {
      mRequest += "\r\nContent-Type: application/x-www-form-urlencoded";

      string encodedData;
      map<string, string>::iterator it;
      for(it = mData.begin(); it != mData.end(); it++) {
         encodedData += urlEncode((*it).first) + "=" + urlEncode((*it).second) + "&";
      }

      char contentLengthHeaderBuffer[1024];
      dSprintf(contentLengthHeaderBuffer, 1024, "\r\nContent-Length: %d", encodedData.length());

      mRequest += contentLengthHeaderBuffer;
      mRequest += "\r\n\r\n";
      mRequest += encodedData;
   } else {
      // just the separator
      mRequest += "\r\n\r\n";
   }

   // send request
   while(true)
   {
      Platform::sleep(5);
      NetError sendError;
      sendError = mSocket->send((unsigned char *) mRequest.c_str(), mRequest.size());

      if(sendError == WouldBlock)
      {
         // need to wait
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

string HttpRequest::urlEncode(const string& str) {
   string result;
   string::const_iterator it;

   for(it = str.begin(); it < str.end(); it++) {
      if(UnreservedCharacters.find(*it) == string::npos) {
         char buffer[4];
         dSprintf(buffer, 16, (const char*) "%%%0.2x", ((S32) *it) & 0xFF);
         result += buffer;
      } else {
         result += *it;
      }
   }
   return result;
}

void HttpRequest::setData(const string& key, const string& value) {
   mData.erase(key);
   mData[key] = value;
}

void HttpRequest::setMethod(const string& method) {
   mMethod = method;
}

}
