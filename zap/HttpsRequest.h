//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef HTTPSREQUEST_H_
#define HTTPSREQUEST_H_

#include <tnl.h>
#include <tnlNetBase.h>
#include <tnlUDP.h>
#include <map>
#include <list>
#include <string>
#include <memory>

using namespace std;
using namespace TNL;

namespace Zap
{

struct HttpsRequestFileInfo
{
   string fileName;
   string fieldName;
   const U8* data;
   U32 length;
};

class HttpsRequestTest;
class HttpsRequest
{
public:
   static const S32 BufferSize = 4096;
   static const S32 OK = 200;
   static const S32 Found = 302;
   static const string GetMethod;
   static const string PostMethod;
   static const S32 PollInterval = 20;
   static const string HttpsRequestBoundary;

   static const string LevelDatabaseBaseUrl;

   static string urlEncodeChar(char c);
   static string urlEncode(const string &str);

   HttpsRequest(const string &url = "/");
   virtual ~HttpsRequest();

   void addFile(string field, string filename, const U8* data, U32 length);
   string buildRequest();
   string getResponseBody();
   S32 getResponseCode();
   string getResponseHead();
   string getError();
   void parseResponse(string response);
   void setData(const string& key, const string& value);
   void setMethod(const string&);
   void setTimeout(U32 timeout);
   void setUrl(const string& url);
   bool send();

   bool sendRequest(string request);
   string receiveResponse();

private:
   shared_ptr<Address> mLocalAddress;
   shared_ptr<Address> mRemoteAddress;
   shared_ptr<Socket> mSocket;

   map<string, string> mData;
   list<HttpsRequestFileInfo> mFiles;
   string mUrl;
   string mMethod;
   string mRequest;
   string mResponse;
   string mResponseHead;
   string mResponseBody;
   S32 mResponseCode;
   U32 mTimeout;
   string mError;

   friend class HttpsRequestTest;
};

}

#endif /* HTTPSREQUEST_H_ */
