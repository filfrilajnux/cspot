#ifndef CRYPTO_H
#define CRYPTO_H

#include <vector>
#include <string>

#ifdef USE_MBEDTLS
#include "CryptoMbedTLS.h"
#define Crypto CryptoMbedTLS
#else
#include "CryptoOpenSSL.h"
#define Crypto CryptoOpenSSL
#endif
#endif