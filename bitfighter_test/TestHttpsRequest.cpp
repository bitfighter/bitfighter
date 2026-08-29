//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "MockSocket.h"
#include "MockAddress.h"

#include "../zap/HttpsRequest.h"

#include "gtest/gtest.h"
#include "tnlLog.h"

#include <string>
#include <memory>

namespace Zap
{

class HttpsRequestTest : public testing::Test
{
   public:
   HttpsRequest req;
   shared_ptr<MockSocket> sock;

   HttpsRequestTest()
      : req("/")
   {
   }

   virtual ~HttpsRequestTest()
   {
   }

   void plantMocks()
   {
      sock.reset(new MockSocket());
      req.mSocket = sock;
      req.mLocalAddress.reset(new MockAddress());
      req.mRemoteAddress.reset(new MockAddress());
      req.mUseMockSocket = true;
   }
};


TEST_F(HttpsRequestTest, urlTest)
{
   req.setUrl("example.com/test");

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("GET /test"));
}


TEST_F(HttpsRequestTest, urlEncodeTest)
{
   string result = req.urlEncode("test string-._~!@#$%^&*()=+{}[]|:;'<>,/?\\\"");

   string expected = "test%20string-._~%21%40%23%24%25%5e%26%2a%28%29%3d%2b%7b%7d%5b%5d%7c%3a%3b%27%3c%3e%2c%2f%3f%5c%22";
   EXPECT_EQ(expected, result);
}

TEST_F(HttpsRequestTest, postData)
{
   unsigned char data[] = "data";
   req.addFile("fieldName", "filename.txt", data, sizeof(data));
   req.setData("testKey", "testValue");
   req.setMethod(HttpsRequest::PostMethod);

   string result = req.buildRequest();
   EXPECT_NE(string::npos, result.find("Content-Disposition: form-data; name=\"testKey\"\r\n\r\ntestValue\r\n--"));
   EXPECT_NE(string::npos, result.find("Content-Disposition: form-data; name=\"fieldName\"; filename=\"filename.txt\""));
}


TEST_F(HttpsRequestTest, goodResponse)
{
   string response = "HTTP/1.1 200 OK\r\n\r\nresponse body";
   req.parseResponse(response);

   EXPECT_STREQ("HTTP/1.1 200 OK", req.getResponseHead().c_str());
   EXPECT_STREQ("response body", req.getResponseBody().c_str());
   EXPECT_EQ(200, req.getResponseCode());
}


TEST_F(HttpsRequestTest, badResponse)
{
   string response = "garbage";
   req.parseResponse(response);

   EXPECT_EQ(0, req.getResponseCode());
}

TEST_F(HttpsRequestTest, separatorOnlyResponse)
{
   req.parseResponse("\r\n\r\n");
   EXPECT_EQ(0, req.getResponseCode());
}

TEST_F(HttpsRequestTest, emptyResponse)
{
   req.parseResponse("");
   EXPECT_EQ(0, req.getResponseCode());
}


TEST_F(HttpsRequestTest, sendSuccess)
{
   plantMocks();
   sock->sendError = NoError;
   EXPECT_TRUE(req.sendRequest(string("test")));
}


TEST_F(HttpsRequestTest, sendTimeout)
{
   plantMocks();
   sock->sendError = WouldBlock;

   // don't really want to wait for a default timeout, so set the timeout
   // to two polling intervals
   req.setTimeout(HttpsRequest::PollInterval * 2);
   EXPECT_FALSE(req.sendRequest(string("test")));
}


TEST_F(HttpsRequestTest, receiveSuccess)
{
   plantMocks();
   sock->receiveError = NoError;
   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpsRequestTest, receiveTimeout)
{
   plantMocks();
   sock->receiveError = WouldBlock;
   req.setTimeout(HttpsRequest::PollInterval * 2);
   EXPECT_STREQ("", req.receiveResponse().c_str());
}


TEST_F(HttpsRequestTest, connectError)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->connectError = UnknownError;
   EXPECT_FALSE(req.send());
   EXPECT_EQ("Connect error", req.getError());
}


TEST_F(HttpsRequestTest, notWritable)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->isWritableResult = false;
   EXPECT_FALSE(req.send());
   EXPECT_EQ("Socket not writable", req.getError());
}


TEST_F(HttpsRequestTest, sendError)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->sendError = UnknownError;
   EXPECT_FALSE(req.send());
   EXPECT_EQ("Can't send request", req.getError());
}


TEST_F(HttpsRequestTest, receiveFail)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   sock->receiveError = UnknownError;
   EXPECT_FALSE(req.send());
   EXPECT_EQ("No response", req.getError());
}


TEST_F(HttpsRequestTest, successTest)
{
   plantMocks();
   sock->data = "HTTP/1.1 200 OK\r\n\r\nresponse";
   ASSERT_TRUE(req.send()) << req.getError();
   EXPECT_EQ(200, req.getResponseCode());
   EXPECT_EQ("response", req.getResponseBody());
}

TEST_F(HttpsRequestTest, blockPlaintextCredentials)
{
   HttpsRequest plainReq("http://example.com/login");
   plainReq.setData("password", "secret123");
   EXPECT_FALSE(plainReq.send());
   EXPECT_EQ("Insecure transmission of credentials over plaintext HTTP blocked", plainReq.getError());
}

};

