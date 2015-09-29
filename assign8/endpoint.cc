#include <arpa/inet.h>
#include <cstring>
#include <sstream>

#include "endpoint.h"
#include "client-socket.h"

using namespace std;

int Endpoint::connect() const {
 int sock = createClientSocket(&ip, port);
 return sock == kClientSocketError ? -1 : sock;
}

string Endpoint::toString() const {
  ostringstream ss;
  ss << inet_ntoa(ip) << ":" << port;
  return ss.str();
}

bool operator < (const Endpoint& e1, const Endpoint& e2) {
  return memcmp(&e1, &e2, sizeof(Endpoint)) < 0;
}
