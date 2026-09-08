//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef MOCKSOCKET_H_
#define MOCKSOCKET_H_

#include "tnlUDP.h"

#include <string>

namespace Zap
{

using namespace TNL;
using namespace std;

/**
 * Mock class for testing low level network functions
 *
 * {connect,send,receive}Error sets the return value of the mocked function.
 * isWritableResult controls Socket::isWritable (HttpRequest::send waits on it).
 */
class MockSocket : public Socket
{
   public:
   // the error returned by mocked network functions
   NetError connectError;
   NetError sendError;
   NetError receiveError;
   // the data to pretend we've received
   string data;
   bool dataSent;
   bool isWritableResult;

   MockSocket()
      : Socket(Address(), 0, 0, false, true),
        dataSent(false),
        connectError(NoError),
        sendError(NoError),
        receiveError(NoError),
        isWritableResult(true)
   { }

   NetError connect(const Address &address) { return connectError; }
   NetError send(const U8 *data, S32 size) { return sendError; }

   // Avoid the real select()-based wait (five seconds of hang on unconnected fds)
   bool isWritable(U32 timeout = 0) { return isWritableResult; }

   /**
    * Copies the contents of this->data to buffer and sets bytesRead.
    * The size argument is ignored and the entire data is written at once.
    * After the function has been called, it will act as though the socket was
    * closed cleanly by the remote host.
    */
   NetError recv(U8 *buffer, S32 size, S32 *bytesRead)
   {
      if(!dataSent)
      {
         data.copy((char*) buffer, data.length());
         buffer[data.length()] = 0;
         *bytesRead = data.length();
         dataSent = true;
      }
      else
      {
         *bytesRead = 0;
      }
      return receiveError;
   }
};
  
};

#endif /* MOCKSOCKET_H_ */
