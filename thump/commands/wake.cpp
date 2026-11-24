#include "../client.hpp"
#include "commands.hpp"

#include <iostream>

int run_wake(const Context &context, const Args &args) {
  if (!args.empty()) {
    std::cerr << "Usage: thump wake\n";
    return 2;
  }
  return send_command(context.hutch_addr, "wake") == 0 ? 0 : 1;
}
