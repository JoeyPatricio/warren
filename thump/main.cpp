#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

int send_command(const char *command);

int main(int argc, char **argv) {
  if (argc < 2 || std::strcmp(argv[1], "wake") != 0) {
    std::cerr << "Usage: thump wake\n";
    return 1;
  }
  return send_command(argv[1]) == 0 ? 0 : 1;
}

int send_command(const char *command) {
  const char *hutch_addr = std::getenv("HUTCH_ADDR");
  if (hutch_addr == nullptr) {
    std::cerr << "HUTCH_ADDR is not set\n";
    return -1;
  }
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
  if (send(clientSocket, command, std::strlen(command), 0) < 0) {
    std::perror("send");
    close(clientSocket);
    return -1;
  }
  close(clientSocket);
  return 0;
}
