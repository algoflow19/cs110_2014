/**
 * File: ip-lookup.cc
 * ------------------
 * This is a simple little program that helps 
 * resolve IP addresses.
 */

#include <iostream>        // for cout
#include <netdb.h>         // for gethostbyname
#include <sys/socket.h>    // for inet_ntoa
#include <netinet/in.h>    // also for inet_ntoa (see man page)
#include <arpa/inet.h>     // also for inet_ntoa (see man page)
#include "string-utils.h"
using namespace std;

/**
 * Function: publishIPAddressInfo
 * ------------------------------
 * Relies on the services of the inet_ntoa and 
 * gethostbyname functions to produce the official
 * host name for the supplied one (some times the same,
 * and some times different) and all of the IP addresses
 * that the supplied host name resolves to.
 */

static void publishIPAddressInfo(const string& host) {
  struct hostent *he = gethostbyname(host.c_str());
  if (he == NULL) { // NULL return value means resolution attempt failed
    cout << host << " could not be resolved to an address.  Did you mistype it?" << endl;
    return;
  }
  
  cout << "Official name is \"" << he->h_name << "\"" << endl;
  cout << "IP Addresses: " << endl;
  struct in_addr **addressList = (struct in_addr **) he->h_addr_list;
  while (*addressList != NULL) {
    cout << "+ " << inet_ntoa(**addressList) << endl;
    addressList++;
  }
}

/**
 * Function: resolveIPAddresses
 * ----------------------------
 * Straightforward query loop asking for the user to
 * type in what's expected to look like a host name
 * (e.g. "www.facebook.com") or an IP address (e.g. "171.64.64.137")
 */

static void resolveIPAddresses() {
  while (true) {
    cout << "Enter a host name: ";
    string host;
    getline(cin, host);
    trim(host);
    if (host.empty()) break;
    publishIPAddressInfo(host);
  }
  
  cout << endl;
  cout << "All done!" << endl;
}

/**
 * Function: main
 * --------------
 * Straightforward wrapper around resolveIPAddress, 
 * which does all of the interest work.
 */

int main(int argc, char *argv[]) {
  cout << "Welcome to the IP address resolver" << endl;
  resolveIPAddresses();
  return 0;
}
