//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "HttpsRequest.h"
#include "Intervals.h"

#include <curl/curl.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

using namespace std;
using namespace TNL;

namespace Zap
{

const string HttpsRequest::GetMethod = "GET";
const string HttpsRequest::PostMethod = "POST";
const string HttpsRequest::HttpsRequestBoundary = "---REQUEST---BOUNDARY---";

const string HttpsRequest::LevelDatabaseBaseUrl = "https://bitfighter.org/pleiades";

static size_t curlWriteBodyCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   string *responseBody = static_cast<string*>(userdata);
   size_t total = size * nmemb;
   static const size_t MaxHttpResponseSize = 10 * 1024 * 1024;
   if(responseBody->size() + total > MaxHttpResponseSize)
      return 0;
   responseBody->append(ptr, total);
   return total;
}

static size_t curlWriteHeaderCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   string *responseHead = static_cast<string*>(userdata);
   size_t total = size * nmemb;
   responseHead->append(ptr, total);
   return total;
}

HttpsRequest::HttpsRequest(const string &url)
   : mUrl(url),
     mMethod("GET"),
     mResponseCode(0),
     mTimeout(THREE_SECONDS),
     mUseMockSocket(false)
{
   mLocalAddress.reset(new Address(TCPProtocol, Address::Any, 0));
   mSocket.reset(new Socket(*mLocalAddress));
   setUrl(url);
}

// Destructor
HttpsRequest::~HttpsRequest()
{

}

bool HttpsRequest::sendViaCurl()
{
   CURL *curl = curl_easy_init();
   if(!curl)
   {
      mError = "Failed to initialize cURL";
      return false;
   }

   string effectiveUrl = mUrl;
   if(effectiveUrl.find("://") == string::npos)
      effectiveUrl = "https://" + effectiveUrl;

   // Enforce HTTPS - never allow plaintext HTTP if credentials might be transmitted
   if(effectiveUrl.rfind("http://", 0) == 0)
   {
      if(mData.find("password") != mData.end() || mData.find("pass") != mData.end())
      {
         mError = "Insecure transmission of credentials over plaintext HTTP blocked";
         curl_easy_cleanup(curl);
         return false;
      }
   }

   curl_easy_setopt(curl, CURLOPT_URL, effectiveUrl.c_str());
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)mTimeout);
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "Bitfighter");

   mResponseBody = "";
   mResponseHead = "";
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteBodyCallback);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mResponseBody);
   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlWriteHeaderCallback);
   curl_easy_setopt(curl, CURLOPT_HEADERDATA, &mResponseHead);

   curl_mime *mime = NULL;

   if(mMethod == PostMethod)
   {
      mime = curl_mime_init(curl);
      for(map<string, string>::iterator it = mData.begin(); it != mData.end(); it++)
      {
         curl_mimepart *part = curl_mime_addpart(mime);
         curl_mime_name(part, it->first.c_str());
         curl_mime_data(part, it->second.c_str(), it->second.size());
      }
      for(list<HttpsRequestFileInfo>::iterator it = mFiles.begin(); it != mFiles.end(); it++)
      {
         curl_mimepart *part = curl_mime_addpart(mime);
         curl_mime_name(part, it->fieldName.c_str());
         curl_mime_filename(part, it->fileName.c_str());
         curl_mime_data(part, (const char*)it->data, it->length);
         curl_mime_type(part, "image/png");
      }
      curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
   }

   CURLcode res = curl_easy_perform(curl);
   if(res != CURLE_OK)
   {
      mError = curl_easy_strerror(res);
      if(mime) curl_mime_free(mime);
      curl_easy_cleanup(curl);
      return false;
   }

   long http_code = 0;
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
   mResponseCode = (S32)http_code;

   if(mime) curl_mime_free(mime);
   curl_easy_cleanup(curl);

   return (mResponseCode >= 200 && mResponseCode < 400);
}


void HttpsRequest::setUrl(const string &url)
{
   mUrl = url;

   string workingUrl = mUrl;
   if(workingUrl.compare(0, 8, "https://") == 0)
      workingUrl = workingUrl.substr(8);
   else if(workingUrl.compare(0, 7, "http://") == 0)
      workingUrl = workingUrl.substr(7);

   // hostname is anything before the first '/'
   size_t index = workingUrl.find('/');
   string host = (index == string::npos) ? workingUrl : workingUrl.substr(0, index);

   string addressString;
   if(host.find(':') != string::npos)
      addressString = "ip:" + host;
   else if(mUrl.compare(0, 8, "https://") == 0)
      addressString = "ip:" + host + ":443";
   else
      addressString = "ip:" + host + ":80";

   mRemoteAddress.reset(new Address(addressString.c_str()));
}


bool HttpsRequest::send()
{
   mError = "";

   if(!mUseMockSocket)
   {
      return sendViaCurl();
   }

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
string HttpsRequest::getError()
{
   return mError;
}


string HttpsRequest::getResponseBody()
{
   return mResponseBody;
}


string HttpsRequest::getResponseHead()
{
   return mResponseHead;
}


S32 HttpsRequest::getResponseCode()
{
   return mResponseCode;
}


void HttpsRequest::parseResponse(string response)
{
   std::size_t separatorIndex = response.find("\r\n\r\n");
   if(separatorIndex == string::npos || response == "")
   {
      // separator not found, this response isn't valid
      return;
   }

   mResponseHead = response.substr(0, separatorIndex);

   size_t bodyIndex = separatorIndex + 4;
   mResponseBody = response.substr(bodyIndex, response.length());

   std::size_t responseCodeStart = mResponseHead.find(" ") + 1;
   std::size_t responseCodeEnd = mResponseHead.find("\r\n", responseCodeStart);
   string responseCode = mResponseHead.substr(responseCodeStart, responseCodeEnd - responseCodeStart);
   mResponseCode = atoi(responseCode.c_str());
}


string HttpsRequest::urlEncodeChar(char c)
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
      dSprintf(buffer, sizeof(buffer), (const char*) "%%%0.2x", (U32) ordinal & 0xFF);
      result = buffer;
   }
   return result;
}


string HttpsRequest::urlEncode(const string& str)
{
   string result;
   string::const_iterator it;

   for(it = str.begin(); it < str.end(); it++)
   {
      result += urlEncodeChar(*it);
   }
   return result;
}


void HttpsRequest::setData(const string& key, const string& value)
{
   mData.erase(key);
   mData[key] = value;
}


void HttpsRequest::setMethod(const string& method)
{
   mMethod = method;
}


string HttpsRequest::buildRequest()
{
   // location is anything that comes after the hostname in mUrl
   string workingUrl = mUrl;
   if(workingUrl.compare(0, 8, "https://") == 0)
      workingUrl = workingUrl.substr(8);
   else if(workingUrl.compare(0, 7, "http://") == 0)
      workingUrl = workingUrl.substr(7);

   size_t index = workingUrl.find('/');
   string location = (index == string::npos) ? "/" : workingUrl.substr(index);

   // request line
   mRequest = mMethod + " " + location + " HTTP/1.0";

   // content type and data encoding for POST requests
   if(mMethod == PostMethod)
   {
      stringstream encodedData("");
      for(map<string, string>::iterator it = mData.begin(); it != mData.end(); it++)
      {
         encodedData << "--" + HttpsRequestBoundary + "\r\n";
         encodedData << "Content-Disposition: form-data; name=\"" + (*it).first + "\"\r\n\r\n";
         encodedData << (*it).second + "\r\n";
      }

      for(list<HttpsRequestFileInfo>::iterator it = mFiles.begin(); it != mFiles.end(); it++)
      {
         stringstream fileData;
         fileData.write((const char*) (*it).data, (*it).length);

         encodedData << "--" + HttpsRequestBoundary + "\r\n";
         encodedData << "Content-Disposition: form-data; name=\"" + (*it).fieldName + "\"; filename=\"" + (*it).fileName + "\"\r\n";
         encodedData << "Content-Type: image/png\r\n";
         encodedData << "Content-Transfer-Encoding: binary\r\n\r\n";
         encodedData << fileData.str();
         encodedData << "\r\n";
      }

      encodedData << "--" + HttpsRequestBoundary + "\r\n";

      char contentLengthHeaderBuffer[1024] = { 0 };
      dSprintf(contentLengthHeaderBuffer, 1024, "\r\nContent-Length: %d", (U32) encodedData.tellp());

      mRequest += contentLengthHeaderBuffer;
      mRequest += "\r\nUser-Agent: Bitfighter";
      mRequest += "\r\nContent-Type: multipart/form-data, boundary=" + HttpsRequestBoundary;
      mRequest += "\r\n\r\n";
      mRequest += encodedData.str();
   }
   else
   {
      mRequest += "\r\n\r\n";
   }

   return mRequest;
}


bool HttpsRequest::sendRequest(string request)
{
   static const U32 bytesAtOnce = 512;
   unsigned char sendBuffer[bytesAtOnce];

   U32 bytesSent = 0, bytesTotal = request.size();
   U32 startTime = Platform::getRealMilliseconds();

   while(Platform::getRealMilliseconds() - startTime < mTimeout)
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
         startTime = Platform::getRealMilliseconds();

         if(bytesSent < bytesTotal)
            continue;

         return true;
      }

      // an error occurred
      return false;
   }
   return false;
}


string HttpsRequest::receiveResponse()
{
   mResponse = "";
   U32 startTime = Platform::getRealMilliseconds();

   static const U32 MaxHttpResponseSize = 10 * 1024 * 1024; // 10MB limit

   while(Platform::getRealMilliseconds() - startTime < mTimeout)
   {
      Platform::sleep(50);
      TNL::NetError recvError;
      S32 bytesRead = 0;
      char receiveBuffer[HttpsRequest::BufferSize] = { 0 };
      recvError = mSocket->recv((unsigned char*) receiveBuffer, HttpsRequest::BufferSize, &bytesRead);

      // Need to wait
      if(recvError == TNL::WouldBlock)
         continue;

      // There was an error, ignore partial responses
      if(recvError == TNL::UnknownError)
      {
         mResponse = "";
         break;
      }

      if(mResponse.size() + bytesRead > MaxHttpResponseSize)
      {
         mResponse = "";
         mError = "Response exceeds maximum size limit";
         break;
      }

      mResponse.append(receiveBuffer, 0, bytesRead);
      startTime = Platform::getRealMilliseconds();

      if(bytesRead == 0)
         break;
   }
   return mResponse;
}

void HttpsRequest::setTimeout(U32 timeout)
{
   mTimeout = timeout;
}

void HttpsRequest::addFile(string field, string filename, const U8* data, U32 length)
{
   HttpsRequestFileInfo info;
   info.fieldName = field;
   info.fileName = filename;
   info.data = data;
   info.length = length;
   mFiles.push_back(info);
}

}
