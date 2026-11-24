#include "commands/commands.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

struct Command {
  std::string_view name;
  std::string_view description;
  int (*run)(const Context &context, const Args &args);
};

const Command commands[] = {
    {"wake", "Wake the workstation", run_wake},
};

void print_usage() {
  std::cerr << "Usage: thump <command> [args]\n\nCommands:\n";
  for (const Command &command : commands) {
    std::cerr << "  " << command.name << "\t" << command.description << "\n";
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 2;
  }
  std::string_view name = argv[1];
  for (const Command &command : commands) {
    if (command.name != name) {
      continue;
    }
    const char *hutch_addr = std::getenv("HUTCH_ADDR");
    if (hutch_addr == nullptr) {
      std::cerr << "HUTCH_ADDR is not set\n";
      return 1;
    }
    return command.run(Context{hutch_addr}, Args(argv + 2, argv + argc));
  }
  std::cerr << "Unknown command: " << name << "\n";
  print_usage();
  return 2;
}
