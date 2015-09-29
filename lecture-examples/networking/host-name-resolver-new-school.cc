/**
 * File: host-name-resolver-new-school.cc
 * --------------------------------------
 * This is functionally similar to 
 * host-name-resolver-old-school.cc, except that it 
 * uses getaddrinfo and freeaddrinfo functions (not 
 * mentioned in your B&O textbook).  getaddrinfo and
 * freeaddrinfo also provide network address translation
 * routines, and a third function called getnameinfo is used
 * to convert information housed in each node of the linked
 * list returned by getaddrinfo into an IP address.
 *
 * I'm perfectly happy with your using the old-school approach, as
 * it's the approach taken by the book, but at least know that
 * the old-school approach is technically considered to be deprecated,
 * even though everyone still uses it.
 */

#include <iostream>        // for cout
#include <set>             // for set (caches ip addresses)
#include <sys/types.h>     // for getaddrinfo/freeaddrinfo (see man pages)
#include <sys/socket.h>    // for getaddrinfo/freeaddrinfo/inet_ntoa (see man pages)
#include <netdb.h>         // for getaddrinfo/freeaddrinfo (see man pages)
#include <netinet/in.h>    // for inet_ntoa (see man page)
#include <arpa/inet.h>     // for inet_ntoa (see man page)
#include "string-utils.h"
using namespace std;

/**
 * Function: publishIPAddressInfo
 * ------------------------------
 * Relies on the more hip getaddrinfo, getnameinfo, and
 * freeaddrinfo to do very much the same thing that
 * the old school's publishIPAddressInfo function does.
 */

static void publishIPAddressInfo(const string& host) {
  struct addrinfo *infoList;
  int ret = 
    getaddrinfo(host.c_str(), /* service = */ NULL,
		/* hints = */ NULL, &infoList);
  if (ret != 0) {
    cout << host << " could not be resolved to anything" << endl;
    return;
  }

  set<string> ipAddresses;
  addrinfo *curr = infoList;
  while (curr != NULL) {
    char ipAddress[64] = {'\0'};
    int ret = getnameinfo(curr->ai_addr, curr->ai_addrlen,
			  ipAddress, sizeof(ipAddress),
			  /* service = */ NULL, /* servlen = */ 0,
			  NI_NUMERICHOST);
    if (ret == 0 && ipAddress[0] != '\0' && 
	ipAddresses.find(ipAddress) == ipAddresses.cend()) {
      ipAddresses.insert(ipAddress);
      cout << "+ " << ipAddress << endl;
    }
    curr = curr->ai_next;
  }
  freeaddrinfo(infoList);
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
