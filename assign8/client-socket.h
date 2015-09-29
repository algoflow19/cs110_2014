/**
 * File: client-socket.h
 * ---------------------
 * Presents the interface of the routine network
 * clients can use to connect to a specific port of
 * a remote host.
 */

#pragma once
#include <string>

/**
 * Constant: kClientSocketError
 * ----------------------------
 * Sentinel used to communicate that a connection
 * to a remote server could not be made.
 */

const int kClientSocketError = -1;

int createClientSocket(const struct in_addr* ip,
                       const unsigned short port);
