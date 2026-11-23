#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_client(int clientSocket);

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
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
    handle_client(clientSocket);
  }
}
