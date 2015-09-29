#pragma once

#include <openssl/rsa.h>
#include <string>
#include <vector>
#include <map>

#include "txn.h"

extern int verbose;
extern bool global;
extern std::map<std::string, RSA*> userKeys;

std::string masterPortFileName();
unsigned long long smallrand();
unsigned long long lrand();
unsigned short randPort();

void log(const char *control, ...);
void vlog(const char *control, ...);

unsigned short readPortFile();
void parseUsers();
std::string getUser(RSA *key);
RSA* getKey(std::string user);

void writeVerifiedTxns(std::vector<Txn>& verifiedTxns);
void readVerifiedTxns(std::vector<Txn>& verifiedTxns);

int lastTxn(std::vector<Txn>& verifiedTxns,
            coin_id_t coinID,
            Txn& last);
