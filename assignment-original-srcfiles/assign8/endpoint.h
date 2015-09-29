#pragma once

#include <netdb.h>
#include <string>

class Endpoint {
 public:
  struct in_addr ip;
  unsigned short port;

  std::string toString() const;
  int connect() const;

  friend bool operator < (const Endpoint& e1, const Endpoint& e2);
};
