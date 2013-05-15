#include "MockSocket.h"
#include "MockAddress.h"

#include "../zap/HttpRequest.h"

#include "boost/shared_ptr.hpp"
#include "gtest/gtest.h"
#include "tnlLog.h"

#include <string>

namespace Zap
{

using boost::shared_ptr;

class HttpRequestTest : public testing::Test
{
   public:
   HttpRequest req;
   shared_ptr<MockSocket> sock;

   HttpRequestTest()
      : req("/")
   {
   }

   virtual ~HttpRequestTest()
   {
   }

   void plantMocks()
   {
      sock.reset(new MockSocket());
      req.mSocket = sock;
      req.mLocalAddress.reset(new MockAddress());
      req.mRemoteAddress.reset(new MockAddress());
   }
};


TEST_F(HttpRequestTest, urlTest)
{
   req.setUrl("example.com/test");

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("GET /test"));
}


TEST_F(HttpRequestTest, urlEncodeTest)
{
   string result = req.urlEncode("test string-._~!@#$%^&*()=+{}[]|:;'<>,/?\\\"");

   string expected = "test%20string-._~%21%40%23%24%25%5e%26%2a%28%29%3d%2b%7b%7d%5b%5d%7c%3a%3b%27%3c%3e%2c%2f%3f%5c%22";
   EXPECT_EQ(expected, result);
}

TEST_F(HttpRequestTest, postData)
{
   unsigned char data[] = "data";
   req.addFile("fieldName", "filename.txt", data, sizeof(data));
   req.setData("testKey", "testValue");
   req.setMethod(HttpRequest::PostMethod);

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("Content-Disposition: form-data; name=\"testKey\"\r\n\r\ntestValue\r\n--"));
   EXPECT_NE(string::npos, result.find("Content-Disposition: form-data; name=\"fieldName\"; filename=\"filename.txt\"\r\n\r\ndata\r\n"));
}


TEST_F(HttpRequestTest, goodResponse)
{
   string response = "HTTP/1.1 200 OK\r\n\r\nresponse body";
   req.parseResponse(response);

   EXPECT_STREQ("HTTP/1.1 200 OK", req.getResponseHead().c_str());
   EXPECT_STREQ("response body", req.getResponseBody().c_str());
   EXPECT_EQ(200, req.getResponseCode());
}


TEST_F(HttpRequestTest, badResponse)
{
   string response = "garbage";
   req.parseResponse(response);

   EXPECT_EQ(0, req.getResponseCode());
}

TEST_F(HttpRequestTest, separatorOnlyResponse)
{
   req.parseResponse("\r\n\r\n");
   EXPECT_EQ(0, req.getResponseCode());
}

TEST_F(HttpRequestTest, emptyResponse)
{
   req.parseResponse("");
   EXPECT_EQ(0, req.getResponseCode());
}


TEST_F(HttpRequestTest, sendSuccess)
{
   plantMocks();
   sock->sendError = NoError;
   EXPECT_TRUE(req.sendRequest(string("test")));
}


TEST_F(HttpRequestTest, sendTimeout)
{
   plantMocks();
   sock->sendError = WouldBlock;

   // don't really want to wait for a default timout, so set the timeout
   // to two polling intervals
   req.setTimeout(HttpRequest::PollInterval * 2);
   EXPECT_FALSE(req.sendRequest(string("test")));
}


TEST_F(HttpRequestTest, receiveSuccess)
{
   plantMocks();
   sock->receiveError = NoError;
   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpRequestTest, receiveTimeout)
{
   plantMocks();
   sock->receiveError = WouldBlock;
   req.setTimeout(HttpRequest::PollInterval * 2);
   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpRequestTest, connectError)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   // XXX: connect returns UnknownError on a successful call
   sock->connectError = NoError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, sendError)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->sendError = UnknownError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, receiveFail)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->receiveError = UnknownError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, successTest)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   EXPECT_TRUE(req.send());
}

};

