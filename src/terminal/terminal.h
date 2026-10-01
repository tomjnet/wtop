#ifndef WTOP_SRC_TERMINAL_TERMINAL_H_
#define WTOP_SRC_TERMINAL_TERMINAL_H_

#include <termios.h>

#include <string_view>

namespace wtop {

struct TerminalSize {
  int columns = 80;
  int rows = 24;
};

// RAII guard that switches the controlling terminal into a full-screen,
// raw-input session (alternate screen, hidden cursor, no echo) and restores
// it on destruction. Only one instance may exist at a time.
class Terminal {
 public:
  Terminal();
  ~Terminal();

  Terminal(const Terminal&) = delete;
  Terminal& operator=(const Terminal&) = delete;

  // Whether stdin/stdout are a terminal that could be configured.
  bool ok() const { return ok_; }

  TerminalSize Size() const;

  // Writes all bytes to stdout, retrying on partial writes.
  void Write(std::string_view data) const;

  // Signal driven flags. ConsumeResize() clears the flag it reports.
  static bool ConsumeResize();
  static bool QuitRequested();

 private:
  void Restore();

  bool ok_ = false;
  termios original_{};
};

// Size of the terminal on stdout, or {0, 0} when stdout is not a terminal.
TerminalSize QueryTerminalSize();

}  // namespace wtop

#endif  // WTOP_SRC_TERMINAL_TERMINAL_H_
