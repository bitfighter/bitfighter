
#ifndef _DATACONNECTION_H_
#define _DATACONNECTION_H_

#include "tnlEventConnection.h"
#include "tnlRPC.h"

using namespace TNL;
using namespace std;

#include <iosfwd>
#include <iostream>
#include <fstream>
#include <stdlib.h>


namespace Zap {

class DataConnection;

enum ActionType {
   SEND_FILE,
   REQUEST_FILE,
   NO_ACTION
};

enum FileType {
   BOT_TYPE,
   LEVEL_TYPE,
   LEVELGEN_TYPE,
   FILE_TYPES,
   INVALID_RESOURCE_TYPE
};


class DataSender 
{
private:
   bool mDone;
   S32 mLineCtr;
   Vector<std::string> mLines;              // Store strings because storing char * will cause problems when source string is gone
   DataConnection *mDataConnection;
   FileType mFileType;

public:
   enum SenderStatus {
      OK,
      COULD_NOT_OPEN_FILE,
      COULD_NOT_FIND_FILE,
      FILE_TOO_LONG,
   };

   DataSender() { mDone = true; }        // Constructor 
   SenderStatus initialize(DataConnection *dataConnection, string filename, FileType fileType);   

   bool isDone() { return mDone; }
   void sendNextLine();

   
};

////////////////////////////////////////
////////////////////////////////////////

class DataConnection : public EventConnection
{

private:
   ActionType mAction;        // Action user wants to undertake
   FileType mFileType;
   string mFilename;          
   string mPassword;          // Password supplied by user
   ofstream mOutputFile;      // Where we'll save any incoming data

public:
   // Quickie Constructor
   DataConnection(ActionType action = NO_ACTION, string password = "", string filename = "", FileType fileType = LEVELGEN_TYPE) 
   { 
      mAction = action; 
      mFilename = filename; 
      mFileType = fileType;
      mPassword = password;
   }     

   DataSender mDataSender;
   void onConnectionEstablished();
   void onConnectionTerminated(TerminationReason, const char *);

   TNL_DECLARE_RPC(s2rSendLine, (StringPtr line));
   TNL_DECLARE_RPC(s2cOkToSend, ());
   TNL_DECLARE_RPC(c2sCommandComplete, ());

   TNL_DECLARE_RPC(c2sSendOrRequestFile, (StringPtr password, RangedU32<0,U32(FILE_TYPES)> filetype, bool isRequest, StringPtr name));

   TNL_DECLARE_NETCONNECTION(DataConnection);
};



};

#endif
