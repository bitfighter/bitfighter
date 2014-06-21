//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif


#include "Md5Utils.h"

#include "stringUtils.h"               // For lcase

#include "tnlTypes.h"

#include <fstream>
#include <iostream>

using std::string;
using namespace TNL;


namespace Md5
{

// Converts the numeric hash to a valid string.
// Based on Jim Howard's code: http://www.codeproject.com/cpp/cmd5.asp.
static string convToString(unsigned char *bytes)
{
	char asciihash[33];

	int p = 0;
	for(int i = 0; i < 16; i++)
	{
		sprintf(&asciihash[p], "%02x", bytes[i]);
		p += 2;
	}	
	asciihash[32] = '\0';
	return string(asciihash);
}


// Internal hash function, calling the basic methods from md5.h
static string hash(const string &text)
{
   unsigned char outBuffer[16] = "";
   hash_state md;

   md5_init(&md);
   md5_process(&md, (unsigned char*)text.c_str(), (unsigned int)text.length());
   md5_done(&md, outBuffer);

	// Convert the hash to a string and return it
	return convToString(outBuffer);
}


// Creates a MD5 hash from "text" and returns it as a string
string getHashFromString(const string &text)
{
	return hash(text); 
}


string getSaltedHashFromString(const string &text)
{
   // From http://clsc.net/tools/random-string-generator.php, in case you care!
   // Changing this will break compatibility with any clients/servers using a different salt.
   string salt = "8-0qf_C5z5xoH_M_--39_0xS5mPC99bbq9q-g80-003_4b__m7";
	return hash(Zap::lcase(text) + salt); 
}


// Creates a MD5 hash from a file specified in "filename" and returns it as string 
// (based on Ronald L. Rivest's code from RFC1321 "The MD5 Message-Digest Algorithm") 
string getHashFromFile(const string &filename)	
{
   const S32 ChunkSize = 1024;

	unsigned int len;
  	unsigned char buffer[ChunkSize], digest[16];

	// Open file
   FILE *file = fopen (filename.c_str(), "rb");
   if(!file)
		return "-1";

	// Init md5
   hash_state md;
   md5_init(&md);

	// Read the file
	while( (len = (unsigned int)fread (buffer, 1, ChunkSize, file)) )
	   md5_process(&md, buffer, len);

	// Generate hash, close the file, and return the hash as string
   md5_done(&md, digest);
 	fclose (file);

	return convToString(digest);
}


// Constructor
IncrementalHasher::IncrementalHasher()
{
   // Init md5
   md5_init(&mHashState);
}


// Add another line of content to the hash
void IncrementalHasher::add(const string &line)
{
   // The (bitwise) contents of the string don't change if the characters
   // are interpreted as unsigned chars instead of signed chars, so we can
   // use a reinterpret_cast to get our string of unsigned chars.

   md5_process(&mHashState, reinterpret_cast<const unsigned char *>(line.c_str()), line.length());
}


// Return the final computed hash
string IncrementalHasher::getHash()
{
   U8 digest[16];
   md5_done(&mHashState, digest);
	return convToString(digest);
}


}

