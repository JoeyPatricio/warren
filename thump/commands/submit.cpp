#include "../client.hpp"
#include "commands.hpp"

#include <iostream>
#include <optional>
#include <string>

int run_submit(const Context &context, const Args &args) {
  if (args.size() < 2 || args[0] != "--") {
    std::cerr << "Usage: thump submit -- <cmd> [args]\n";
    return 2;
  }
  // Arguments may contain spaces or newlines but never NUL, so ending each
  // field with NUL keeps them intact.
  std::string request = "submit";
  request += '\0';
  for (size_t i = 1; i < args.size(); ++i) {
    request += args[i];
    request += '\0';
  }
  std::optional<std::string> id = send_command(context.hutch_addr, request);
  if (!id) {
    return 1;
  }
  std::cout << *id << "\n";
  return 0;
}
