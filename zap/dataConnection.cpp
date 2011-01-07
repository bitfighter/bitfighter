#include "gameLoader.h"                   // For MAX_LEVEL_FILE_LENGTH def  <-- there has to be a better way!

#include "dataConnection.h"
#include "tnlEventConnection.h"
#include "game.h"                         // For ServerGame def

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

class ServerGame;
extern ServerGame *gServerGame;
extern ConfigDirectories gConfigDirs;



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
   

static string getFullFilename(string filename, FileType fileType)
{
   // Don't return "" if empty directory, allow client load the file if on the same directory as EXE.
   string name;
   if(fileType == BOT_TYPE)
      name = ConfigDirectories::findBotFile(filename);

   else if(fileType == LEVEL_TYPE)
      name = ConfigDirectories::findLevelFile(filename);

   else if(fileType == LEVELGEN_TYPE)
      name = ConfigDirectories::findLevelGenScript(filename);

   else
      name = "";

   return (name == "") ? filename : name;
}


static string getOutputFolder(FileType filetype)
{
   if(filetype == BOT_TYPE) 
      return gConfigDirs.robotDir;
   else if(filetype == LEVEL_TYPE) 
      return gConfigDirs.levelDir;
   else if(filetype == LEVELGEN_TYPE) 
      return gConfigDirs.levelDir;
   else return "";
}


////////////////////////////////////////
////////////////////////////////////////

// For readability
#define MAX_LINE_LEN  HuffmanStringProcessor::MAX_SENDABLE_LINE_LENGTH

DataSender::SenderStatus DataSender::initialize(DataSendable *connection, string filename, FileType fileType)
{
   string fullname = getFullFilename(filename, fileType);
   if(fullname == "")
   {
      return COULD_NOT_FIND_FILE;
   }

   //ifstream file;
   //file.open(fullname.c_str());
   FILE *file = fopen(fullname.c_str(), "r");

   //if(!file.is_open())
   if(!file)
      return COULD_NOT_OPEN_FILE;




   // Allocate a buffer
   //char *buffer = new char[MAX_LINE_LEN + 1];      // 255 for data, + 1 for terminator
   char buffer[MAX_LINE_LEN + 1];      // 255 for data, + 1 for terminator
   S32 size;

   // We'll read the file in 255 char chunks; this is the largest string we can send, and we want to be as large as possible to get
   // maximum benefit of the string compression that occurs during the transmission process.
   /*
   while(!file.eof() && mLines.size() * MAX_LINE_LEN < MAX_LEVEL_FILE_LENGTH)
   {
      file.read(buffer, MAX_LINE_LEN);
      if(file.gcount() > 0)
      {
         buffer[file.gcount()] = '\0';     // Null terminate
         mLines.push_back(buffer); 
      }
   }
   file.close();
   */

   size = (S32) fread(buffer, 1, MAX_LINE_LEN, file);

   while(size > 0 && mLines.size() * MAX_LINE_LEN < MAX_LEVEL_FILE_LENGTH){
       buffer[size]=0;           // Null terminate
       mLines.push_back(buffer);
       size = (S32) fread(buffer, 1, MAX_LINE_LEN, file);
   }

   fclose(file);

   //delete[] buffer;

   // Not exactly accurate -- if final line is only a few bytes, this will count it as being the full 255.
   if(mLines.size() * MAX_LINE_LEN >= MAX_LEVEL_FILE_LENGTH)
   {
      mLines.clear();
      return FILE_TOO_LONG;
   }

   if(mLines.size() == 0)          // Read nothing
      return COULD_NOT_OPEN_FILE;

   mConnection = connection;
   mFileType = fileType;
   mDone = false;
   mLineCtr = 0;
   return OK;
}

#undef MAX_LINE_LEN


// Send next line of our file
void DataSender::sendNextLine()
{
   if(mDone)
      return;

   if(mLineCtr < mLines.size())
   {
      mConnection->s2rSendLine(mLines[mLineCtr].c_str());
      mLineCtr++;
   }
   else
   {
      mConnection->s2rCommandComplete();
      mDone = true;
      mLines.clear();      // Liberate some memory
   }
}


////////////////////////////////////////
////////////////////////////////////////

// static method
string DataConnection::getErrorMessage(DataSender::SenderStatus stat, const string &filename)
{
   if(stat == DataSender::COULD_NOT_OPEN_FILE)
      return "Could not open file " + filename;

   else if(stat == DataSender::COULD_NOT_FIND_FILE)
      return "Could not find file " + filename;

   else if(stat == DataSender::FILE_TOO_LONG)
      return "File " + filename + " is too big to send";

   else
      return "Unknown problem";
}


TNL_IMPLEMENT_NETCONNECTION(DataConnection, NetClassGroupGame, true);


extern string gAdminPassword;
extern IniSettings gIniSettings;
extern md5wrapper md5;

// Client sends this message to set up the coming transfer.  Server checks for the password, and then, if the client is requesting
// a file, initiates the transfer.  If client is sending a file, it gets things ready then sends s2cOkToSend to indicate it's ready.
TNL_IMPLEMENT_RPC(DataConnection, c2sSendOrRequestFile, 
                  (StringPtr password, RangedU32<0,U32(FILE_TYPES)> filetype, bool isRequest, StringPtr filename), 
                  (password, filetype, isRequest, filename), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirClientToServer, 1)
{
   // Check if data connections are allowed
   if(!gIniSettings.allowDataConnections)
   {
      logprintf("This server does not allow remote access to resources.  It can be enabled in the server's INI file.");
      disconnect(ReasonConnectionsForbidden, "");
      return;
   }
   // Check password
   if(gAdminPassword == "" || strcmp(md5.getSaltedHashFromString(gAdminPassword).c_str(), password))
   {
      logprintf("Incorrect password!");
      disconnect(ReasonBadLogin, "Incorrect pasword");
      return;
   }

   // Process request
   if(isRequest)     // Client wants to get a file from us... they should have a file open and waiting for this data
   {
      // Initialize on the server to start sending requested file -- will return OK if everything is set up right
      DataSender::SenderStatus stat = gServerGame->dataSender.initialize(this, filename.getString(), (FileType)(U32)filetype);

      if(stat != DataSender::OK)
      {
         string msg = getErrorMessage(stat, filename.getString());

         logprintf("%s", msg.c_str());
         disconnect(ReasonError, msg.c_str());
         return;
      }
   }
   else              // Client wants to send us a file -- get ready for incoming data!
   {
      string folder = getOutputFolder((FileType)(U32)filetype);

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
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 1)
{
   // Initialize on the client to start sending file we want to send
   DataSender::SenderStatus stat = mDataSender.initialize(this, mFilename.c_str(), mFileType);
   if(stat != DataSender::OK)
   {
      string msg = getErrorMessage(stat, mFilename);

      logprintf("%s", msg.c_str());
      disconnect(ReasonError, msg.c_str());
   }
}


// << DataSendable >>
// Send a chunk of the file -- this gets run on the receiving end       
TNL_IMPLEMENT_RPC(DataConnection, s2rSendLine, (StringPtr line), (line), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 1)
{
   if(mOutputFile)
      fwrite(line.getString(), 1, strlen(line.getString()), mOutputFile);
      //mOutputFile.write(line.getString(), strlen(line.getString()));
   // else... what?
}


// << DataSendable >>
// When client has finished sending its data, it sends a commandComplete message, which triggers the server to disconnect the client
TNL_IMPLEMENT_RPC(DataConnection, s2rCommandComplete, (), (), 
                  NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 1)
{
   disconnect(ReasonNone, "done");     // Terminate connection

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
         string folder = getOutputFolder(mFileType);

         if(folder == "")     // filetype was bogus; should never happen
         {
            logprintf("Error resolving folder!");
                              // we can save files without needing folder
         }

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


extern void exitGame(S32);

// Make sure things are cleaned up -- will run on both client and server
void DataConnection::onConnectionTerminated(TerminationReason reason, const char *reasonMsg)
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