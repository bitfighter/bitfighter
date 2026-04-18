# libtommath is required for the in-tree libtomcrypt build.
# If not found via find_package, provide a hint:
#   cmake -DTOMMATH_ROOT=/path/to/libtommath ..
# or install it with:
#   Ubuntu/Debian: sudo apt install libtommath-dev
#   macOS:         brew install libtommath
#   Windows (vcpkg): vcpkg install libtommath
find_path(TOMMATH_INCLUDE_DIR tommath.h
    HINTS ${TOMMATH_ROOT}/include ${TOMMATH_ROOT}
          /usr/include /usr/local/include)
find_library(TOMMATH_LIBRARY
    NAMES tommath
    HINTS ${TOMMATH_ROOT}/lib ${TOMMATH_ROOT}
          /usr/lib /usr/local/lib)

if(NOT TOMMATH_INCLUDE_DIR OR NOT TOMMATH_LIBRARY)
    message(FATAL_ERROR
        "libtommath not found. The in-tree libtomcrypt 1.18.2 requires "
        "libtommath for big-integer arithmetic (ECC/RSA).\n"
        "  Ubuntu/Debian: sudo apt install libtommath-dev\n"
        "  macOS:         brew install libtommath\n"
        "  Windows:       vcpkg install libtommath\n"
        "Or set TOMMATH_ROOT to the install prefix.")
endif()

message(STATUS "Found libtommath: ${TOMMATH_LIBRARY} (headers: ${TOMMATH_INCLUDE_DIR})")

add_library(tomcrypt STATIC
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/aes/aes.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/aes/aes_enc.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/aes/aes_tab.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/anubis.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/blowfish.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/camellia.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/cast5.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/des.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/kasumi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/khazad.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/kseed.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/multi2.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/noekeon.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/rc2.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/rc5.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/rc6.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/safer/safer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/safer/safer_tab.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/safer/saferp.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/skipjack.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/twofish/twofish.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/twofish/twofish_tab.c
${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/xtea.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_add_aad.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_add_nonce.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_reset.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ccm/ccm_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_add_aad.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_setiv_rfc7905.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/chachapoly/chacha20poly1305_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_addheader.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_decrypt_verify_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_encrypt_authenticate_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/eax/eax_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_add_aad.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_add_iv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_gf_mult.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_mult_h.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_reset.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/gcm/gcm_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_decrypt_verify_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_done_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_done_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_encrypt_authenticate_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_ntz.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_shift_xor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/ocb_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb/s_ocb_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_add_aad.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_decrypt_last.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_decrypt_verify_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_encrypt_authenticate_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_encrypt_last.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_int_ntz.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_int_xor_blocks.c
${CMAKE_CURRENT_SOURCE_DIR}/src/encauth/ocb3/ocb3_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/blake2b.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/blake2s.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/chc/chc.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/helper/hash_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/helper/hash_filehandle.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/helper/hash_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/helper/hash_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/md2.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/md4.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/md5.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/rmd128.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/rmd160.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/rmd256.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/rmd320.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha1.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha224.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha256.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha384.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha512.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha512_224.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha2/sha512_256.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha3.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/sha3_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/tiger.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/whirl/whirl.c
${CMAKE_CURRENT_SOURCE_DIR}/src/hashes/whirl/whirltab.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2bmac.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2bmac_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2bmac_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2bmac_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2bmac_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2smac.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2smac_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2smac_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2smac_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/blake2/blake2smac_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/f9/f9_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/hmac/hmac_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/omac/omac_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pelican/pelican.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pelican/pelican_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pelican/pelican_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_ntz.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_shift_xor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/pmac/pmac_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/poly1305/poly1305.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/poly1305/poly1305_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/poly1305/poly1305_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/poly1305/poly1305_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/poly1305/poly1305_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_file.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_memory.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_memory_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/mac/xcbc/xcbc_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/fp/ltc_ecc_fp_mulmod.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/ltm_desc.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/radix_to_bin.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/rand_bn.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/rand_prime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/math/tfm_desc.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/adler32.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/base64/base64_decode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/base64/base64_encode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/burn_stack.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/compare_testvector.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crc32.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_argchk.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_cipher_descriptor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_cipher_is_valid.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_constants.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_cipher.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_cipher_any.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_cipher_id.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_hash_any.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_hash_id.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_hash_oid.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_find_prng.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_fsa.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_hash_descriptor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_hash_is_valid.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_inits.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_ltc_mp_descriptor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_prng_descriptor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_prng_is_valid.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_prng_rng_descriptor.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_all_ciphers.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_all_hashes.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_all_prngs.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_cipher.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_register_prng.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_sizes.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_unregister_cipher.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_unregister_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/crypt/crypt_unregister_prng.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/error_to_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/hkdf/hkdf.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/hkdf/hkdf_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/mem_neq.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/pk_get_oid.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/pkcs5/pkcs_5_1.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/pkcs5/pkcs_5_2.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/pkcs5/pkcs_5_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/misc/zeromem.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cbc/cbc_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/cfb/cfb_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ctr/ctr_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ecb/ecb_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ecb/ecb_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ecb/ecb_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ecb/ecb_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/f8/f8_test_mode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_process.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/lrw/lrw_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_getiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_setiv.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/ofb/ofb_start.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_decrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_encrypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_init.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_mult_x.c
${CMAKE_CURRENT_SOURCE_DIR}/src/modes/xts/xts_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/bit/der_decode_bit_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/bit/der_decode_raw_bit_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/bit/der_encode_bit_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/bit/der_encode_raw_bit_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/bit/der_length_bit_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/boolean/der_decode_boolean.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/boolean/der_encode_boolean.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/boolean/der_length_boolean.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/choice/der_decode_choice.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/generalizedtime/der_decode_generalizedtime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/generalizedtime/der_encode_generalizedtime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/generalizedtime/der_length_generalizedtime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/ia5/der_decode_ia5_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/ia5/der_encode_ia5_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/ia5/der_length_ia5_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/integer/der_decode_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/integer/der_encode_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/integer/der_length_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/object_identifier/der_decode_object_identifier.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/object_identifier/der_encode_object_identifier.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/object_identifier/der_length_object_identifier.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/octet/der_decode_octet_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/octet/der_encode_octet_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/octet/der_length_octet_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/printable_string/der_decode_printable_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/printable_string/der_encode_printable_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/printable_string/der_length_printable_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_ex.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_decode_sequence_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_decode_subject_public_key_info.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_encode_sequence_ex.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_encode_sequence_multi.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_encode_subject_public_key_info.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_length_sequence.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_sequence_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/sequence/der_sequence_shrink.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/set/der_encode_set.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/set/der_encode_setof.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/short_integer/der_decode_short_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/short_integer/der_encode_short_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/short_integer/der_length_short_integer.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/teletex_string/der_decode_teletex_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/teletex_string/der_length_teletex_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utctime/der_decode_utctime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utctime/der_encode_utctime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utctime/der_length_utctime.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utf8/der_decode_utf8_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utf8/der_encode_utf8_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/asn1/der/utf8/der_length_utf8_string.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_check_pubkey.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_export_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_generate_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_set.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_set_pg_dhparam.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dh/dh_shared_secret.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_decrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_encrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_generate_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_generate_pqg.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_make_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_set.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_set_pqg_dsaparam.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_shared_secret.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_sign_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_verify_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/dsa/dsa_verify_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_ansi_x963_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_ansi_x963_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_decrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_encrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_get_size.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_make_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_shared_secret.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_sign_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_sizes.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ecc_verify_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_is_valid_idx.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_map.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_mul2add.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_mulmod.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_mulmod_timing.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_points.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_projective_add_point.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/ecc/ltc_ecc_projective_dbl_point.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_decrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_encrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_exptmod.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/katja/katja_make_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_i2osp.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_mgf1.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_oaep_decode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_oaep_encode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_os2ip.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_pss_decode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_pss_encode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_v1_5_decode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/pkcs1/pkcs_1_v1_5_encode.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_decrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_encrypt_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_export.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_exptmod.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_free.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_get_size.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_import.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_import_pkcs8.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_import_x509.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_make_key.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_set.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_sign_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_sign_saltlen_get.c
${CMAKE_CURRENT_SOURCE_DIR}/src/pk/rsa/rsa_verify_hash.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/chacha20.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/fortuna.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/rc4.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/rng_get_bytes.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/rng_make_prng.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/sober128.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/sprng.c
${CMAKE_CURRENT_SOURCE_DIR}/src/prngs/yarrow.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_crypt.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_done.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_ivctr32.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_ivctr64.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_keystream.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_setup.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/chacha/chacha_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/rc4/rc4_stream.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/rc4/rc4_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/sober128/sober128_stream.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/sober128/sober128_test.c
${CMAKE_CURRENT_SOURCE_DIR}/src/stream/sober128/sober128tab.c
)

target_include_directories(tomcrypt
    SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/headers
    PRIVATE
    ${TOMMATH_INCLUDE_DIR}
)

# Keep warning policy strict for project code, but suppress third-party warnings.
target_compile_options(tomcrypt PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W0 /WX- /wd28251 /analyze->
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>>:-w>
)

# ranlib on OSX is really 'libtool'. To mimic the correct behavior of ranlib for
# static libraries we need to use 'ranlib -c'
if(APPLE)
    SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -c <TARGET>")
endif()

# LTC_SOURCE: standard libtomcrypt source build flag.
# LTC_NO_ROLC: disable runtime overhead from rol/ror intrinsics.
target_compile_definitions(tomcrypt PRIVATE LTC_SOURCE LTC_NO_ROLC)

# LTM_DESC tells both the library source AND its consumers (via PUBLIC) that
# LibTomMath is the big-integer back-end.  This is necessary so that
# tomcrypt_math.h exposes the extern ltm_desc descriptor to code that links
# against tomcrypt (e.g. tnl/random.cpp where ltc_mp = ltm_desc is called).
target_compile_definitions(tomcrypt PUBLIC LTM_DESC)

# aes_enc.c must be compiled a second time with ENCRYPT_ONLY so that the
# rijndael_enc_* / aes_enc_* symbols (needed by Yarrow) are available,
# while the primary aes.c is compiled without that flag.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ciphers/aes/aes_enc.c
    PROPERTIES
    COMPILE_DEFINITIONS "ENCRYPT_ONLY"
)

# Expose libtommath to consumers of the tomcrypt target
set(TOMMATH_LIBRARY "${TOMMATH_LIBRARY}" CACHE INTERNAL "")
target_link_libraries(tomcrypt PUBLIC ${TOMMATH_LIBRARY})
