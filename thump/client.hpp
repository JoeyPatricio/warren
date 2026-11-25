#pragma once

#include <optional>
#include <string>
#include <string_view>

// Returns the text after "ok" in Hutch's reply, or nullopt after printing an
// error.
std::optional<std::string> send_command(const char *hutch_addr,
                                        std::string_view command);
