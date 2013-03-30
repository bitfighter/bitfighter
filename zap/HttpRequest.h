#ifndef _HttpRequest_H_
#define _HttpRequest_H_

#include <tnl.h>
#include <tnlUDP.h>
#include <string>

using namespace std;
using namespace TNL;

namespace Zap
{

class HttpRequest {
   public:
   static const int OK = 200;

   HttpRequest(string url);
   bool send();
   string getResponse();
   string getResponseBody();
   int getResponseCode();
   string netErrorToString(NetError err);
   void parseResponse();

   private:
   string mUrl;
   string mRequest;
   string mResponse;
   string mResponseHead;
   string mResponseBody;
   int mResponseCode;
   TNL::Address* mAddress;
   TNL::Socket* mSocket;
};

}

#endif