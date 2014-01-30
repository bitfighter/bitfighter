//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "HttpRequest.h"
#include "Intervals.h"

#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;
using namespace TNL;

namespace Zap
{

const string HttpRequest::GetMethod = "GET";
const string HttpRequest::PostMethod = "POST";
const string HttpRequest::HttpRequestBoundary = "---REQUEST---BOUNDARY---";

const string HttpRequest::LevelDatabaseBaseUrl = "bitfighter.org/pleiades";

HttpRequest::HttpRequest(const string &url)
   : mUrl(url), 
     mMethod("GET"), 
     mResponseCode(0), 
     mTimeout(THREE_SECONDS)
{
   mLocalAddress.reset(new Address(TCPProtocol, Address::Any, 0));
   mSocket.reset(new Socket(*mLocalAddress));
   setUrl(url);
}

// Destructor
HttpRequest::~HttpRequest()
{

}


void HttpRequest::setUrl(const string &url)
{
   mUrl = url;

   // hostname is anything before the first '/'
   TNL::U32 index = mUrl.find('/');
   string host = mUrl.substr(0, index);
   string addressString = "ip:" + host + ":80";

   mRemoteAddress.reset(new Address(addressString.c_str()));
}


bool HttpRequest::send()
{
   mError = "";

   // check that TNL understands the supplied address
   if(!mRemoteAddress->isValid())
   {
      mError = "Address invalid";
      return false;
   }

   S32 connectError = mSocket->connect(*mRemoteAddress);
   if(connectError == UnknownError)
   {
      mError = "Connect error";
	   return false;
   }
   
   if(!mSocket->isWritable(FIVE_SECONDS))
   {
      mError = "Socket not writable";
	   return false;
   }
 
   buildRequest();
   if(!sendRequest(mRequest))
   {
      mError = "Can't send request";
      return false;
   }

   string response = receiveResponse();
   if(response == "")
   {
      mError = "No response";
      return false;
   }

   parseResponse(response);
   if(getResponseCode() == 0)
   {
      mError = "Invalid response code";
      return false;
   }

   return true;
}


// Returns most recent error message
string HttpRequest::getError()
{
   return mError;
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
   std::size_t seperatorIndex = response.find("\r\n\r\n");
   if(seperatorIndex == string::npos || response == "")
   {
      // seperator not found, this response isn't valid
      return;
   }

   mResponseHead = response.substr(0, seperatorIndex);

   U32 bodyIndex = seperatorIndex + 4;
   mResponseBody = response.substr(bodyIndex, response.length());

   std::size_t responseCodeStart = mResponseHead.find(" ") + 1;
   std::size_t responseCodeEnd = mResponseHead.find("\r\n", responseCodeStart);
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

   // request line
   mRequest = mMethod + " " + location + " HTTP/1.0";

   // content type and data encoding for POST requests
   if(mMethod == PostMethod)
   {
      stringstream encodedData("");
      for(map<string, string>::iterator it = mData.begin(); it != mData.end(); it++)
      {
         encodedData << "--" + HttpRequestBoundary + "\r\n";
         encodedData << "Content-Disposition: form-data; name=\"" + (*it).first + "\"\r\n\r\n";
         encodedData << (*it).second + "\r\n";
      }

      for(list<HttpRequestFileInfo>::iterator it = mFiles.begin(); it != mFiles.end(); it++)
      {
         stringstream fileData;
         fileData.write((const char*) (*it).data, (*it).length);

         encodedData << "--" + HttpRequestBoundary + "\r\n";
         encodedData << "Content-Disposition: form-data; name=\"" + (*it).fieldName + "\"; filename=\"" + (*it).fileName + "\"\r\n";
         encodedData << "Content-Type: image/png\r\n";
         encodedData << "Content-Transfer-Encoding: binary\r\n\r\n";
         encodedData << fileData.str();
         encodedData << "\r\n";
      }

      encodedData << "--" + HttpRequestBoundary + "\r\n";

      char contentLengthHeaderBuffer[1024] = { 0 };
      dSprintf(contentLengthHeaderBuffer, 1024, "\r\nContent-Length: %d", (U32) encodedData.tellp());

      mRequest += contentLengthHeaderBuffer;
      mRequest += "\r\nUser-Agent: Bitfighter";
      mRequest += "\r\nContent-Type: multipart/form-data, boundary=" + HttpRequestBoundary;
      mRequest += "\r\n\r\n";
      mRequest += encodedData.str();
   }
   else
   {
      mRequest += "\r\n\r\n";
   }

   return mRequest;
}


bool HttpRequest::sendRequest(string request)
{
   static const U32 bytesAtOnce = 512;
   unsigned char sendBuffer[bytesAtOnce];

   U32 bytesSent = 0, bytesTotal = request.size();
   U32 startTime = Platform::getRealMilliseconds();

   bool sentData = false;
   // Continue to send indefinitely if data was successfully sent
   while(sentData || Platform::getRealMilliseconds() - startTime < mTimeout)
   {
      Platform::sleep(PollInterval);

      memcpy(sendBuffer, request.c_str() + bytesSent, min(bytesTotal - bytesSent, bytesAtOnce));
      NetError sendError;
      sendError = mSocket->send(sendBuffer, min(bytesTotal - bytesSent, bytesAtOnce));

      if(sendError == WouldBlock)
      {
         // need to wait
         continue;
      }
      else if(sendError == NoError)
      {
         // data was transmitted
         bytesSent += bytesAtOnce;

         sentData = true;

         if(bytesSent < bytesTotal)
            continue;

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

void HttpRequest::addFile(string field, string filename, const U8* data, U32 length)
{
   HttpRequestFileInfo info;
   info.fieldName = field;
   info.fileName = filename;
   info.data = data;
   info.length = length;
   mFiles.push_back(info);
}

}
