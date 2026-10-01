#ifndef WTOP_SRC_TERMINAL_INPUT_H_
#define WTOP_SRC_TERMINAL_INPUT_H_

#include <chrono>
#include <string_view>
#include <vector>

namespace wtop {

enum class KeyCode {
  kChar,  // Printable character or control byte, see Key::ch.
  kEnter,
  kEscape,
  kTab,
  kBackTab,
  kBackspace,
  kUp,
  kDown,
  kLeft,
  kRight,
  kHome,
  kEnd,
  kPageUp,
  kPageDown,
  // Function keys, kept contiguous so FunctionKey() can index them.
  kF1,
  kF2,
  kF3,
  kF4,
  kF5,
  kF6,
  kF7,
  kF8,
  kF9,
  kF10,
  kF11,
  kF12,
};

// KeyCode of function key F<number>, for number in 1..12.
constexpr KeyCode FunctionKey(int number) {
  return static_cast<KeyCode>(static_cast<int>(KeyCode::kF1) + number - 1);
}

struct Key {
  KeyCode code = KeyCode::kChar;
  char ch = 0;  // For kChar. Ctrl+<letter> arrives as 1..26.

  bool IsChar(char c) const { return code == KeyCode::kChar && ch == c; }
};

inline constexpr char kCtrlB = 0x02;
inline constexpr char kCtrlC = 0x03;

// Decodes raw terminal input bytes into keys. A trailing lone ESC byte is
// treated as the Escape key.
std::vector<Key> DecodeKeys(std::string_view bytes);

// Waits up to `timeout` for stdin to become readable and returns the keys
// read. Returns an empty list on timeout or when interrupted by a signal.
std::vector<Key> ReadKeys(std::chrono::milliseconds timeout);

}  // namespace wtop

#endif  // WTOP_SRC_TERMINAL_INPUT_H_
