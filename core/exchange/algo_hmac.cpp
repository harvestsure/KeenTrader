#include "algo_hmac.h"
#include <openssl/evp.h>
#include <openssl/params.h>
#include <string.h>
#include <iostream>

using namespace std;

#ifdef _MSC_VER
int strcasecmp(const char *s1, const char *s2)
{
    while (toupper((unsigned char)*s1) == toupper((unsigned char)*s2++))
        if (*s1++ == '\0')
            return 0;

    return(toupper((unsigned char)*s1) - toupper((unsigned char)*--s2));
}
#endif

int HmacEncode(const char * algo,
        const char * key, unsigned int key_length,
        const char * input, unsigned int input_length,
        unsigned char * &output, unsigned int &output_length) {
    const EVP_MD *engine = NULL;
    if (strcasecmp("sha512", algo) == 0) {
        engine = EVP_sha512();
    } else if (strcasecmp("sha256", algo) == 0) {
        engine = EVP_sha256();
    } else if (strcasecmp("sha1", algo) == 0) {
        engine = EVP_sha1();
    } else if (strcasecmp("md5", algo) == 0) {
        engine = EVP_md5();
    } else if (strcasecmp("sha224", algo) == 0) {
        engine = EVP_sha224();
    } else if (strcasecmp("sha384", algo) == 0) {
        engine = EVP_sha384();
    } else {
        cout << "Algorithm " << algo << " is not supported by this program!" << endl;
        return -1;
    }

    output = (unsigned char*)malloc(EVP_MD_size(engine));
    if (output == NULL) {
        cout << "Memory allocation failed!" << endl;
        return -1;
    }

    EVP_MAC_CTX *ctx = NULL;
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (mac == NULL) {
        cout << "EVP_MAC_fetch() failed!" << endl;
        free(output);
        return -1;
    }

    ctx = EVP_MAC_CTX_new(mac);
    if (ctx == NULL) {
        cout << "EVP_MAC_CTX_new() failed!" << endl;
        EVP_MAC_free(mac);
        free(output);
        return -1;
    }
    
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("digest", (char *)EVP_MD_name(engine), 0),
        OSSL_PARAM_construct_end()
    };

    if (EVP_MAC_init(ctx, (const unsigned char *)key, key_length, params) != 1) {
        cout << "EVP_MAC_init() failed!" << endl;
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        free(output);
        return -1;
    }

    if (EVP_MAC_update(ctx, (const unsigned char *)input, input_length) != 1) {
        cout << "EVP_MAC_update() failed!" << endl;
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        free(output);
        return -1;
    }

    size_t len = 0;  // This will hold the actual length of the output
    if (EVP_MAC_final(ctx, output, &len, EVP_MD_size(engine)) != 1) {
        cout << "EVP_MAC_final() failed!" << endl;
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        free(output);
        return -1;
    }

    output_length = len;

    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    return 0;
}
