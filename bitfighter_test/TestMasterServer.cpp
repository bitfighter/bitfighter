//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE for full copyright information
//------------------------------------------------------------------------------

#include "../master/master.h"

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

using namespace Master;

namespace MasterTest
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


// Helper function to write a string to a file
static void writeFileContents(const std::string &filepath, const std::string &contents)
{
   std::ofstream file(filepath);
   if (file.is_open())
   {
      file << contents;
      file.close();
   }
}


// Helper to create directory structure
static bool createDirectory(const std::string &dirpath)
{
#ifdef _WIN32
   return _mkdir(dirpath.c_str()) == 0 || errno == EEXIST;
#else
   return mkdir(dirpath.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}


class MasterSettingsTest : public testing::Test
{
protected:
   virtual void SetUp()
   {
      cleanupTestFiles();
   }

   virtual void TearDown()
   {
      cleanupTestFiles();
   }

   void cleanupTestFiles()
   {
      // Clean up MOTD test files
      if (fileExists("test_motd_relative"))
         std::remove("test_motd_relative");

      if (fileExists("test_motd_subfolder/test_motd"))
         std::remove("test_motd_subfolder/test_motd");

      if (fileExists("test_motd_subfolder"))
         rmdir("test_motd_subfolder");

      if (fileExists("deep_motd_test/nested/path/motd"))
         std::remove("deep_motd_test/nested/path/motd");

      if (fileExists("deep_motd_test/nested/path"))
         rmdir("deep_motd_test/nested/path");

      if (fileExists("deep_motd_test/nested"))
         rmdir("deep_motd_test/nested");

      if (fileExists("deep_motd_test"))
         rmdir("deep_motd_test");

      if (fileExists("test_motd_empty"))
         std::remove("test_motd_empty");

      if (fileExists("nonexistent_motd"))
         std::remove("nonexistent_motd");
   }
};


// Handle nonexistent MOTD file gracefully
TEST_F(MasterSettingsTest, NonexistentMOTDFile)
{
   // Create MasterSettings
   MasterSettings settings("dummy.ini");

   // Attempting to read nonexistent file should return default
   std::string result = settings.getMotd(0);

   // Should return default message, not crash
   EXPECT_FALSE(result.empty()) << "getMotd should return a default message for nonexistent file";
}



}  // namespace MasterTest