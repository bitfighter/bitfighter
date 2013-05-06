/*
 *	This is part of my wrapper-class to create
 *	a MD5 Hash from a string and a file.
 *
 *	This code is completly free, you 
 *	can copy it, modify it, or do 
 *	what ever you want with it.
 *
 *	Feb. 2005
 *	Benjamin Gr�delbach
 */

#ifdef _MSC_VER
#  pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

//---------------------------------------------------------------------- 
//basic includes
#include <fstream>
#include <iostream>

//my includes
#include "md5wrapper.h"
#include "../libtomcrypt/mycrypt.h"



//---------privates--------------------------

/*
 * internal hash function, calling
 * the basic methods from md5.h
 */	
std::string md5wrapper::hashit(std::string text)
{
   unsigned char outBuffer[16] = "";
   hash_state md;
   md5_init(&md);
   md5_process(&md, (unsigned char*)text.c_str(), (unsigned int)text.length());
   md5_done(&md, outBuffer);

	//convert the hash to a string and return it
	return convToString(outBuffer);
}

/*
 * converts the numeric hash to
 * a valid std::string.
 * (based on Jim Howard's code;
 * http://www.codeproject.com/cpp/cmd5.asp)
 */
std::string md5wrapper::convToString(unsigned char *bytes)
{
	char asciihash[33];

	int p = 0;
	for(int i=0; i<16; i++)
	{
		::sprintf(&asciihash[p],"%02x",bytes[i]);
		p += 2;
	}	
	asciihash[32] = '\0';
	return std::string(asciihash);
}


// Convert string to lower case
std::string lcase(std::string strToConvert)
{
   for(std::string::size_type i = 0; i < strToConvert.length(); i++)
      strToConvert[i] = tolower(strToConvert[i]);
   return strToConvert;
}

//---------publics--------------------------

// Constructor
md5wrapper::md5wrapper()
{
   //	Do nothing
}


// Destructor
md5wrapper::~md5wrapper()
{
	// Do nothing
}

/*
 * creates a MD5 hash from
 * "text" and returns it as
 * string
 */	
std::string md5wrapper::getHashFromString(std::string text)
{
	return this->hashit(text); 
}

std::string md5wrapper::getHashFromString(const char *text)
{
   std::string str;

   // Check for null
   if(text == 0)
      str = "";
   else
      str = std::string(text);

   return getHashFromString(str);
}


std::string md5wrapper::getSaltedHashFromString(std::string text)
{
   // From http://clsc.net/tools/random-string-generator.php, in case you care!
   // Changing this will break compatibility with any clients/servers using a different salt.
   std::string salt = "8-0qf_C5z5xoH_M_--39_0xS5mPC99bbq9q-g80-003_4b__m7";
	return this->hashit(lcase(text) + salt); 
}

std::string md5wrapper::getSaltedHashFromString(const char *text)
{
   std::string str;

   // Check for null
   if(text == 0)
      str = "";
   else
      str = std::string(text);

   return getSaltedHashFromString(str);
}




/*
 * creates a MD5 hash from
 * a file specified in "filename" and 
 * returns it as string
 * (based on Ronald L. Rivest's code
 * from RFC1321 "The MD5 Message-Digest Algorithm")
 */	
std::string md5wrapper::getHashFromFile(std::string filename)	
{
	FILE *file;
   hash_state md;

	unsigned int len;
  	unsigned char buffer[1024], digest[16];

	//open file
  	if ((file = fopen (filename.c_str(), "rb")) == NULL)
	{
		return "-1";
	}

	//init md5
   md5_init(&md);

	//read the filecontent
	while ( (len = (unsigned int)fread (buffer, 1, 1024, file)) )
   {
	   md5_process(&md, buffer, len);
	}

	/*
	generate hash, close the file and return the
	hash as std::string
	*/
   md5_done(&md, digest);
 	fclose (file);

	return convToString(digest);
}

/*
 * EOF
 */

