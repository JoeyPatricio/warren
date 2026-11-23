#include <cstdlib>
#include <iostream>

int send_wol(const char *mac_address);

int main() {
  const char *wol_addr = std::getenv("WOL_ADDR");
  if (wol_addr == nullptr) {
    std::cerr << "WOL_ADDR is not set\n";
    return 1;
  }
  return send_wol(wol_addr) == 0 ? 0 : 1;
}
