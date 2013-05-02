#ifndef MOCKSOCKET_H_
#define MOCKSOCKET_H_

#include "tnlUDP.h"

#include <string>

using namespace TNL;
using namespace std;

/**
 * Mock class for testing low level network functions
 *
 * {connect,send,receive}Error sets the return value of the mocked function
 *
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

   MockSocket()
      : Socket(Address(), 0, 0, false, true),
        dataSent(false),
        connectError(UnknownError),
        sendError(NoError),
        receiveError(NoError)
   { }

   NetError connect(const Address &address) { return connectError; };
   NetError send(const U8 *data, S32 size) { return sendError; }

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
         buffer[data.length()] = NULL;
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

#endif /* MOCKSOCKET_H_ */
