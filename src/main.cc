// wtop: a Windows Task Manager style "top" for the terminal.

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "app.h"
#include "core/file_util.h"
#include "terminal/terminal.h"

namespace {

constexpr std::string_view kUsage =
    R"(Usage: wtop [options]

wtop - a Windows Task Manager style "top": a system monitor for the terminal.

Options:
  -d, --delay <ms>       Refresh interval in milliseconds (default 1000,
                         minimum 100)
  -t, --tab <n>          Tab to open first (0 = CPU, 1 = Memory, ...)
  -s, --snapshot [WxH]   Print every tab once as plain text and exit
                         (default size: the terminal, or 120x40)
  -h, --help             Show this help and exit
  -v, --version          Show the version and exit

Keys (htop style):
  F1                     Show / hide key bindings
  F10                    Quit

Keys (tmux style, prefix Ctrl+b):
  C-b n / C-b p          Next / previous tab
  C-b 0..9               Go to tab by number
  C-b l                  Last used tab
  C-b w                  Choose a tab from a list
  C-b ?                  Show key bindings
  C-b d, q, Ctrl+c       Quit

Platform: Linux. See wtop(1) for details: man wtop
)";

constexpr std::string_view kVersionNotice =
    R"(Copyright (c) 2026 Tom J.
License MIT: <https://opensource.org/licenses/MIT>
This is free software: you are free to change and redistribute it.
)";

struct ParsedArgs {
  wtop::Options options;
  bool snapshot = false;
  int snapshot_width = 0;
  int snapshot_height = 0;
};

// Parses "WxH", e.g. "120x40".
bool ParseSize(std::string_view text, int& width, int& height) {
  const auto x = text.find('x');
  if (x == std::string_view::npos) return false;
  const auto w = wtop::ParseNumber<int>(text.substr(0, x));
  const auto h = wtop::ParseNumber<int>(text.substr(x + 1));
  if (!w || !h || *w < 20 || *h < 10) return false;
  width = *w;
  height = *h;
  return true;
}

// Returns the exit code to stop with, or std::nullopt to continue.
std::optional<int> ParseArgs(int argc, char** argv, ParsedArgs& args) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    const auto next_value = [&]() -> std::optional<std::string_view> {
      if (i + 1 >= argc) return std::nullopt;
      return std::string_view(argv[++i]);
    };
    if (arg == "-h" || arg == "--help") {
      std::cout << kUsage;
      return 0;
    }
    if (arg == "-v" || arg == "--version") {
      std::cout << "wtop " << WTOP_VERSION << '\n' << kVersionNotice;
      return 0;
    }
    if (arg == "-d" || arg == "--delay") {
      const auto value = next_value();
      const auto ms = value ? wtop::ParseNumber<int>(*value) : std::nullopt;
      if (!ms || *ms < 100) {
        std::cerr << "wtop: --delay expects milliseconds >= 100\n";
        return 2;
      }
      args.options.interval = std::chrono::milliseconds(*ms);
    } else if (arg == "-t" || arg == "--tab") {
      const auto value = next_value();
      const auto tab = value ? wtop::ParseNumber<int>(*value) : std::nullopt;
      if (!tab || *tab < 0) {
        std::cerr << "wtop: --tab expects a tab number\n";
        return 2;
      }
      args.options.initial_tab = *tab;
    } else if (arg == "-s" || arg == "--snapshot") {
      args.snapshot = true;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        if (!ParseSize(argv[++i], args.snapshot_width, args.snapshot_height)) {
          std::cerr << "wtop: --snapshot expects a size such as 120x40\n";
          return 2;
        }
      }
    } else {
      std::cerr << "wtop: unknown option '" << arg << "'\n\n" << kUsage;
      return 2;
    }
  }
  return std::nullopt;
}

}  // namespace

int main(int argc, char** argv) {
  ParsedArgs args;
  if (const auto exit_code = ParseArgs(argc, argv, args)) return *exit_code;

  wtop::App app(args.options);
  if (args.snapshot) {
    int width = args.snapshot_width;
    int height = args.snapshot_height;
    if (width == 0) {
      const wtop::TerminalSize size = wtop::QueryTerminalSize();
      width = size.columns > 0 ? size.columns : 120;
      height = size.rows > 0 ? size.rows : 40;
    }
    return app.PrintSnapshot(width, height);
  }
  return app.Run();
}
