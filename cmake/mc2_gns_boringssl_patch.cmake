# Three small changes so GameNetworkingSockets v1.6.1 builds with BoringSSL, which it
# does not support as shipped. Run from its source directory. Stops if a line it looks
# for is gone, which is the sign to look again after changing the version.
#
# - HMAC-SHA256 through HMAC(), as the same file already does for SHA-1; BoringSSL has
#   no EVP_PKEY_new_mac_key. Same result by definition, and the handshake needs it to
#   match a peer built with BCrypt (Windows), so that is worth checking by hand.
# - No custom random source: GNS skips it on Android already, and it has the wrong
#   signature for BoringSSL, whose own source is the system's.
# - No CRYPTO_cleanup_all_ex_data, which BoringSSL does not have and does not need.

function(mc2_patch file)
    file(READ "${file}" text)
    if(text MATCHES "OPENSSL_IS_BORINGSSL")
        return()	# already done
    endif()
    list(LENGTH ARGN count)
    math(EXPR last "${count} - 1")
    foreach(i RANGE 0 ${last} 2)
        math(EXPR j "${i} + 1")
        list(GET ARGN ${i} old)
        list(GET ARGN ${j} new)
        string(FIND "${text}" "${old}" found)
        if(found EQUAL -1)
            message(FATAL_ERROR "mc2_gns: not found in ${file}: ${old}")
        endif()
        string(REPLACE "${old}" "${new}" text "${text}")
    endforeach()
    file(WRITE "${file}" "${text}")
endfunction()

mc2_patch(src/common/crypto_digest_opensslevp.cpp
    "\tEVPCTXPointer<EVP_MD_CTX *, EVP_MD_CTX_free> mdctx(EVP_MD_CTX_create())"
    "#ifdef OPENSSL_IS_BORINGSSL\n\tunsigned int hmacLength = (unsigned int)sizeof(SHA256Digest_t)\;\n\tVerifyFatal( HMAC( EVP_sha256(), pubKey, cubKey, pubData, cubData, (unsigned char*)pOutputDigest, &hmacLength ) != NULL )\;\n#else\n\tEVPCTXPointer<EVP_MD_CTX *, EVP_MD_CTX_free> mdctx(EVP_MD_CTX_create())"
    "\tVerifyFatal(EVP_DigestSignFinal(mdctx.ctx, *pOutputDigest, &needed) == 1)\;"
    "\tVerifyFatal(EVP_DigestSignFinal(mdctx.ctx, *pOutputDigest, &needed) == 1)\;\n#endif")

mc2_patch(src/common/opensslwrapper.cpp
    "#if !IsAndroid() && ( OPENSSL_VERSION_NUMBER < 0x30000000 )"
    "#if !IsAndroid() && ( OPENSSL_VERSION_NUMBER < 0x30000000 ) && !defined(OPENSSL_IS_BORINGSSL)"
    "\t\tCRYPTO_cleanup_all_ex_data()\;"
    "#ifndef OPENSSL_IS_BORINGSSL\n\t\tCRYPTO_cleanup_all_ex_data()\;\n#endif")
