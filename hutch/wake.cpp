#include <charconv>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h> // sockaddr_in, htons
#include <sys/socket.h> // socket, setsockopt, sendto, SOL_SOCKET, SO_BROADCAST
#include <unistd.h>     // close

int send_wol(const char *mac_address) {
  unsigned char mac[6];
  if (std::strlen(mac_address) != 17) {
    std::cerr << "Expected a MAC address in XX:XX:XX:XX:XX:XX format\n";
    return -1;
  }
  for (size_t i = 0; i < sizeof(mac); ++i) {
    const char *octet = mac_address + 3 * i;
    auto result = std::from_chars(octet, octet + 2, mac[i], 16);
    if (result.ec != std::errc{} || result.ptr != octet + 2 ||
        (i > 0 && octet[-1] != ':')) {
      std::cerr << "Expected a MAC address in XX:XX:XX:XX:XX:XX format\n";
      return -1;
    }
  }
  unsigned char packet[6 + 16 * sizeof(mac)];
  std::memset(packet, 0xff, 6);
  for (int i = 0; i < 16; ++i) {
    std::memcpy(packet + 6 + i * sizeof(mac), mac, sizeof(mac));
  }

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    std::perror("socket");
    return -1;
  }
  int optval = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
    std::perror("setsockopt(SO_BROADCAST)");
    close(fd);
    return -1;
  }
  sockaddr_in broadcastAddr{};
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_port = htons(9);
  broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  ssize_t sent = sendto(fd, packet, sizeof(packet), 0,
                        reinterpret_cast<sockaddr *>(&broadcastAddr),
                        sizeof(broadcastAddr));
  if (sent < 0) {
    std::perror("sendto");
    close(fd);
    return -1;
  }
  close(fd);
  std::cout << "Wake packet sent\n";
  return 0;
}
