#include <cstdio>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int send_wol(const char *mac_address);

// Thump sends one command and then shuts down its write side, so read until
// EOF. The reply is "ok" or an error message.
void handle_client(int clientSocket, const char *wol_addr) {
  std::string message;
  char buffer[1024];
  ssize_t n;
  while ((n = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
    message.append(buffer, n);
  }
  if (n < 0) {
    std::perror("recv");
    close(clientSocket);
    return;
  }
  std::cout << "Message from client: " << message << "\n";
  std::string reply = "ok";
  if (message == "wake") {
    if (send_wol(wol_addr) != 0) {
      reply = "could not send wake packet";
    }
  } else {
    std::cerr << "Unknown command: " << message << "\n";
    reply = "unknown command";
  }
  // Without MSG_NOSIGNAL, a client that already disconnected would kill Hutch.
  if (send(clientSocket, reply.data(), reply.size(), MSG_NOSIGNAL) < 0) {
    std::perror("send");
  }
  close(clientSocket);
}
