/*
Copyright (c) 2009, William M Brandt (aka 'Taekvideo')
All rights reserved.
Email: taekvideo@gmail.com (feel free to contact me with any questions, concerns, or suggestions)

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistribution of any files containing source code covered by this license must retain the above copyright notice, 
  this list of conditions, and the following disclaimer.
* Neither the name William M Brandt nor the pseudonym 'Taekvideo' may be used to endorse or 
  promote products derived from this software without specific prior written permission.
* It's not required, but I would appreciate being included in your credits.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/





#if defined(VERIFY_PHPBB3) && !defined(AUTHENTICATOR_H)
#define AUTHENTICATOR_H

#include <string>

namespace mysqlpp{
	class TCPConnection;
}

class Authenticator{
public:
	Authenticator();
	Authenticator(std::string server, std::string username, std::string password, std::string database, std::string tablePrefix, int securityLevel = 1);
	~Authenticator();
	/* 
		Server: Ip address where mysql server is, in the form "192.168.1.102" (can use external ip addresses as well)
		Table Prefix: in the form "phpbb3_", just send an empty string if there's no prefix used.
		Security levels: 0 = no security (no checking for sql-injection attempts, not recommended unless you add your own security)
						 1 = basic security (prevents the use of any of these characters in the username: "(\"*^';&></) " including the space)
						 2 = alphanumeric (only allows alphanumeric characters in the username)
	*/
	void initialize(std::string server, std::string username, std::string password, std::string database, std::string tablePrefix, int securityLevel = 1);
	bool authenticate(std::string &username, std::string password);
	/*
	username: The username to verify in the database.
	password: The password the user is attempting to login with.
	returns: 'true' if the username was found and the password is correct, else 'false'.
	errorCodes:
		0 = unable to connect to mysql server
		1 = username doesn't exist
		2 = invalid password
		3 = username contains invalid characters (possibly an sql injection attempt)
	*/
	bool authenticate(std::string &username, std::string password, int &errorCode);
	/*
		0 = no security (no checking for sql-injection attempts, not recommended unless you add your own security)
		1 = basic security (prevents the use of any of these characters in the username: "(\"*^';&></) " including the space)  (only " is forbidden by PHPBB3)
		2 = alphanumeric (only allows alphanumeric characters in the username)
	*/
	void setSecurityLevel(int level);

private:
	bool isSqlSafe(std::string s);

	std::string sqlServer;
	std::string sqlUsername;
	std::string sqlPassword;
	std::string sqlDatabase;
	std::string prefix;
	mysqlpp::TCPConnection *connection;
	int securityLevel;
};

#endif


