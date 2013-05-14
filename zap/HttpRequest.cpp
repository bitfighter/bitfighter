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

const string HttpRequest::GetMethod = "GET";
const string HttpRequest::PostMethod = "POST";

HttpRequest::HttpRequest(string url)
   : mUrl(url), mMethod("GET"), mResponseCode(0), mTimeout(30000), mSocket(0), mLocalAddress(0), mRemoteAddress(0)
{
   mLocalAddress = new Address(TCPProtocol, Address::Any, 0);
   mSocket = new Socket(*mLocalAddress);
   setUrl(url);
}

// Destructor
HttpRequest::~HttpRequest()
{
   delete mLocalAddress;
   delete mSocket;
   delete mRemoteAddress;
}


void HttpRequest::setUrl(const string& url)
{
   // hostname is anything before the first '/'
   TNL::U32 index = mUrl.find('/');
   string host = mUrl.substr(0, index);
   string addressString = "ip:" + host + ":80";

   delete mRemoteAddress;
   mRemoteAddress = new Address(addressString.c_str());
}


bool HttpRequest::send()
{
   // check that TNL understands the supplied address
   if(!mRemoteAddress->isValid())
   {
      return false;
   }

   // initiate the connection. this will block if DNS resolution is required
   if(mSocket->connect(*mRemoteAddress) != UnknownError)
   {
      return false;
   }

   buildRequest();
   if(!sendRequest(mRequest))
   {
      return false;
   }

   string response = receiveResponse();
   if(response == "")
   {
      return false;
   }

   parseResponse(response);
   if(getResponseCode() == 0)
   {
      return false;
   }

   return true;
}


string HttpRequest::getResponseBody()
{
   return mResponseBody;
}


string HttpRequest::getResponseHead()
{
   return mResponseHead;
}


S32 HttpRequest::getResponseCode()
{
   return mResponseCode;
}


void HttpRequest::parseResponse(string response)
{
   U32 seperatorIndex = response.find("\r\n\r\n");
   if(seperatorIndex == string::npos || response == "")
   {
      // seperator not found, this response isn't valid
      return;
   }

   mResponseHead = response.substr(0, seperatorIndex);

   U32 bodyIndex = seperatorIndex + 4;
   mResponseBody = response.substr(bodyIndex, response.length());

   U32 responseCodeStart = mResponseHead.find(" ") + 1;
   U32 responseCodeEnd = mResponseHead.find("\r\n", responseCodeStart);
   string responseCode = mResponseHead.substr(responseCodeStart, responseCodeEnd - responseCodeStart);
   mResponseCode = atoi(responseCode.c_str());
}


string HttpRequest::urlEncodeChar(char c)
{
   U32 ordinal = c;
   string result;
   // see if the character is unreserved
   if(
      (ordinal >= 0x41 && ordinal <= 0x5A) || // lowercase
      (ordinal >= 0x61 && ordinal <= 0x7A) || // uppercase
      (ordinal >= 0x30 && ordinal <= 0x39) || // digits
      ordinal == 0x2D ||                      // hyphen
      ordinal == 0x2E ||                      // period
      ordinal == 0x5F ||                      // underscore
      ordinal == 0x7E                         // tilde
   )
   {
      result = ordinal;
   }
   else
   {
      char buffer[4];
      // Convert ordinal to a two character hex number in the range [0, 255],
      // prefixed by a percentage sign
      dSprintf(buffer, 16, (const char*) "%%%0.2x", (U32) ordinal & 0xFF);
      result = buffer;
   }
   return result;
}


string HttpRequest::urlEncode(const string& str)
{
   string result;
   string::const_iterator it;

   for(it = str.begin(); it < str.end(); it++)
   {
      result += urlEncodeChar(*it);
   }
   return result;
}


void HttpRequest::setData(const string& key, const string& value)
{
   mData.erase(key);
   mData[key] = value;
}


void HttpRequest::setMethod(const string& method)
{
   mMethod = method;
}


string HttpRequest::buildRequest()
{
   // location is anything that comes after the first '/'
   TNL::U32 index = mUrl.find('/');
   string location = mUrl.substr(index, mUrl.length() - index);

   // construct the request
   mRequest = "";

   // request line
   mRequest += mMethod + " " + location + " HTTP/1.0";

   // content type and data encoding for POST requests
   if(mMethod == PostMethod)
   {
      mRequest += "\r\nContent-Type: application/x-www-form-urlencoded";

      string encodedData;
      map<string, string>::iterator it;
      for(it = mData.begin(); it != mData.end(); it++)
      {
         encodedData += urlEncode((*it).first) + "=" + urlEncode((*it).second) + "&";
      }

      char contentLengthHeaderBuffer[1024];
      dSprintf(contentLengthHeaderBuffer, 1024, "\r\nContent-Length: %d", encodedData.length());

      mRequest += contentLengthHeaderBuffer;
      mRequest += "\r\n\r\n";
      mRequest += encodedData;
   }
   else
   {
      mRequest += "\r\n\r\n";
   }
   return mRequest;
}


bool HttpRequest::sendRequest(string request)
{
   U32 startTime = Platform::getRealMilliseconds();
   while(Platform::getRealMilliseconds() - startTime < mTimeout)
   {
      Platform::sleep(PollInterval);
      NetError sendError;
      sendError = mSocket->send((unsigned char *) mRequest.c_str(), mRequest.size());

      if(sendError == WouldBlock)
      {
         // need to wait
         continue;
      }
      else if(sendError == NoError)
      {
         // data was transmitted
         return true;
      }

      // an error occured
      return false;
   }
   return false;
}


string HttpRequest::receiveResponse()
{
   mResponse = "";
   S32 startTime = Platform::getRealMilliseconds();
   while(Platform::getRealMilliseconds() - startTime < mTimeout)
   {
      Platform::sleep(50);
      TNL::NetError recvError;
      S32 bytesRead = 0;
      char receiveBuffer[HttpRequest::BufferSize] = { 0 };
      recvError = mSocket->recv((unsigned char*) receiveBuffer, HttpRequest::BufferSize, &bytesRead);

      if(recvError == TNL::WouldBlock)
      {
         // need to wait
         continue;
      }

      if(recvError == TNL::UnknownError)
      {
         // there was an error, ignore partial responses
         mResponse = "";
         break;
      }

      mResponse.append(receiveBuffer, 0, bytesRead);

      if(bytesRead == 0)
      {
         break;
      }

      // more data to read
   }
   return mResponse;
}

void HttpRequest::setTimeout(U32 timeout)
{
   mTimeout = timeout;
}

}
