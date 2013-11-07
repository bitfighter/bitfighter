//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

   static const string LevelDatabaseBaseUrl;

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
