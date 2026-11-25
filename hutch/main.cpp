#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

int run_server(int listener, const char *wol_addr);

int main() {
  std::cout << std::unitbuf;
  const char *wol_addr = std::getenv("WOL_ADDR");
  if (wol_addr == nullptr) {
    std::cerr << "WOL_ADDR is not set\n";
    return 1;
  }
  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (fd < 0) {
    std::perror("socket");
    return 1;
  }
  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::perror("setsockopt(SO_REUSEADDR)");
    return 1;
  }
  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  if (bind(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    std::cerr << "Socket bind failed\n";
    return -1;
  }
  if (listen(fd, SOMAXCONN) < 0) {
    std::perror("listen");
    return 1;
  }
  return run_server(fd, wol_addr) == 0 ? 0 : 1;
}
