//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlLog.h"

#include "gtest/gtest.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#  define rmdir(x) _rmdir(x)
#else
#  include <unistd.h>
#endif

namespace TNL
{

// Helper function to check if a file exists
static bool fileExists(const std::string &filepath)
{
   struct stat buffer;
   return (stat(filepath.c_str(), &buffer) == 0);
}


// Helper function to read entire file contents
static std::string readFileContents(const std::string &filepath)
{
   std::ifstream file(filepath);
   if (!file.is_open())
      return "";

   std::stringstream buffer;
   buffer << file.rdbuf();
   return buffer.str();
}


// Helper function to check if string contains substring
static bool stringContains(const std::string &str, const std::string &substr)
{
   return str.find(substr) != std::string::npos;
}


// Helper function to check if a line contains a timestamp pattern (YYYY-MM-DD)
static bool lineContainsTimestamp(const std::string &line)
{
   // Look for timestamp pattern like "2025-11-22" at the beginning
   return (line.length() >= 10 && 
           isdigit(line[0]) && isdigit(line[1]) && isdigit(line[2]) && isdigit(line[3]) &&
           line[4] == '-' &&
           isdigit(line[5]) && isdigit(line[6]) &&
           line[7] == '-' &&
           isdigit(line[8]) && isdigit(line[9]));
}


 // Helper to extract first line from file contents
static std::string getFirstLine(const std::string &contents)
{
   size_t endPos = contents.find('\n');
   if (endPos == std::string::npos)
      return contents;
   return contents.substr(0, endPos);
}


class FileLoggingTest : public testing::Test
{
protected:
   virtual void SetUp()
   {
      // Clean up any leftover test files
      cleanupTestFiles();
   }

   virtual void TearDown()
   {
      // Clean up test files after test
      cleanupTestFiles();
   }

   void cleanupTestFiles()
   {
      // Delete test log files if they exist
      if (fileExists("test_logs/test_output.log"))
         std::remove("test_logs/test_output.log");

      if (fileExists("test_logs"))
         rmdir("test_logs");

      if (fileExists("deep/nested/logs/output.log"))
         std::remove("deep/nested/logs/output.log");

      if (fileExists("deep/nested/logs"))
         rmdir("deep/nested/logs");

      if (fileExists("deep/nested"))
         rmdir("deep/nested");

      if (fileExists("deep"))
         rmdir("deep");

      if (fileExists("test_logs/bitfighter_master.log"))
         std::remove("test_logs/bitfighter_master.log");

      if (fileExists("test_logs/timestamp_test.log"))
         std::remove("test_logs/timestamp_test.log");
   }
};


// Test that verifies logging to a single-level subfolder
TEST_F(FileLoggingTest, LogToSingleLevelSubfolder)
{
   // Create a FileLogConsumer and initialize it with a file in a subfolder
   FileLogConsumer consumer;
   std::string filename = "test_logs/test_output.log";

   consumer.init(filename, "w");

   // Log a test message
   const char* testMessage = "This is a test log message to a subfolder";
   consumer.logprintf("%s", testMessage);

   // Verify the file was created
   EXPECT_TRUE(fileExists(filename)) << "Log file was not created in subfolder";

   // Read the file and verify the message is there
   std::string contents = readFileContents(filename);
   EXPECT_TRUE(stringContains(contents, testMessage)) << "Log message not found in log file";

   if (fileExists(filename))
       std::remove(filename.c_str());
}


// Test that verifies logging to a multi-level subfolder
TEST_F(FileLoggingTest, LogToDeepNestedSubfolder)
{
   // Create a FileLogConsumer and initialize it with a file in deeply nested subfolders
   FileLogConsumer consumer;
   std::string filename = "test_logs/deep/nested/logs/output.log";
   consumer.init(filename, "w");

   // Log test messages
   const char *message1 = "First message to deep folder";
   const char *message2 = "Second message to deep folder";

   consumer.logprintf("%s", message1);
   consumer.logprintf("%s", message2);

   // Verify the file was created
   EXPECT_TRUE(fileExists(filename)) << "Log file was not created in deeply nested subfolder";

   // Read the file and verify both messages are there
   std::string contents = readFileContents(filename);
   EXPECT_TRUE(stringContains(contents, message1)) << "First log message not found in log file";
   EXPECT_TRUE(stringContains(contents, message2)) << "Second log message not found in log file";

   if (fileExists(filename))
       std::remove(filename.c_str());

}


// Test that verifies multiple log messages are written correctly
TEST_F(FileLoggingTest, MultipleMessagesInSubfolder)
{
   FileLogConsumer consumer;
   std::string filename = "test_logs/test_output.log";
   consumer.init(filename, "w");

   // Log multiple messages of different types
   consumer.logprintf("Error: Something went wrong");
   consumer.logprintf("Warning: Check this condition");
   consumer.logprintf("Info: Normal operation");

   // Verify all messages are in the log
   std::string contents = readFileContents("test_logs/test_output.log");
   EXPECT_TRUE(stringContains(contents, "Error: Something went wrong"));
   EXPECT_TRUE(stringContains(contents, "Warning: Check this condition"));
   EXPECT_TRUE(stringContains(contents, "Info: Normal operation"));

   if (fileExists(filename))
       std::remove(filename.c_str());
}


// Test that verifies append mode works with subfolders
TEST_F(FileLoggingTest, AppendModeToSubfolder)
{
   std::string filename = "test_logs/test_output.log";
   // First write
   {
      FileLogConsumer consumer;
      consumer.init(filename, "w");
      consumer.logprintf("First session message");
    }

   // Second write in append mode
   {
       FileLogConsumer consumer;
       consumer.init(filename, "a");
       consumer.logprintf("Second session message");
   }

   // Verify both messages are present
   std::string contents = readFileContents(filename);
   EXPECT_TRUE(stringContains(contents, "First session message")) << "First message lost in append mode";
   EXPECT_TRUE(stringContains(contents, "Second session message")) << "Second message not appended";

   if (fileExists(filename))
       std::remove(filename.c_str());

}


// Test matching master/main.cpp usage pattern
TEST_F(FileLoggingTest, MasterMainLogPattern)
{
   std::string filename = "test_logs/bitfighter_master.log";
   // This test mirrors the logging setup from master/main.cpp
   FileLogConsumer fileLogConsumer;
   fileLogConsumer.init(filename, "a");
   fileLogConsumer.setMsgTypes(LogConsumer::AllErrorTypes | LogConsumer::LogConnection);
   fileLogConsumer.logprintf("------ Bitfighter Master Server Log File ------");
   fileLogConsumer.logprintf("Server starting up...");

   // Verify the log file was created and contains expected messages
   EXPECT_TRUE(fileExists(filename)) << "Master log file not created";

   std::string contents = readFileContents(filename);
   EXPECT_TRUE(stringContains(contents, "Bitfighter Master Server Log File")) << "Header message not found in master log";
   EXPECT_TRUE(stringContains(contents, "Server starting up")) << "Startup message not found in master log";

   if (fileExists(filename))
       std::remove(filename.c_str());
}


// NEW TEST: Verify timestamps are present with MsgType parameter (logprintf with LogError)
TEST_F(FileLoggingTest, TimestampWithMsgType)
{
   std::string filename = "test_logs/timestamp_test.log";
   FileLogConsumer consumer;
   consumer.init(filename, "w");
   consumer.setMsgTypes(LogConsumer::LogError);

   // Log using the overload that takes MsgType
   logprintf(LogConsumer::LogError, "Unable to open MOTD file -- using default MOTD.");

   // Read and verify
   std::string contents = readFileContents(filename);
   std::string firstLine = getFirstLine(contents);

   EXPECT_TRUE(stringContains(contents, "Unable to open MOTD file")) << "Error message not found in log";
   
   EXPECT_TRUE(lineContainsTimestamp(firstLine)) << "Timestamp missing from LogError message. First line: " << firstLine;

   if (fileExists(filename))
       std::remove(filename.c_str());
}


// NEW TEST: Verify timestamps are present without MsgType parameter (logprintf without MsgType)
TEST_F(FileLoggingTest, TimestampWithoutMsgType)
{
   std::string filename = "test_logs/timestamp_test.log";
   FileLogConsumer consumer;
   consumer.init(filename, "w");
   consumer.setMsgTypes(LogConsumer::All);

   // Log using the overload that takes no MsgType (goes to All)
   logprintf("Master Server started - listening on port 25955");

   // Read and verify
   std::string contents = readFileContents(filename);
   std::string firstLine = getFirstLine(contents);

   EXPECT_TRUE(stringContains(contents, "Master Server started")) << "Server message not found in log";
   
   EXPECT_TRUE(lineContainsTimestamp(firstLine)) << "Timestamp missing from general logprintf message. First line: " << firstLine;

   if (fileExists(filename))
       std::remove(filename.c_str());
}


 // NEW TEST: Compare both logging paths to ensure consistent timestamp behavior
TEST_F(FileLoggingTest, BothLoggingPathsHaveTimestamps)
{
   std::string filename = "test_logs/timestamp_test.log";
   FileLogConsumer consumer;
   consumer.init(filename, "w");
   consumer.setMsgTypes(LogConsumer::All);

   // Log first message with MsgType
   logprintf(LogConsumer::LogError, "Error message with LogError type");

   // Log second message without MsgType
   logprintf("General message without MsgType");

   // Read and verify both have timestamps
   std::string contents = readFileContents(filename);
   size_t pos = 0;
   int lineCount = 0;
   std::string line;

   // Extract and check each line for timestamp
   std::istringstream stream(contents);
   while (std::getline(stream, line) && lineCount < 2)
   {
      if (!line.empty())
      {
         EXPECT_TRUE(lineContainsTimestamp(line)) << "Line " << (lineCount + 1) << " missing timestamp: " << line;
         lineCount++;
      }
   }

   EXPECT_EQ(lineCount, 2) << "Expected at least 2 logged lines with content";

   if (fileExists(filename))
       std::remove(filename.c_str());
}

}  // namespace TNL