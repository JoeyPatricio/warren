#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int send_wol(const char *mac_address);

int wol();

// Thump sends one command and then closes its end, so read until EOF.
void handle_client(int clientSocket) {
  std::string message;
  char buffer[1024];
  ssize_t n;
  while ((n = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
    message.append(buffer, n);
  }
  close(clientSocket);
  if (n < 0) {
    std::perror("recv");
    return;
  }
  std::cout << "Message from client: " << message << "\n";
  if (message == "wake") {
    wol();
  } else {
    std::cerr << "Unknown command: " << message << "\n";
  }
}

int wol() {
  const char *wol_addr = std::getenv("WOL_ADDR");
  if (wol_addr == nullptr) {
    std::cerr << "WOL_ADDR is not set\n";
    return 1;
  }
  return send_wol(wol_addr) == 0 ? 0 : 1;
}
