#pragma once

#include <string_view>
#include <vector>

struct Context {
  const char *hutch_addr;
};

// Arguments after the command name.
using Args = std::vector<std::string_view>;

// Every handler returns Thump's exit code: 0 success, 1 failure, 2 usage error.
int run_wake(const Context &context, const Args &args);
int run_submit(const Context &context, const Args &args);
