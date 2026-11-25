#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sqlite3.h>
#include <string>
#include <sys/socket.h>

int create_jobs_table(sqlite3 *db);
int run_server(int listener, const char *wol_addr, sqlite3 *db);

int main() {
  std::cout << std::unitbuf;
  const char *wol_addr = std::getenv("WOL_ADDR");
  if (wol_addr == nullptr) {
    std::cerr << "WOL_ADDR is not set\n";
    return 1;
  }
  // systemd sets this from StateDirectory=.
  const char *state_dir = std::getenv("STATE_DIRECTORY");
  if (state_dir == nullptr) {
    std::cerr << "STATE_DIRECTORY is not set\n";
    return 1;
  }
  std::string db_path = std::string(state_dir) + "/hutch.db";
  sqlite3 *handle = nullptr;
  int opened = sqlite3_open(db_path.c_str(), &handle);
  // A failed open still returns a handle to close, so take it before checking.
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(handle, sqlite3_close);
  if (opened != SQLITE_OK) {
    std::cerr << "Could not open " << db_path << ": "
              << sqlite3_errmsg(db.get()) << "\n";
    return 1;
  }
  if (create_jobs_table(db.get()) < 0) {
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
  return run_server(fd, wol_addr, db.get()) == 0 ? 0 : 1;
}
