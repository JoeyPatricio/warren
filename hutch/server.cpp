#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <optional>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

std::optional<std::int64_t> queue_job(sqlite3 *db,
                                      const std::vector<std::string> &job);
int send_wol(const char *mac_address);

using Clock = std::chrono::steady_clock;

// Together these bound the memory that clients can hold to 64 MiB.
constexpr size_t max_request = 1 << 20;
constexpr size_t max_clients = 64;
constexpr std::chrono::seconds client_timeout{5};

struct Client {
  std::string request;
  Clock::time_point deadline;
};

struct Server {
  int epoll;
  int listener;
  int timer;
  const char *wol_addr;
  sqlite3 *db;
  std::unordered_map<int, Client> clients;
};

int watch(int epoll, int fd) {
  epoll_event event{};
  event.events = EPOLLIN;
  event.data.fd = fd;
  if (epoll_ctl(epoll, EPOLL_CTL_ADD, fd, &event) < 0) {
    std::perror("epoll_ctl(EPOLL_CTL_ADD)");
    return -1;
  }
  return 0;
}

int arm_timer(int timer, Clock::duration delay) {
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(delay);
  itimerspec spec{};
  spec.it_value.tv_sec = seconds.count();
  spec.it_value.tv_nsec = std::chrono::nanoseconds(delay - seconds).count();
  if (timerfd_settime(timer, 0, &spec, nullptr) < 0) {
    std::perror("timerfd_settime");
    return -1;
  }
  return 0;
}

// NUL separates the command name from its arguments. The reply is "ok", "ok"
// and a space before a result, or an error message.
std::string handle_request(const std::string &message, const char *wol_addr,
                           sqlite3 *db) {
  std::istringstream request(message);
  std::string command;
  std::getline(request, command, '\0');
  std::cout << "Command from client: " << command << "\n";
  if (command == "wake") {
    return send_wol(wol_addr) == 0 ? "ok" : "could not send wake packet";
  }
  if (command == "submit") {
    std::vector<std::string> job;
    for (std::string arg; std::getline(request, arg, '\0');) {
      job.push_back(std::move(arg));
    }
    if (job.empty() || job[0].empty()) {
      return "no command to run";
    }
    std::optional<std::int64_t> id = queue_job(db, job);
    return id ? "ok " + std::to_string(*id) : "could not queue job";
  }
  std::cerr << "Unknown command: " << command << "\n";
  return "unknown command";
}

int accept_client(Server &server) {
  int fd =
      accept4(server.listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (fd < 0) {
    // The listener stays readable when the client was not taken off its queue,
    // so carrying on after other errors would spin.
    if (errno == EAGAIN || errno == EINTR || errno == ECONNABORTED) {
      return 0;
    }
    std::perror("accept4");
    return -1;
  }
  if (watch(server.epoll, fd) < 0) {
    close(fd);
    return -1;
  }
  if (server.clients.empty() && arm_timer(server.timer, client_timeout) < 0) {
    close(fd);
    return -1;
  }
  server.clients[fd] = Client{{}, Clock::now() + client_timeout};
  if (server.clients.size() == max_clients) {
    // New connections wait in the listen backlog until a client leaves.
    if (epoll_ctl(server.epoll, EPOLL_CTL_DEL, server.listener, nullptr) < 0) {
      std::perror("epoll_ctl(EPOLL_CTL_DEL)");
      return -1;
    }
  }
  return 0;
}

int close_client(Server &server, int fd) {
  bool was_full = server.clients.size() == max_clients;
  close(fd);
  server.clients.erase(fd);
  return was_full ? watch(server.epoll, server.listener) : 0;
}

// Thump sends one command and then shuts down its write side, so the request is
// complete at EOF.
int read_client(Server &server, int fd, Client &client) {
  char buffer[4096];
  ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
  if (n < 0) {
    if (errno == EAGAIN || errno == EINTR) {
      return 0;
    }
    std::perror("recv");
    return close_client(server, fd);
  }
  if (n > 0) {
    // Keep one byte past the limit to mark the request as too large, and drop
    // the rest. Closing with unread data would reset the connection, and Thump
    // might never see the reply.
    size_t room = max_request + 1 - client.request.size();
    client.request.append(buffer, std::min<size_t>(n, room));
    return 0;
  }
  std::string reply = client.request.size() > max_request
                          ? "request too large"
                          : handle_request(client.request, server.wol_addr,
                                           server.db);
  // A reply is one short line on an unused send buffer, so it always fits.
  // Without MSG_NOSIGNAL, a client that already disconnected would kill Hutch.
  if (send(fd, reply.data(), reply.size(), MSG_NOSIGNAL) < 0) {
    std::perror("send");
  }
  return close_client(server, fd);
}

int expire_clients(Server &server) {
  // Accepting a client can rearm the timer after epoll reported it, which
  // leaves nothing to read.
  std::uint64_t expirations;
  if (read(server.timer, &expirations, sizeof(expirations)) < 0 &&
      errno != EAGAIN) {
    std::perror("read(timerfd)");
    return -1;
  }
  Clock::time_point now = Clock::now();
  Clock::time_point next = Clock::time_point::max();
  std::vector<int> expired;
  for (const auto &[fd, client] : server.clients) {
    if (client.deadline <= now) {
      expired.push_back(fd);
    } else {
      next = std::min(next, client.deadline);
    }
  }
  for (int fd : expired) {
    std::cerr << "Client timed out\n";
    if (close_client(server, fd) < 0) {
      return -1;
    }
  }
  return server.clients.empty() ? 0 : arm_timer(server.timer, next - now);
}

// Returns 0 after SIGTERM or SIGINT, or -1 on an error. The listener must be
// nonblocking.
int run_server(int listener, const char *wol_addr, sqlite3 *db) {
  sigset_t stop_signals;
  sigemptyset(&stop_signals);
  sigaddset(&stop_signals, SIGTERM);
  sigaddset(&stop_signals, SIGINT);
  if (sigprocmask(SIG_BLOCK, &stop_signals, nullptr) < 0) {
    std::perror("sigprocmask");
    return -1;
  }
  int signals = signalfd(-1, &stop_signals, SFD_NONBLOCK | SFD_CLOEXEC);
  if (signals < 0) {
    std::perror("signalfd");
    return -1;
  }
  int timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timer < 0) {
    std::perror("timerfd_create");
    return -1;
  }
  int epoll = epoll_create1(EPOLL_CLOEXEC);
  if (epoll < 0) {
    std::perror("epoll_create1");
    return -1;
  }
  if (watch(epoll, listener) < 0 || watch(epoll, timer) < 0 ||
      watch(epoll, signals) < 0) {
    return -1;
  }
  Server server{epoll, listener, timer, wol_addr, db, {}};
  epoll_event events[16];
  while (true) {
    int count = epoll_wait(epoll, events, std::size(events), -1);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::perror("epoll_wait");
      return -1;
    }
    for (int i = 0; i < count; ++i) {
      int fd = events[i].data.fd;
      int result = 0;
      if (fd == signals) {
        return 0;
      } else if (fd == listener) {
        result = accept_client(server);
      } else if (fd == timer) {
        result = expire_clients(server);
      } else if (auto client = server.clients.find(fd);
                 client != server.clients.end()) {
        // An earlier event in this batch may have closed the client.
        result = read_client(server, fd, client->second);
      }
      if (result < 0) {
        return -1;
      }
    }
  }
}
