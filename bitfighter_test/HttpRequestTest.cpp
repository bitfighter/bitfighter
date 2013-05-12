#include "MockSocket.h"
#include "MockAddress.h"

#include "../zap/HttpRequest.h"

#include "gtest/gtest.h"
#include "tnlLog.h"

#include <string>

using namespace Zap;
class HttpRequestTest : public testing::Test
{
   public:
   HttpRequest req;
   MockSocket sock;

   HttpRequestTest()
      : sock(), req("/", &sock)
   {
   }
};


TEST_F(HttpRequestTest, urlTest)
{
   req = HttpRequest("example.com/test");

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("GET /test"));
}


TEST_F(HttpRequestTest, urlEncodeTest)
{
   string result = req.urlEncode("test string-._~!@#$%^&*()=+{}[]|:;'<>,/?\\\"");

   string expected = "test%20string-._~%21%40%23%24%25%5e%26%2a%28%29%3d%2b%7b%7d%5b%5d%7c%3a%3b%27%3c%3e%2c%2f%3f%5c%22";
   EXPECT_EQ(expected, result);
}


TEST_F(HttpRequestTest, postDataTest)
{
   req.setData("foo", "bar");
   req.setMethod(HttpRequest::PostMethod);

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("\r\n\r\nfoo=bar&"));
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
   sock.sendError = NoError;
   EXPECT_TRUE(req.sendRequest(string("test")));
}


TEST_F(HttpRequestTest, sendTimeout)
{
   sock.sendError = WouldBlock;

   // don't really want to wait for a default timout, so set the timeout
   // to two polling intervals
   req.setTimeout(HttpRequest::PollInterval * 2);
   EXPECT_FALSE(req.sendRequest(string("test")));
}


TEST_F(HttpRequestTest, receiveSuccess)
{
   sock.receiveError = NoError;

   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpRequestTest, receiveTimeout)
{
   sock.receiveError = WouldBlock;

   req.setTimeout(HttpRequest::PollInterval * 2);
   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpRequestTest, connectError)
{
   req = HttpRequest("/", &sock, NULL, new MockAddress());
   sock.data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   // XXX: connect returns UnknownError on a successful call
   sock.connectError = NoError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, sendError)
{
   req = HttpRequest("/", &sock, NULL, new MockAddress());
   sock.data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock.sendError = UnknownError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, receiveFail)
{
   req = HttpRequest("/", &sock, NULL, new MockAddress());
   sock.data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock.receiveError = UnknownError;
   EXPECT_FALSE(req.send());
}


TEST_F(HttpRequestTest, successTest)
{
   req = HttpRequest("/", &sock, NULL, new MockAddress());
   sock.data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   EXPECT_TRUE(req.send());
}


