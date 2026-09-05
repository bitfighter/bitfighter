//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "dataConnection.h"
#include "ServerGame.h"
#include "gameNetInterface.h"             // for GetGame() through GameNetInterface

#include "stringUtils.h"

using namespace TNL;


namespace Zap {


FileType getResourceType(const char *fileType)
{
   if(stricmp(fileType, "bot") == 0)
      return BOT_TYPE;

   if(stricmp(fileType, "level") == 0)
      return LEVEL_TYPE;

   if(stricmp(fileType, "levelgen") == 0)
      return LEVELGEN_TYPE;

   return INVALID_RESOURCE_TYPE;
}


static string getFullFilename(const FolderManager *configDirs, string filename, FileType fileType)
{
   if(!configDirs || extractFilename(filename) != filename || filename.find("..") != string::npos || filename.find('/') != string::npos || filename.find('\\') != string::npos)
      return "";

   string name;
   if(fileType == BOT_TYPE)
      name = configDirs->findBotFile(filename);

   else if(fileType == LEVEL_TYPE)
      name = configDirs->findLevelFile(filename);

   else if(fileType == LEVELGEN_TYPE)
      name = configDirs->findLevelGenScript(filename);

   else
      name = "";

   return name;
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


extern bool writeToConsole();
extern void exitToOs(S32 errcode);



void transferResource(GameSettings *settings, const string &addr, const string &pw, const string &fileName, const string &resourceType, bool sending)
{
   writeToConsole();

   Address address(addr.c_str());

   if(!address.isValid())
   {
      printf("Invalid address: Use format IP:nnn.nnn.nnn.nnn:port\n");
      exitToOs(1);
   }

   string password = pw;

   FileType fileType = getResourceType(resourceType.c_str());
   if(fileType == INVALID_RESOURCE_TYPE)
   {
      printf("Invalid resource type: Please specify BOT, LEVEL, or LEVELGEN\n");
      exitToOs(1);
   }

   DataConnection *dataConn;

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
   mLineCtr = 0;
   mFileType = INVALID_RESOURCE_TYPE;
}

// Destructor
DataSender::~DataSender()
{
   // Do nothing
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
   size_t size;

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

   const U32 MAX_LEVEL_FILE_LENGTH = 256 * 1024;     // 256K -- Need some limit to avoid overflowing server; arbitrary value

   size = fread(buffer, 1, MAX_CHUNK_LEN, file);

   while(size > 0 && mLines.size() * MAX_CHUNK_LEN < MAX_LEVEL_FILE_LENGTH)
   {
       buffer[size] = 0;           // Null terminate
       mLines.push_back(buffer);
       size = fread(buffer, 1, MAX_CHUNK_LEN, file);
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
   mBytesReceived = 0;
   mTargetPath = "";
   mTempPath = "";
   mTransferSuccess = false;
}


DataConnection::DataConnection(GameSettings *settings, const Nonce &clientId)
{
   mSettings = settings;
   mClientId = clientId;

   mAction = REQUEST_CURRENT_LEVEL;
   mFileType = INVALID_RESOURCE_TYPE;
   mOutputFile = NULL;
   mBytesReceived = 0;
   mTargetPath = "";
   mTempPath = "";
   mTransferSuccess = false;
}

// Destructor
DataConnection::~DataConnection()
{
   if(mOutputFile)
   {
      fclose((FILE*)mOutputFile);
      mOutputFile = NULL;
   }

   if(!mTransferSuccess && mTempPath != "")
   {
      remove(mTempPath.c_str());
   }
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


static string computeDataConnectionChallenge(const NetConnection *conn, const string &saltedPwdHash)
{
   if(!conn || saltedPwdHash.empty())
      return "";
   string tag = "Bitfighter:DataConnection:Auth:";
   string clientNonce = conn->getNonce().toString();
   string serverNonce = conn->getServerNonce().toString();
   return Game::md5.getHashFromString(tag + saltedPwdHash + ":" + clientNonce + ":" + serverNonce);
}

static string computeLegacyDataChallenge(const NetConnection *conn, const string &saltedPwdHash)
{
   if(!conn || saltedPwdHash.empty())
      return "";
   return Game::md5.getHashFromString(saltedPwdHash + conn->getNonce().toString());
}

TNL_IMPLEMENT_NETCONNECTION(DataConnection, NetClassGroupGame, true);


// Client sends this message to set up the coming transfer.  Server checks for the password, and then, if the client is requesting
// a file, initiates the transfer.  If client is sending a file, it gets things ready then sends s2cOkToSend to indicate it's ready.
TNL_IMPLEMENT_RPC(DataConnection, c2sSendOrRequestFile,
                  (StringPtr password, RangedU32<0,(U32)FILE_TYPES> filetype, bool isRequest, StringPtr filename),
                  (password, filetype, isRequest, filename),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 0)
{
   TNLAssert(dynamic_cast<GameNetInterface *>(getInterface()), "Not a GameNetInterface");
   TNLAssert(((GameNetInterface *)getInterface())->getGame()->isServer(), "Not a ServerGame");

   ServerGame *game = static_cast<ServerGame *>(((GameNetInterface *)getInterface())->getGame());
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

   string adminHash = adminPW != "" ? Game::md5.getSaltedHashFromString(adminPW) : "";
   string ownerHash = ownerPW != "" ? Game::md5.getSaltedHashFromString(ownerPW) : "";

   string challengedAdminHash = computeDataConnectionChallenge(this, adminHash);
   string challengedOwnerHash = computeDataConnectionChallenge(this, ownerHash);
   string legacyAdminHash = computeLegacyDataChallenge(this, adminHash);
   string legacyOwnerHash = computeLegacyDataChallenge(this, ownerHash);

   bool goodOwnerPW = ownerPW != "" && (strcmp(challengedOwnerHash.c_str(), password) == 0 || strcmp(legacyOwnerHash.c_str(), password) == 0);
   bool goodAdminPW = adminPW != "" && (strcmp(challengedAdminHash.c_str(), password) == 0 || strcmp(legacyAdminHash.c_str(), password) == 0);

   if(!goodOwnerPW && !goodAdminPW)
   {
      logprintf("Incorrect password!");
      disconnect(ReasonBadLogin, "Incorrect password");
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
      string filenameStr = filename.getString();
      if(!safeFilename(filenameStr.c_str()) || extractFilename(filenameStr) != filenameStr || filenameStr.find("..") != string::npos)
      {
         logprintf("Invalid or unsafe filename for upload: %s", filenameStr.c_str());
         disconnect(ReasonError, "Invalid or unsafe filename");
         return;
      }

      FolderManager *folderManager = settings->getFolderManager();

      string folder = getOutputFolder(folderManager, (FileType)(U32)filetype);

      if(folder == "")     // filetype was bogus, will probably never happen
      {
         logprintf("Error resolving folder!");
         disconnect(ReasonError, "Error resolving folder");
         return;
      }

      if(mOutputFile)
      {
         fclose((FILE*)mOutputFile);
         mOutputFile = NULL;
      }

      mTargetPath = strictjoindir(folder, filename.getString());
      mTempPath = mTargetPath + ".tmp." + itos(Platform::getRealMilliseconds());
      mTransferSuccess = false;
      mBytesReceived = 0;

      mOutputFile = fopen(mTempPath.c_str(), "wb");

      if(!mOutputFile)
      {
         logprintf("Problem opening file %s for writing", mTempPath.c_str());
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
   static const U32 MaxDataUploadSize = 10 * 1024 * 1024; // 10MB limit

   if(!mOutputFile && isInitiator() && mAction == REQUEST_FILE)
   {
      TNLAssert(dynamic_cast<GameNetInterface *>(getInterface()), "Not a GameNetInterface");
      Game *game = static_cast<GameNetInterface *>(getInterface())->getGame();
      FolderManager *folderManager = game->getSettings()->getFolderManager();
      string folder = getOutputFolder(folderManager, mFileType);
      mTargetPath = strictjoindir(folder, mFilename);
      mTempPath = mTargetPath + ".tmp." + itos(Platform::getRealMilliseconds());
      mTransferSuccess = false;
      mBytesReceived = 0;
      mOutputFile = fopen(mTempPath.c_str(), "wb");
      if(!mOutputFile)
      {
         logprintf("Problem opening file %s for writing", mTempPath.c_str());
         disconnect(ReasonError, "done");
         return;
      }
   }

   if(mOutputFile)
   {
      U32 lineLen = (U32)strlen(line.getString());
      if(mBytesReceived + lineLen > MaxDataUploadSize)
      {
         logprintf("Data upload exceeded maximum allowed size");
         fclose((FILE*)mOutputFile);
         mOutputFile = NULL;
         if(mTempPath != "")
            remove(mTempPath.c_str());
         disconnect(ReasonError, "File size limit exceeded");
         return;
      }
      size_t written = fwrite(line.getString(), 1, lineLen, (FILE*)mOutputFile);
      if(written != lineLen)
      {
         logprintf("Problem writing chunk to file %s", mTempPath.c_str());
         fclose((FILE*)mOutputFile);
         mOutputFile = NULL;
         if(mTempPath != "")
            remove(mTempPath.c_str());
         disconnect(ReasonError, "File write error");
         return;
      }
      mBytesReceived += lineLen;
   }
}


// << DataSendable >>
// When client has finished sending its data, it sends a commandComplete message, which triggers the server to disconnect the client
TNL_IMPLEMENT_RPC(DataConnection, s2rCommandComplete, (RangedU32<0,SENDER_STATUS_COUNT> status), (status),
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   if(mOutputFile)
   {
      fclose((FILE*)mOutputFile);
      mOutputFile = NULL;
   }

   if(status == STATUS_OK && mTempPath != "" && mTargetPath != "")
   {
      remove(mTargetPath.c_str());
      if(rename(mTempPath.c_str(), mTargetPath.c_str()) == 0)
      {
         mTransferSuccess = true;
      }
      else
      {
         remove(mTempPath.c_str());
      }
   }
   else if(mTempPath != "")
   {
      remove(mTempPath.c_str());
   }

   disconnect(ReasonNone, "done");     // Terminate connection... should probably send different message depending on status
}


void DataConnection::onConnectionEstablished()
{
   if(isInitiator())    // i.e. client
   {
      string pwdHash = Game::md5.getSaltedHashFromString(mPassword);
      string challengedHash = computeDataConnectionChallenge(this, pwdHash);

      if(mAction == SEND_FILE)
      {
         c2sSendOrRequestFile(challengedHash.c_str(), mFileType, false, mFilename.c_str());
      }

      else if(mAction == REQUEST_FILE)
      {
         c2sSendOrRequestFile(challengedHash.c_str(), mFileType, true, mFilename.c_str());
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

   if(!mTransferSuccess && mTempPath != "")
   {
      remove(mTempPath.c_str());
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
