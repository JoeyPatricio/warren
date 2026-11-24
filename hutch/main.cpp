#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

void handle_client(int clientSocket, const char *wol_addr);

int main() {
  std::cout << std::unitbuf;
  const char *wol_addr = std::getenv("WOL_ADDR");
  if (wol_addr == nullptr) {
    std::cerr << "WOL_ADDR is not set\n";
    return 1;
  }
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::perror("setsockopt(SO_REUSEADDR)");
    return 1;
  }
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  if (bind(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    std::cerr << "Socket bind failed\n";
    return -1;
  }
  listen(fd, 5);
  while (true) {
    int clientSocket = accept(fd, nullptr, nullptr);
    if (clientSocket < 0) {
      std::perror("accept");
      continue;
    }
    // Clients are served one at a time, so an idle one must not block the rest.
    timeval timeout{5, 0};
    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout)) < 0) {
      std::perror("setsockopt(SO_RCVTIMEO)");
      close(clientSocket);
      continue;
    }
    handle_client(clientSocket, wol_addr);
  }
}
