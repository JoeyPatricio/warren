#include "client.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

int send_command(const char *hutch_addr, const char *command) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *addr;
  int err = getaddrinfo(hutch_addr, "8080", &hints, &addr);
  if (err != 0) {
    std::cerr << "Could not resolve " << hutch_addr << ": "
              << gai_strerror(err) << "\n";
    return -1;
  }
  int clientSocket = socket(addr->ai_family, addr->ai_socktype, 0);
  if (clientSocket < 0) {
    std::perror("socket");
    freeaddrinfo(addr);
    return -1;
  }
  int connected = connect(clientSocket, addr->ai_addr, addr->ai_addrlen);
  freeaddrinfo(addr);
  if (connected < 0) {
    std::perror("connect");
    close(clientSocket);
    return -1;
  }
  // Longer than Hutch's 5 s limit on a client, so a full Hutch has time to make
  // room for us.
  timeval timeout{10, 0};
  if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout)) < 0) {
    std::perror("setsockopt(SO_RCVTIMEO)");
    close(clientSocket);
    return -1;
  }
  // Without MSG_NOSIGNAL, Hutch closing early would kill Thump with no message.
  if (send(clientSocket, command, std::strlen(command), MSG_NOSIGNAL) < 0) {
    std::perror("send");
    close(clientSocket);
    return -1;
  }
  // Hutch reads until EOF before it replies.
  if (shutdown(clientSocket, SHUT_WR) < 0) {
    std::perror("shutdown");
    close(clientSocket);
    return -1;
  }
  std::string reply;
  char buffer[1024];
  ssize_t n;
  while ((n = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
    reply.append(buffer, n);
  }
  if (n < 0) {
    std::perror("recv");
    close(clientSocket);
    return -1;
  }
  close(clientSocket);
  if (reply.empty()) {
    std::cerr << "Hutch closed the connection without replying\n";
    return -1;
  }
  if (reply != "ok") {
    std::cerr << "Hutch: " << reply << "\n";
    return -1;
  }
  return 0;
}
