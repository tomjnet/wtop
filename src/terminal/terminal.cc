#include "terminal/terminal.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <csignal>

namespace wtop {
namespace {

std::atomic<bool> g_resized{false};
std::atomic<bool> g_quit{false};

static_assert(std::atomic<bool>::is_always_lock_free,
              "signal handlers need lock-free atomics");

extern "C" void HandleResize(int) { g_resized.store(true); }
extern "C" void HandleQuit(int) { g_quit.store(true); }

void InstallHandler(int signal, void (*handler)(int)) {
  struct sigaction action{};
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  // No SA_RESTART: poll() must return EINTR so the UI reacts immediately.
  action.sa_flags = 0;
  ::sigaction(signal, &action, nullptr);
}

// Alternate screen, hidden cursor and no automatic line wrap.
constexpr std::string_view kEnterSequence = "\x1b[?1049h\x1b[?25l\x1b[?7l";
constexpr std::string_view kLeaveSequence =
    "\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l";

}  // namespace

TerminalSize QueryTerminalSize() {
  winsize ws{};
  if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 &&
      ws.ws_row > 0) {
    return {ws.ws_col, ws.ws_row};
  }
  return {0, 0};
}

Terminal::Terminal() {
  if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) return;
  if (::tcgetattr(STDIN_FILENO, &original_) != 0) return;

  termios raw = original_;
  // Byte-at-a-time input without echo; Ctrl+C/Ctrl+Z arrive as keys so the
  // screen is always restored through the normal exit path.
  raw.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cflag |= CS8;
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  if (::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) return;

  InstallHandler(SIGWINCH, HandleResize);
  InstallHandler(SIGTERM, HandleQuit);
  InstallHandler(SIGHUP, HandleQuit);
  InstallHandler(SIGINT, HandleQuit);

  ok_ = true;
  Write(kEnterSequence);
}

Terminal::~Terminal() { Restore(); }

void Terminal::Restore() {
  if (!ok_) return;
  Write(kLeaveSequence);
  ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
  ok_ = false;
}

TerminalSize Terminal::Size() const {
  TerminalSize size = QueryTerminalSize();
  if (size.columns == 0) return TerminalSize{};
  return size;
}

void Terminal::Write(std::string_view data) const {
  while (!data.empty()) {
    const ssize_t n = ::write(STDOUT_FILENO, data.data(), data.size());
    if (n < 0) {
      if (errno == EINTR) continue;
      return;
    }
    data.remove_prefix(static_cast<std::size_t>(n));
  }
}

bool Terminal::ConsumeResize() { return g_resized.exchange(false); }

bool Terminal::QuitRequested() { return g_quit.load(); }

}  // namespace wtop
