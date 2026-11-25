#include "client.hpp"

#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

std::optional<std::string> send_command(const char *hutch_addr,
                                        std::string_view command) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *addr;
  int err = getaddrinfo(hutch_addr, "8080", &hints, &addr);
  if (err != 0) {
    std::cerr << "Could not resolve " << hutch_addr << ": "
              << gai_strerror(err) << "\n";
    return std::nullopt;
  }
  int clientSocket = socket(addr->ai_family, addr->ai_socktype, 0);
  if (clientSocket < 0) {
    std::perror("socket");
    freeaddrinfo(addr);
    return std::nullopt;
  }
  int connected = connect(clientSocket, addr->ai_addr, addr->ai_addrlen);
  freeaddrinfo(addr);
  if (connected < 0) {
    std::perror("connect");
    close(clientSocket);
    return std::nullopt;
  }
  // Longer than Hutch's 5 s limit on a client, so a full Hutch has time to make
  // room for us.
  timeval timeout{10, 0};
  if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout)) < 0) {
    std::perror("setsockopt(SO_RCVTIMEO)");
    close(clientSocket);
    return std::nullopt;
  }
  // Without MSG_NOSIGNAL, Hutch closing early would kill Thump with no message.
  if (send(clientSocket, command.data(), command.size(), MSG_NOSIGNAL) < 0) {
    std::perror("send");
    close(clientSocket);
    return std::nullopt;
  }
  // Hutch reads until EOF before it replies.
  if (shutdown(clientSocket, SHUT_WR) < 0) {
    std::perror("shutdown");
    close(clientSocket);
    return std::nullopt;
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
    return std::nullopt;
  }
  close(clientSocket);
  if (reply.empty()) {
    std::cerr << "Hutch closed the connection without replying\n";
    return std::nullopt;
  }
  if (reply == "ok") {
    return "";
  }
  if (reply.compare(0, 3, "ok ") == 0) {
    return reply.substr(3);
  }
  std::cerr << "Hutch: " << reply << "\n";
  return std::nullopt;
}
