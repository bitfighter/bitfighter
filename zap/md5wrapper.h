/*
 *	This is my wrapper-class to create
 *	a MD5 Hash from a string and a file.
 *
 *	This code is completly free, you 
 *	can copy it, modify it, or do 
 *	what ever you want with it.
 *
 *	Feb. 2005
 *	Benjamin Gr�delbach
 */

//include protection
#ifndef MD5WRAPPER_H
#define MD5WRAPPER_H

//basic includes
#include <string>

class md5wrapper
{
	private:
	
		/*
		 * internal hash function, calling
		 * the basic methods from md5.h
		 */	
		std::string hashit(std::string text);

		/*
		 * converts the numeric giets to
		 * a valid std::string
		 */
		std::string convToString(unsigned char *bytes);


	public:
		//constructor
		md5wrapper();

		//destructor
		virtual ~md5wrapper();
		
		/*
		 * creates a MD5 hash from
		 * "text" and returns it as
		 * string
		 */	
		std::string getHashFromString(std::string text);
		std::string getHashFromString(const char *text);

      // Gets hash with appended salt, and makes text lowercase for case insensitivity
		std::string getSaltedHashFromString(std::string text);
		std::string getSaltedHashFromString(const char *text);

		/*
		 * creates a MD5 hash from
		 * a file specified in "filename" and 
		 * returns it as string
		 */	
		std::string getHashFromFile(std::string filename);
};

//include protection
#endif // MD5WRAPPER_H
