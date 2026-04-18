/* Compile aes.c a second time with ENCRYPT_ONLY to produce the
 * rijndael_enc_* / aes_enc_* symbols used by Yarrow.
 * This file exists only so CMake can compile it with
 * -DENCRYPT_ONLY while aes.c is compiled without that flag. */
#define ENCRYPT_ONLY
#include "aes.c"
