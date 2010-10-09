/*
Copyright (c) 2009, William M Brandt (aka 'Taekvideo')
[[Minor modifications for Bitfighter usage by Chris Eykamp]]

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


#include "phpbbhash.h"
#include "mycrypt.h"
#include <ctype.h>
#include <math.h>


PHPBB3Password::PHPBB3Password(){
	itoa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}

bool PHPBB3Password::check_hash(string password, string hash) {
	if (hash.length() == 34){
		if (do_hash(password, hash).compare(hash) == 0) return true;
		return false;
	}
	else {
		if (md5(password).compare(hash) == 0) return true;
		return false;
	}
}


string PHPBB3Password::md5(string data) {
	unsigned char hash[16];
	hash_state state;
	md5_init(&state);

	md5_process(&state, (const unsigned char *)data.c_str(), data.length());
	md5_done(&state, hash);
	string ret;
	for (int i=0; i<16; i++) ret += hash[i];
	return ret;
}

string PHPBB3Password::encode(string input, int count) {
	string output = "";
	int i = 0;

	do {
		int value = (unsigned char)input[i++];
		output += itoa64[value & 0x3f];
		if (i < count) value |= (((unsigned char)input[i]) << 8);
		output += itoa64[(value >> 6) & 0x3f];
		if (i++ >= count) break;
		if (i < count) value |= (((unsigned char)input[i]) << 16);
		output += itoa64[(value >> 12) & 0x3f];
		if (i++ >= count) break;
		output += itoa64[(value >> 18) & 0x3f];
	} while (i < count);

	return output;

}

string PHPBB3Password::do_hash(string password, string setting) {
	string output = "*";

	// Check for correct hash, return if not found
	if (setting.substr(0, 3).compare("$H$")) return output;

	int count_log2 = itoa64.find_first_of(setting[3]);
	if (count_log2 < 7 || count_log2 > 30) return output;

	int count = 1 << count_log2;
	string salt = setting.substr(4, 8);
	if (salt.length() != 8) return output;

	string hash = md5(salt + password);
	do {
	  hash = md5(hash + password);
	} while (--count);

	output = setting.substr(0, 12);
	output += encode(hash, 16);

	return output;
}

