#pragma once

#include <openssl/rsa.h>
#include <string>

// length of a 1024 bit RSA public key in PEM SubjectPublicKeyInfo format
const int kPubKeyLen = 272;

// length of a signature computed from a 1024 bit RSA private key
const int kSigLen = 128;

const std::string kPrivKeyFname = "privkey.pem";

void RSASign(RSA* key, void* data, int len, unsigned char* sigret);
int RSAVerify(RSA* key, void* data, int len, unsigned char* sig);

RSA* readPrivKey(std::string fname);
RSA* readPubKey(std::string fname);
char* charPubKey(RSA* key);
RSA* rsaPubKey(char* pubkey);

// Compute the hash of the given data buffer.  The buffer should be of
// length "len".  The hash is stored in "hash".
void computeHash(void* data, int len, unsigned char* hash);
