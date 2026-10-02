#pragma once

#include <string>

// Demonstration-only EventSystem payload. The stock application does NOT
// publish DemoLogEvent in production; features use Qt signals for real data
// (see AGENTS.md Production status of DemoLogEvent / AppLog). Named Demo* so
// code shape does not imply a live production event flow.
struct DemoLogEvent {
  std::string message;
};
