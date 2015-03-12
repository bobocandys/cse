/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.
#include <string>
#include <cstring>
#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd".

  // MISSING:
  struct addrinfo *result;
  struct addrinfo hints = {0};

  hints.ai_family = ai_family;      // allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "INADDR_ANY"
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  int res;
  char port_string[6] = {0};
  sprintf(port_string, "%d", (int)port_);
  
  res = getaddrinfo(NULL, port_string, &hints, &result);
  if (res) {
    return false;
  }

  int lsn_fd = -1;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    lsn_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (lsn_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
      lsn_fd = 0;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(lsn_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(lsn_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(lsn_fd);
    lsn_fd = -1;
  }

  if (listen(lsn_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << strerror(errno) << std::endl;
    close(lsn_fd);
    return false;
  }
  
  sock_family_ = result->ai_family;
  freeaddrinfo(result);
  listen_sock_fd_ = *listen_fd = lsn_fd;
  return true;
}

bool ServerSocket::Accept(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dnsname,
                          std::string *server_addr,
                          std::string *server_dnsname) {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // MISSING:
  struct sockaddr_storage clt_addr;
  socklen_t clt_addrlen = sizeof(clt_addr);
  char clt_dns[NI_MAXHOST], clt_ip[NI_MAXHOST], clt_port[NI_MAXSERV];
  struct sockaddr_storage srv_addr;
  socklen_t srv_addrlen = sizeof(srv_addr);
  char srv_dns[NI_MAXHOST], srv_ip[NI_MAXHOST], srv_port[NI_MAXSERV];
  int newfd;

  /* accept(fd, NULL, NULL) */
  newfd = accept(listen_sock_fd_, (struct sockaddr *)&clt_addr, &clt_addrlen);
  if (newfd < 0) return false;

  getnameinfo((struct sockaddr *)&clt_addr, clt_addrlen, clt_dns,
	      sizeof(clt_dns), NULL, 0, 0);
  getnameinfo((struct sockaddr *)&clt_addr, clt_addrlen, clt_ip,
	      sizeof(clt_ip), clt_port, sizeof(clt_port),
	      NI_NUMERICHOST | NI_NUMERICSERV);

  *accepted_fd = newfd;  
  *client_addr = std::string(clt_ip);
  *client_dnsname = std::string(clt_dns);

  uint16_t count = 1;
  *client_port = 0;
  for (uint16_t i = strlen(clt_port); i > 0; i--) {
    *client_port += count * (clt_port[i - 1] - '0');
    count *= 10;
  }
  
 
  getsockname(newfd, (struct sockaddr *)&srv_addr, &srv_addrlen);
  getnameinfo((struct sockaddr *)&srv_addr, srv_addrlen, srv_dns,
	      sizeof(srv_dns), NULL, 0, 0);
  getnameinfo((struct sockaddr *)&srv_addr, srv_addrlen, srv_ip,
	      sizeof(srv_ip), srv_port, sizeof(srv_port),
	      NI_NUMERICHOST | NI_NUMERICSERV);
  
  *server_addr = std::string(srv_ip);
  *server_dnsname = std::string(srv_dns);

  return true;
}

}  // namespace hw4
