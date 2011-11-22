
#ifndef _DATACONNECTION_H_
#define _DATACONNECTION_H_

#include "tnlEventConnection.h"
#include "tnlRPC.h"
#include "tnlString.h"

using namespace TNL;
using namespace std;

//#include <iosfwd>
//#include <iostream>
//#include <fstream>    // linux complains about missing fprintf, fputc, vscanf, ...
//#include <stdlib.h>


namespace Zap {

class DataConnection;
class GameSettings;

enum ActionType {
   SEND_FILE,
   REQUEST_FILE,
   REQUEST_CURRENT_LEVEL,
   NO_ACTION
};

enum FileType {
   BOT_TYPE,
   LEVEL_TYPE,
   LEVELGEN_TYPE,
   FILE_TYPES,
   INVALID_RESOURCE_TYPE
};

enum SenderStatus {
   STATUS_OK,
   COULD_NOT_OPEN_FILE,
   COULD_NOT_FIND_FILE,
   FILE_TOO_LONG,
   COMMAND_NOT_ALLOWED,
   SENDER_STATUS_COUNT
};


////////////////////////////////////
////////////////////////////////////

// Interface class
class DataSendable
{
public:
   DataSendable();           // Constructor
   virtual ~DataSendable();  // Destructor

   TNL_DECLARE_RPC_INTERFACE(s2rSendLine, (StringPtr line));      // Send a chunk of data
   TNL_DECLARE_RPC_INTERFACE(s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status));   // Signal that data has been sent
};


////////////////////////////////////
////////////////////////////////////

class GameSettings;
struct FolderManager;

class DataSender 
{
private:
   bool mDone;
   S32 mLineCtr;
   Vector<string> mLines;           // Store strings because storing char * will cause problems when source string is gone
   SafePtr<Object> mConnection;     // need to use SafePtr, as it is possible that a player disconnect making it no longer valid
   FileType mFileType;

public:
   DataSender();        // Constructor
   SenderStatus initialize(DataSendable *connection, FolderManager *folderManager, string filename, FileType fileType);   

   bool isDone();
   void sendNextLine();
};


////////////////////////////////////////
////////////////////////////////////////

class DataConnection : public EventConnection, public DataSendable
{

private:
   ActionType mAction;        // Action user wants to undertake
   FileType mFileType;
   string mFilename;          
   string mPassword;          // Password supplied by user
   FILE *mOutputFile;         // Where we'll save any incoming data

   Nonce mClientId;           // When called from an active connection, client ID can be used to deterimine if player
                              // has sufficient permissions

   bool connectionsAllowed();

   GameSettings *mSettings;

public:
   // Constructors
   DataConnection(GameSettings *settings = NULL, ActionType action = NO_ACTION, string password = "", string filename = "", FileType fileType = LEVELGEN_TYPE);
   DataConnection(GameSettings *settings, const Nonce &clientId);

   DataSender mDataSender;
   void onConnectionEstablished();
   void onConnectionTerminated(NetConnection::TerminationReason, const char *);

   static string getErrorMessage(SenderStatus stat, const string &filename);

   // These from the DataSendable interface class
   TNL_DECLARE_RPC(s2rSendLine, (StringPtr line));
   TNL_DECLARE_RPC(s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status));

   TNL_DECLARE_RPC(s2cOkToSend, ());

   TNL_DECLARE_RPC(c2sSendOrRequestFile, (StringPtr password, RangedU32<0,(U32)FILE_TYPES> filetype, bool isRequest, StringPtr name));
   TNL_DECLARE_NETCONNECTION(DataConnection);
};


};

#endif
