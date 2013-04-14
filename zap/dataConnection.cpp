//#include "gameLoader.h"                   // For MAX_LEVEL_FILE_LENGTH def  <-- there has to be a better way!

#include "dataConnection.h"
#include "tnlEventConnection.h"
#include "ServerGame.h"
#include "gameNetInterface.h"             // for GetGame() through GameNetInterface

#include "tnl.h"
#include "tnlLog.h"
#include "tnlVector.h"
#include "tnlNetBase.h"
#include "tnlHuffmanStringProcessor.h"    // For HuffmanStringProcessor::MAX_SENDABLE_LINE_LENGTH

#include "stringUtils.h"
#include "config.h"                       // For configDirs

#include "md5wrapper.h"                   // For password verification

using namespace TNL;
//using namespace std;

namespace Zap {


FileType getResourceType(const char *fileType)
{
   if(!stricmp(fileType, "bot"))
      return BOT_TYPE;

   if(!stricmp(fileType, "level"))
      return LEVEL_TYPE;

   if(!stricmp(fileType, "levelgen"))
      return LEVELGEN_TYPE;

   return INVALID_RESOURCE_TYPE;
}
   

static string getFullFilename(const FolderManager *configDirs, string filename, FileType fileType)
{
   // Don't return "" if empty directory, allow client load the file if on the same directory as EXE.
   string name;
   if(fileType == BOT_TYPE)
      name = configDirs->findBotFile(filename);

   else if(fileType == LEVEL_TYPE)
      name = configDirs->findLevelFile(filename);

   else if(fileType == LEVELGEN_TYPE)
      name = configDirs->findLevelGenScript(filename);

   else
      name = "";

   return (name == "") ? filename : name;
}


static string getOutputFolder(FolderManager *folderManager, FileType filetype)
{
   if(filetype == BOT_TYPE) 
      return folderManager->robotDir;
   else if(filetype == LEVEL_TYPE) 
      return folderManager->levelDir;
   else if(filetype == LEVELGEN_TYPE) 
      return folderManager->levelDir;
   else return "";
}


extern md5wrapper md5;
extern bool writeToConsole();
extern void exitToOs(S32 errcode);
extern DataConnection *dataConn;

void transferResource(GameSettings *settings, const string &addr, const string &pw, const string &fileName, const string &resourceType, bool sending)
{
   writeToConsole();

   Address address(addr.c_str());

   if(!address.isValid())
   {
      printf("Invalid address: Use format IP:nnn.nnn.nnn.nnn:port\n");
      exitToOs(1);
   }

   string password = md5.getSaltedHashFromString(pw);

   FileType fileType = getResourceType(resourceType.c_str());
   if(fileType == INVALID_RESOURCE_TYPE)
   {
      printf("Invalid resource type: Please sepecify BOT, LEVEL, or LEVELGEN\n");
      exitToOs(1);
   }

   dataConn = new DataConnection(settings, sending ? SEND_FILE : REQUEST_FILE, password, fileName, fileType);

   NetInterface *netInterface = new NetInterface(Address());
   dataConn->connect(netInterface, address);

   bool started = false;

   while(!started || (dataConn && dataConn->isEstablished()))
   {
      if(dataConn && dataConn->isEstablished())
      {
         if(!dataConn->mDataSender.isDone())
            dataConn->mDataSender.sendNextLine();

         started = true;
      }

      netInterface->checkIncomingPackets();
      netInterface->processConnections();
      Platform::sleep(1);                    // Don't eat CPU
      if((!started) && (!dataConn))
      {
         printf("Failed to connect");
         started = true;                     // Get out of this loop
      }
   }

   delete netInterface;
   delete dataConn;

   exitToOs(0);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
DataSendable::DataSendable()
{
   // Do nothing
}

// Destructor
DataSendable::~DataSendable()
{
   // Do nothing
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
DataSender::DataSender()
{
   mDone = true;
}


bool DataSender::isDone()
{
   return mDone;
}

// For readability
#define MAX_CHUNK_LEN HuffmanStringProcessor::MAX_SENDABLE_LINE_LENGTH

SenderStatus DataSender::initialize(DataSendable *connection, FolderManager *folderManager, string filename, FileType fileType)
{
   string fullname = getFullFilename(folderManager, filename, fileType);

   if(fullname == "")
      return COULD_NOT_FIND_FILE;

   //ifstream file;
   //file.open(fullname.c_str());
   FILE *file = fopen(fullname.c_str(), "r");

   //if(!file.is_open())
   if(!file)
      return COULD_NOT_OPEN_FILE;

   // Allocate a buffer
   //char *buffer = new char[MAX_CHUNK_LEN + 1];      // 255 for data, + 1 for terminator
   char buffer[MAX_CHUNK_LEN + 1];      // 255 for data, + 1 for terminator
   S32 size;

   // We'll read the file in 255 char chunks; this is the largest string we can send, and we want to be as large as possible to get
   // maximum benefit of the string compression that occurs during the transmission process.
   /*
   while(!file.eof() && mLines.size() * MAX_CHUNK_LEN < MAX_LEVEL_FILE_LENGTH)
   {
      file.read(buffer, MAX_CHUNK_LEN);
      if(file.gcount() > 0)
      {
         buffer[file.gcount()] = '\0';     // Null terminate
         mLines.push_back(buffer); 
      }
   }
   file.close();
   */

   const S32 MAX_LEVEL_FILE_LENGTH = 256 * 1024;     // 256K -- Need some limit to avoid overflowing server; arbitrary value

   size = (S32) fread(buffer, 1, MAX_CHUNK_LEN, file);

   while(size > 0 && mLines.size() * MAX_CHUNK_LEN < MAX_LEVEL_FILE_LENGTH)
   {
       buffer[size] = 0;           // Null terminate
       mLines.push_back(buffer);
       size = (S32) fread(buffer, 1, MAX_CHUNK_LEN, file);
   }

   fclose(file);

   //delete[] buffer;

   // Not exactly accurate -- if final line is only a few bytes, it will count as 255; but since our limit is arbitrary, it matters not
   if(mLines.size() * MAX_CHUNK_LEN >= MAX_LEVEL_FILE_LENGTH)
   {
      mLines.clear();
      return FILE_TOO_LONG;
   }

   if(mLines.size() == 0)          // Read nothing
      return COULD_NOT_OPEN_FILE;

   mConnection = dynamic_cast<Object *>(connection);
   mFileType = fileType;
   mDone = false;
   mLineCtr = 0;
   return STATUS_OK;
}

#undef MAX_CHUNK_LEN


// Send next line of our file
void DataSender::sendNextLine()
{
   DataSendable *connection = dynamic_cast<DataSendable *>(mConnection.getPointer());
   if(!connection)
      mDone = true;

   if(mDone)
      return;

   if(mLineCtr < mLines.size())
   {
      connection->s2rSendLine(mLines[mLineCtr].c_str());
      mLineCtr++;
   }
   else
   {
      connection->s2rCommandComplete(STATUS_OK);
      mDone = true;
      mLines.clear();      // Liberate some memory
   }
}


////////////////////////////////////////
////////////////////////////////////////


DataConnection::DataConnection(GameSettings *settings, ActionType action, string password, string filename, FileType fileType)
{
   mSettings = settings;
   mAction = action;
   mFilename = filename;
   mFileType = fileType;
   mPassword = password;

   mOutputFile = NULL;
}


DataConnection::DataConnection(GameSettings *settings, const Nonce &clientId)
{
   mSettings = settings;
   mClientId = clientId;

   mAction = REQUEST_CURRENT_LEVEL;
}



// static method
string DataConnection::getErrorMessage(SenderStatus stat, const string &filename)
{
   if(stat == COULD_NOT_OPEN_FILE)
      return "Could not open file " + filename;

   else if(stat == COULD_NOT_FIND_FILE)
      return "Could not find file " + filename;

   else if(stat == FILE_TOO_LONG)
      return "File " + filename + " is too big to send";

   else
      return "Unknown problem";
}


TNL_IMPLEMENT_NETCONNECTION(DataConnection, NetClassGroupGame, true);


extern md5wrapper md5;

// Client sends this message to set up the coming transfer.  Server checks for the password, and then, if the client is requesting
// a file, initiates the transfer.  If client is sending a file, it gets things ready then sends s2cOkToSend to indicate it's ready.
TNL_IMPLEMENT_RPC(DataConnection, c2sSendOrRequestFile, 
                  (StringPtr password, RangedU32<0,(U32)FILE_TYPES> filetype, bool isRequest, StringPtr filename),
                  (password, filetype, isRequest, filename), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   TNLAssert(dynamic_cast<GameNetInterface *>(getInterface()), "Not a GameNetInterface");
   TNLAssert(((GameNetInterface *)getInterface())->getGame()->isServer(), "Not a ServerGame");

   ServerGame *game = (ServerGame *)(((GameNetInterface *)getInterface())->getGame());
   GameSettings *settings = game->getSettings();

   // Check if data connections are allowed
   if(!settings->getIniSettings()->allowDataConnections)
   {
      logprintf("This server does not allow remote access to resources.  It can be enabled in the server's INI file.");
      disconnect(ReasonConnectionsForbidden, "");
      return;
   }

   // Check password.  Should be admin or owner
   string adminPW = settings->getAdminPassword();
   string ownerPW = settings->getOwnerPassword();

   bool goodOwnerPW = ownerPW != "" && strcmp(md5.getSaltedHashFromString(ownerPW).c_str(), password) == 0;
   bool goodAdminPW = adminPW != "" && strcmp(md5.getSaltedHashFromString(adminPW).c_str(), password) == 0;

   if(!goodOwnerPW && !goodAdminPW)
   {
      logprintf("Incorrect password!");
      disconnect(ReasonBadLogin, "Incorrect pasword");
      return;
   }

   // Process request
   if(isRequest)     // Client wants to get a file from us... they should have a file open and waiting for this data
   {
      // Initialize on the server to start sending requested file -- will return OK if everything is set up right
      SenderStatus stat = game->dataSender.initialize(this, settings->getFolderManager(), filename.getString(), (FileType)(U32)filetype);

      if(stat != STATUS_OK)
      {
         string msg = getErrorMessage(stat, filename.getString());

         logprintf("%s", msg.c_str());
         disconnect(ReasonError, msg.c_str());
         return;
      }
   }
   else              // Client wants to send us a file -- get ready for incoming data!
   {
      FolderManager *folderManager = settings->getFolderManager();

      string folder = getOutputFolder(folderManager, (FileType)(U32)filetype);

      if(folder == "")     // filetype was bogus, will probably never happen
      {
         logprintf("Error resolving folder!");
         disconnect(ReasonError, "Error resolving folder");
         return;
      }

      if(mOutputFile) 
         fclose((FILE*)mOutputFile);

      mOutputFile = fopen(strictjoindir(folder, filename.getString()).c_str(), "w");

      if(!mOutputFile)
      {
         logprintf("Problem opening file %s for writing", strictjoindir(folder, filename.getString()).c_str());
         disconnect(ReasonError, "Problem writing to file");
         return;
      }

      s2cOkToSend();
   }
}


// Server tells us it's ok to send... so start sending!
TNL_IMPLEMENT_RPC(DataConnection, s2cOkToSend, (), (), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   // Initialize on the client to start sending file we want to send
   SenderStatus stat = mDataSender.initialize(this, mSettings->getFolderManager(), mFilename.c_str(), mFileType);
   if(stat != STATUS_OK)
   {
      string msg = getErrorMessage(stat, mFilename);

      logprintf("%s", msg.c_str());
      disconnect(ReasonError, msg.c_str());
   }
}


// << DataSendable >>
// Send a chunk of the file -- this gets run on the receiving end       
TNL_IMPLEMENT_RPC(DataConnection, s2rSendLine, (StringPtr line), (line), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(mOutputFile)
      fwrite(line.getString(), 1, strlen(line.getString()), mOutputFile);
      //mOutputFile.write(line.getString(), strlen(line.getString()));
   // else... what?
}


// << DataSendable >>
// When client has finished sending its data, it sends a commandComplete message, which triggers the server to disconnect the client
TNL_IMPLEMENT_RPC(DataConnection, s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status), (status), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   disconnect(ReasonNone, "done");     // Terminate connection... should probably send different message depending on status

   if(mOutputFile)
   {
      fclose(mOutputFile);
      mOutputFile = NULL;
   }
}


void DataConnection::onConnectionEstablished()
{
   if(isInitiator())    // i.e. client
   {
      if(mAction == SEND_FILE)
      {
         c2sSendOrRequestFile(mPassword.c_str(), mFileType, false, mFilename.c_str());
      }

      else if(mAction == REQUEST_FILE)
      {
         TNLAssert(dynamic_cast<GameNetInterface *>(getInterface()), "Not a GameNetInterface");
         Game *game = static_cast<GameNetInterface *>(getInterface())->getGame();

         FolderManager *folderManager = game->getSettings()->getFolderManager();
         string folder = getOutputFolder(folderManager, mFileType);

         if(folder == "")     // filetype was bogus; should never happen
            logprintf("Error resolving folder!");      // But... we can save files without needing folder, so log and cary on

         //mOutputFile.open(strictjoindir(folder, mFilename).c_str());
         if(mOutputFile) 
            fclose(mOutputFile);
         mOutputFile = fopen(strictjoindir(folder, mFilename).c_str(), "w");
         if(!mOutputFile)
         {
            logprintf("Problem opening file %s for writing", strictjoindir(folder, mFilename).c_str());
            disconnect(ReasonError, "done");
            return;
         }

         c2sSendOrRequestFile(mPassword.c_str(), mFileType, true, mFilename.c_str());
      }
   }
}


// Make sure things are cleaned up -- will run on both client and server
void DataConnection::onConnectionTerminated(NetConnection::TerminationReason reason, const char *reasonMsg)
{
   if(mOutputFile)
   {
      fclose((FILE*)mOutputFile);
      mOutputFile = NULL;
   }

   if(isInitiator())    // i.e. client
   {
      if(reason == ReasonError)
         logprintf("Error sending file: %s", reasonMsg);
      else if(reason == ReasonConnectionsForbidden)                   
         logprintf("Data connections are disallowed on this server!");
   }
}

};
