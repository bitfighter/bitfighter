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

#ifndef PHPBBHASH_H
#define PHPBBHASH_H

#include <string>

using namespace std;

class PHPBB3Password {
public:
	PHPBB3Password();

	string do_hash(string password, string setting);
	bool check_hash(string password, string hash);

	string encode(string input, int count);
	string md5(string data);
	
private:
	string itoa64;
};

#endif


