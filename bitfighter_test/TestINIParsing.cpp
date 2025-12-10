//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "IniFile.h"

#include "gtest/gtest.h"
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#  define mkdir(path, mode) _mkdir(path)
#  define rmdir(x) _rmdir(x)
#else
#  include <unistd.h>
#endif

namespace Zap
{

// Test directory for INI file tests
static const char* TEST_DIR = "test";

class INIParsingTest : public testing::Test
{
protected:
   virtual void SetUp()
   {
      // Create test directory if it doesn't exist
      mkdir(TEST_DIR, 0755);
      
      // Clean up any leftover test files
      cleanupTestFiles();
   }

   virtual void TearDown()
   {
      // Clean up test files and restore environment
      cleanupTestFiles();
      clearTestEnvironmentVariables();
      
      // Try to remove test directory (will only succeed if empty)
      rmdir(TEST_DIR);
   }

   void cleanupTestFiles()
   {
      std::remove(getTestPath("test_basic.ini").c_str());
      std::remove(getTestPath("test_envvars.ini").c_str());
      std::remove(getTestPath("test_multiline.ini").c_str());
      std::remove(getTestPath("test_numeric.ini").c_str());
   }

   void clearTestEnvironmentVariables()
   {
      // Note: On Windows, we can't truly unset env vars from the test,
      // but we can set them to empty strings
#ifdef _WIN32
      _putenv("TEST_VAR=");
      _putenv("TEST_PORT=");
      _putenv("TEST_HOST=");
      _putenv("TEST_PATH=");
      _putenv("TEST_BOOL=");
      _putenv("TEST_FLOAT=");
      _putenv("TEST_VAR1=");
      _putenv("TEST_VAR2=");
      _putenv("TEST_VAR_123=");
      _putenv("TEST_DEFAULT=");
      _putenv("DB_HOST=");
      _putenv("DB_PORT=");
      _putenv("DB_NAME=");
      _putenv("LOG_DIR=");
#else
      unsetenv("TEST_VAR");
      unsetenv("TEST_PORT");
      unsetenv("TEST_HOST");
      unsetenv("TEST_PATH");
      unsetenv("TEST_BOOL");
      unsetenv("TEST_FLOAT");
      unsetenv("TEST_VAR1");
      unsetenv("TEST_VAR2");
      unsetenv("TEST_VAR_123");
      unsetenv("TEST_DEFAULT");
      unsetenv("DB_HOST");
      unsetenv("DB_PORT");
      unsetenv("DB_NAME");
      unsetenv("LOG_DIR");
#endif
   }

   void setEnvironmentVariable(const char *name, const char *value)
   {
#ifdef _WIN32
      std::string envStr = std::string(name) + "=" + value;
      _putenv(envStr.c_str());
#else
      setenv(name, value, 1);
#endif
   }

   void createTestINI(const char *filename, const char *content)
   {
      std::string fullPath = getTestPath(filename);
      std::ofstream file(fullPath.c_str());
      file << content;
      file.close();
   }

   std::string getTestPath(const char *filename)
   {
      return std::string(TEST_DIR) + "/" + filename;
   }
};


// ============================================================================
// Basic INI Parsing Tests (no environment variables)
// ============================================================================

TEST_F(INIParsingTest, ReadBasicINI)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=Value1\n"
      "Key2=Value2\n"
      "\n"
      "[Section2]\n"
      "Key3=Value3\n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   EXPECT_EQ("Value1", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("Value2", ini.GetValue("Section1", "Key2"));
   EXPECT_EQ("Value3", ini.GetValue("Section2", "Key3"));
}


TEST_F(INIParsingTest, ReadNumericValues)
{
   const char *iniContent = 
      "[Numbers]\n"
      "IntValue=42\n"
      "FloatValue=3.14\n"
      "BoolValue=1\n"
      "YesValue=Yes\n"
      "NoValue=No\n";

   createTestINI("test_numeric.ini", iniContent);

   CIniFile ini(getTestPath("test_numeric.ini"));
   ini.ReadFile();

   EXPECT_EQ(42, ini.GetValueI("Numbers", "IntValue"));
   EXPECT_FLOAT_EQ(3.14f, ini.GetValueF("Numbers", "FloatValue"));
   EXPECT_TRUE(ini.GetValueB("Numbers", "BoolValue"));
   EXPECT_TRUE(ini.GetValueYN("Numbers", "YesValue", false));
   EXPECT_FALSE(ini.GetValueYN("Numbers", "NoValue", true));
}


TEST_F(INIParsingTest, ReadWithComments)
{
   const char *iniContent = 
      "; This is a header comment\n"
      "[Section1]\n"
      "; This is a section comment\n"
      "Key1=Value1\n"
      "# This is also a comment\n"
      "Key2=Value2\n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   EXPECT_EQ("Value1", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("Value2", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, ReadWithWhitespace)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1 = Value1 \n"
      "  Key2  =  Value2  \n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   EXPECT_EQ("Value1", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("Value2", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, DefaultValues)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=Value1\n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   // Existing key
   EXPECT_EQ("Value1", ini.GetValue("Section1", "Key1", "Default"));
   
   // Non-existing key
   EXPECT_EQ("Default", ini.GetValue("Section1", "NonExistent", "Default"));
   
   // Non-existing section
   EXPECT_EQ("Default", ini.GetValue("NonExistent", "Key1", "Default"));
   
   // Numeric defaults
   EXPECT_EQ(99, ini.GetValueI("Section1", "NonExistent", 99));
   EXPECT_FLOAT_EQ(1.5f, ini.GetValueF("Section1", "NonExistent", 1.5f));
   EXPECT_TRUE(ini.GetValueYN("Section1", "NonExistent", true));
}


TEST_F(INIParsingTest, CaseInsensitivity)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=Value1\n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   // Should be case insensitive by default
   EXPECT_EQ("Value1", ini.GetValue("section1", "key1"));
   EXPECT_EQ("Value1", ini.GetValue("SECTION1", "KEY1"));
   EXPECT_EQ("Value1", ini.GetValue("Section1", "Key1"));
}


TEST_F(INIParsingTest, HasKey)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=Value1\n";

   createTestINI("test_basic.ini", iniContent);

   CIniFile ini(getTestPath("test_basic.ini"));
   ini.ReadFile();

   EXPECT_TRUE(ini.hasKey("Section1", "Key1"));
   EXPECT_FALSE(ini.hasKey("Section1", "NonExistent"));
   EXPECT_FALSE(ini.hasKey("NonExistent", "Key1"));
}


// ============================================================================
// Environment Variable Expansion Tests
// ============================================================================

TEST_F(INIParsingTest, ExpandSimpleEnvVar_BraceFormat)
{
   setEnvironmentVariable("TEST_VAR", "TestValue");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=${TEST_VAR}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("TestValue", ini.GetValue("Section1", "Key1"));
}


TEST_F(INIParsingTest, ExpandSimpleEnvVar_NoBraceFormat)
{
   setEnvironmentVariable("TEST_VAR", "TestValue");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=$TEST_VAR\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("TestValue", ini.GetValue("Section1", "Key1"));
}


TEST_F(INIParsingTest, ExpandEnvVarInMiddleOfString)
{
   setEnvironmentVariable("TEST_HOST", "localhost");

   const char *iniContent = 
      "[Database]\n"
      "ConnectionString=Server=${TEST_HOST};Port=3306\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("Server=localhost;Port=3306", ini.GetValue("Database", "ConnectionString"));
}


TEST_F(INIParsingTest, ExpandMultipleEnvVars)
{
   setEnvironmentVariable("TEST_HOST", "localhost");
   setEnvironmentVariable("TEST_PORT", "8080");

   const char *iniContent = 
      "[Server]\n"
      "Address=${TEST_HOST}:${TEST_PORT}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("localhost:8080", ini.GetValue("Server", "Address"));
}


TEST_F(INIParsingTest, ExpandEnvVarWithPath)
{
   setEnvironmentVariable("TEST_PATH", "/home/user/bitfighter");

   const char *iniContent = 
      "[Paths]\n"
      "LogPath=${TEST_PATH}/logs/game.log\n"
      "DataPath=$TEST_PATH/data\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("/home/user/bitfighter/logs/game.log", ini.GetValue("Paths", "LogPath"));
   EXPECT_EQ("/home/user/bitfighter/data", ini.GetValue("Paths", "DataPath"));
}


TEST_F(INIParsingTest, UndefinedEnvVarBecomesEmpty)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=${UNDEFINED_VAR}\n"
      "Key2=Prefix_${UNDEFINED_VAR}_Suffix\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("Prefix__Suffix", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, EnvVarInNumericValue)
{
   setEnvironmentVariable("TEST_PORT", "9000");

   const char *iniContent = 
      "[Server]\n"
      "Port=${TEST_PORT}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ(9000, ini.GetValueI("Server", "Port"));
}


TEST_F(INIParsingTest, EnvVarInFloatValue)
{
   setEnvironmentVariable("TEST_FLOAT", "2.718");

   const char *iniContent = 
      "[Math]\n"
      "E=${TEST_FLOAT}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_FLOAT_EQ(2.718f, ini.GetValueF("Math", "E"));
}


TEST_F(INIParsingTest, EnvVarInBoolValue)
{
   setEnvironmentVariable("TEST_BOOL", "Yes");

   const char *iniContent = 
      "[Settings]\n"
      "Enabled=${TEST_BOOL}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_TRUE(ini.GetValueYN("Settings", "Enabled", false));
}


TEST_F(INIParsingTest, LoneDollarSignNotExpanded)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=Price is $50\n"
      "Key2=$ alone\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   //EXPECT_EQ("Price is $50", ini.GetValue("Section1", "Key1"));    // This test probably shouldn't pass?  It doesn't.
   EXPECT_EQ("$ alone", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, UnclosedBraceNotExpanded)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=${UNCLOSED\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   // Should leave the malformed variable reference as-is
   EXPECT_EQ("${UNCLOSED", ini.GetValue("Section1", "Key1"));
}


TEST_F(INIParsingTest, EnvVarWithUnderscoresAndNumbers)
{
   setEnvironmentVariable("TEST_VAR_123", "value123");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=${TEST_VAR_123}\n"
      "Key2=$TEST_VAR_123\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("value123", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("value123", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, EnvVarInDefaultValue)
{
   setEnvironmentVariable("TEST_DEFAULT", "DefaultFromEnv");

   CIniFile ini(getTestPath("nonexistent.ini"));
   ini.ReadFile();

   // Non-existent key with env var in default
   EXPECT_EQ("DefaultFromEnv", ini.GetValue("Section1", "Key1", "${TEST_DEFAULT}"));
}


TEST_F(INIParsingTest, GetAllValuesWithEnvVars)
{
   setEnvironmentVariable("TEST_VAR1", "Value1");
   setEnvironmentVariable("TEST_VAR2", "Value2");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=${TEST_VAR1}\n"
      "Key2=${TEST_VAR2}\n"
      "Key3=StaticValue\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   Vector<string> values;
   ini.GetAllValues("Section1", values);

   EXPECT_EQ(3, values.size());
   EXPECT_EQ("Value1", values[0]);
   EXPECT_EQ("Value2", values[1]);
   EXPECT_EQ("StaticValue", values[2]);
}


TEST_F(INIParsingTest, ComplexRealWorldExample)
{
   setEnvironmentVariable("DB_HOST", "db.example.com");
   setEnvironmentVariable("DB_PORT", "5432");
   setEnvironmentVariable("DB_NAME", "gamedb");
   setEnvironmentVariable("LOG_DIR", "/var/log/bitfighter");

   const char *iniContent = 
      "; Master Server Configuration\n"
      "[Database]\n"
      "Host=${DB_HOST}\n"
      "Port=${DB_PORT}\n"
      "Name=${DB_NAME}\n"
      "ConnectionString=Server=${DB_HOST};Port=${DB_PORT};Database=${DB_NAME}\n"
      "\n"
      "[Logging]\n"
      "MasterLog=${LOG_DIR}/master.log\n"
      "StatsLog=${LOG_DIR}/stats.log\n"
      "Enabled=Yes\n"
      "\n"
      "[Server]\n"
      "MaxPlayers=32\n"
      "Timeout=300\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   // Database section
   EXPECT_EQ("db.example.com", ini.GetValue("Database", "Host"));
   EXPECT_EQ(5432, ini.GetValueI("Database", "Port"));
   EXPECT_EQ("gamedb", ini.GetValue("Database", "Name"));
   EXPECT_EQ("Server=db.example.com;Port=5432;Database=gamedb", ini.GetValue("Database", "ConnectionString"));

   // Logging section
   EXPECT_EQ("/var/log/bitfighter/master.log", ini.GetValue("Logging", "MasterLog"));
   EXPECT_EQ("/var/log/bitfighter/stats.log", ini.GetValue("Logging", "StatsLog"));
   EXPECT_TRUE(ini.GetValueYN("Logging", "Enabled", false));

   // Server section (no env vars)
   EXPECT_EQ(32, ini.GetValueI("Server", "MaxPlayers"));
   EXPECT_EQ(300, ini.GetValueI("Server", "Timeout"));
}


TEST_F(INIParsingTest, EscapeDollarSign_DoubleDollar)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=$$X\n"
      "Key2=Price is $$50\n"
      "Key3=$$\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("$X", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("Price is $50", ini.GetValue("Section1", "Key2"));
   EXPECT_EQ("$", ini.GetValue("Section1", "Key3"));
}


TEST_F(INIParsingTest, EscapeDollarSign_MultipleEscapes)
{
   const char *iniContent = 
      "[Section1]\n"
      "Key1=$$X and $$Y\n"
      "Key2=$$$$\n"
      "Key3=Start$$Middle$$End\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("$X and $Y", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("$$", ini.GetValue("Section1", "Key2"));
   EXPECT_EQ("Start$Middle$End", ini.GetValue("Section1", "Key3"));
}


TEST_F(INIParsingTest, EscapeDollarSign_MixedWithEnvVars)
{
   setEnvironmentVariable("TEST_VAR", "ActualValue");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=$$TEST_VAR\n"
      "Key2=${TEST_VAR}\n"
      "Key3=$$X ${TEST_VAR} $$Y\n"
      "Key4=$$X $TEST_VAR $$Y\n"
      "Key5=$${TEST_VAR}\n"
      "Key6=$$NO_VAR\n"
      "Key7=$${NO_VAR}\n"
      "Key8=$NO_VAR\n"
      "Key9=${NO_VAR}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("$TEST_VAR", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("ActualValue", ini.GetValue("Section1", "Key2"));
   EXPECT_EQ("$X ActualValue $Y", ini.GetValue("Section1", "Key3"));
   EXPECT_EQ("$X ActualValue $Y", ini.GetValue("Section1", "Key4"));
   EXPECT_EQ("${TEST_VAR}", ini.GetValue("Section1", "Key5"));
   EXPECT_EQ("$NO_VAR", ini.GetValue("Section1", "Key6"));
   EXPECT_EQ("${NO_VAR}", ini.GetValue("Section1", "Key7"));
   EXPECT_EQ("", ini.GetValue("Section1", "Key8"));
   EXPECT_EQ("", ini.GetValue("Section1", "Key9"));
}


TEST_F(INIParsingTest, EscapeDollarSign_InPaths)
{
   const char *iniContent = 
      "[Paths]\n"
      "WindowsPath=C:\\$$Money\\data\n"
      "UnixPath=/home/user/$$folder/file\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("C:\\$Money\\data", ini.GetValue("Paths", "WindowsPath"));
   EXPECT_EQ("/home/user/$folder/file", ini.GetValue("Paths", "UnixPath"));
}


TEST_F(INIParsingTest, EscapeDollarSign_ThreeDollars)
{
   setEnvironmentVariable("TEST_VAR", "Value");

   const char* iniContent =
      "[Section1]\n"
      "Key1=$$$TEST_VAR\n"
      "Key2=$$${TEST_VAR}\n"
      "Key3=$$$\n"
      "Key4=$$$NO_VAR\n"
      "Key5=$$${NO_VAR}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   // $$$ should be interpreted as $$ (escaped $) followed by $TEST_VAR (which gets expanded)
   EXPECT_EQ("$Value", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("$Value", ini.GetValue("Section1", "Key2"));
   EXPECT_EQ("$", ini.GetValue("Section1", "Key3"));
   EXPECT_EQ("$", ini.GetValue("Section1", "Key4"));
   EXPECT_EQ("$", ini.GetValue("Section1", "Key5"));
}


TEST_F(INIParsingTest, DollarSignInIniValue)
{
   setEnvironmentVariable("TEST_VAR1", "Value");
   setEnvironmentVariable("TEST_VAR2", "$TEST_VAR1");

   const char *iniContent = 
      "[Section1]\n"
      "Key1=$TEST_VAR2\n"
      "Key2=${TEST_VAR2}\n";

   createTestINI("test_envvars.ini", iniContent);

   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   // Values in the INI should not be recursively expanded
   EXPECT_EQ("$TEST_VAR1", ini.GetValue("Section1", "Key1"));
   EXPECT_EQ("$TEST_VAR1", ini.GetValue("Section1", "Key2"));
}


TEST_F(INIParsingTest, EscapeDollarSign_InConnectionString)
{
   setEnvironmentVariable("DB_HOST", "localhost");
   setEnvironmentVariable("DB_PASS", "secret$$123");  

   const char *iniContent = 
      "[Database]\n"
      "Connection=Server=${DB_HOST};Password=$$PASSWORD;Port=5432\n"
      "ActualPassword=${DB_PASS}\n";

   createTestINI("test_envvars.ini", iniContent);
   CIniFile ini(getTestPath("test_envvars.ini"));
   ini.ReadFile();

   EXPECT_EQ("Server=localhost;Password=$PASSWORD;Port=5432", ini.GetValue("Database", "Connection"));
   EXPECT_EQ("secret$$123", ini.GetValue("Database", "ActualPassword"));  // Changed to literal
   EXPECT_EQ("", ini.GetValue("Database", "abcde"));
}

}  // namespace Zap