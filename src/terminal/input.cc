#include "terminal/input.h"

#include <poll.h>
#include <unistd.h>

#include <string>

#include "core/file_util.h"

namespace wtop {
namespace {

// Maps the final byte of "ESC [ <final>" / "ESC O <final>" sequences.
bool MapFinalByte(char final_byte, Key& key) {
  switch (final_byte) {
    case 'A':
      key.code = KeyCode::kUp;
      return true;
    case 'B':
      key.code = KeyCode::kDown;
      return true;
    case 'C':
      key.code = KeyCode::kRight;
      return true;
    case 'D':
      key.code = KeyCode::kLeft;
      return true;
    case 'H':
      key.code = KeyCode::kHome;
      return true;
    case 'F':
      key.code = KeyCode::kEnd;
      return true;
    case 'Z':
      key.code = KeyCode::kBackTab;
      return true;
    // xterm sends F1-F4 as "ESC O P".."ESC O S".
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
      key.code = FunctionKey(final_byte - 'P' + 1);
      return true;
    default:
      return false;
  }
}

// Maps "ESC [ <number> ~" sequences.
bool MapTildeNumber(std::string_view number, Key& key) {
  const auto value = ParseNumber<int>(number);
  if (!value) return false;
  switch (*value) {
    case 1:
    case 7:
      key.code = KeyCode::kHome;
      return true;
    case 4:
    case 8:
      key.code = KeyCode::kEnd;
      return true;
    case 5:
      key.code = KeyCode::kPageUp;
      return true;
    case 6:
      key.code = KeyCode::kPageDown;
      return true;
    default:
      break;
  }
  // Function keys; the numbering skips 16 and 22 for historical reasons.
  if (*value >= 11 && *value <= 15) {
    key.code = FunctionKey(*value - 10);
  } else if (*value >= 17 && *value <= 21) {
    key.code = FunctionKey(*value - 11);
  } else if (*value == 23 || *value == 24) {
    key.code = FunctionKey(*value - 12);
  } else {
    return false;
  }
  return true;
}

bool PollStdin(int timeout_ms) {
  pollfd fd{STDIN_FILENO, POLLIN, 0};
  return ::poll(&fd, 1, timeout_ms) > 0 && (fd.revents & POLLIN);
}

}  // namespace

std::vector<Key> DecodeKeys(std::string_view bytes) {
  std::vector<Key> keys;
  std::size_t i = 0;
  while (i < bytes.size()) {
    const char c = bytes[i];
    Key key;
    if (c == '\x1b') {
      if (i + 1 >= bytes.size()) {
        key.code = KeyCode::kEscape;
        keys.push_back(key);
        ++i;
        continue;
      }
      const char kind = bytes[i + 1];
      if (kind != '[' && kind != 'O') {
        // Alt+<key> or a stray ESC: report Escape, then the key itself.
        key.code = KeyCode::kEscape;
        keys.push_back(key);
        ++i;
        continue;
      }
      // The Linux console sends F1-F5 as "ESC [ [ A".."ESC [ [ E".
      if (kind == '[' && i + 3 < bytes.size() && bytes[i + 2] == '[') {
        const char letter = bytes[i + 3];
        if (letter >= 'A' && letter <= 'E') {
          key.code = FunctionKey(letter - 'A' + 1);
          keys.push_back(key);
        }
        i += 4;
        continue;
      }
      // Parameters run until a final byte in the range 0x40..0x7e.
      std::size_t j = i + 2;
      while (j < bytes.size() && !(bytes[j] >= 0x40 && bytes[j] <= 0x7e)) {
        ++j;
      }
      if (j >= bytes.size()) break;  // Incomplete sequence; drop it.
      const std::string_view params = bytes.substr(i + 2, j - i - 2);
      const char final_byte = bytes[j];
      bool known = false;
      if (final_byte == '~') {
        known = MapTildeNumber(params.substr(0, params.find(';')), key);
      } else {
        known = MapFinalByte(final_byte, key);
      }
      if (known) keys.push_back(key);
      i = j + 1;
      continue;
    }
    if (c == '\r' || c == '\n') {
      key.code = KeyCode::kEnter;
    } else if (c == '\t') {
      key.code = KeyCode::kTab;
    } else if (c == 0x7f || c == 0x08) {
      key.code = KeyCode::kBackspace;
    } else {
      key.ch = c;
    }
    keys.push_back(key);
    ++i;
  }
  return keys;
}

std::vector<Key> ReadKeys(std::chrono::milliseconds timeout) {
  if (!PollStdin(static_cast<int>(timeout.count()))) return {};
  std::string bytes;
  char buffer[256];
  while (true) {
    const ssize_t n = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (n <= 0) break;
    bytes.append(buffer, static_cast<std::size_t>(n));
    // A lone ESC may be the start of a sequence split across reads; give
    // the rest a moment to arrive before treating it as the Escape key.
    if (!PollStdin(bytes.back() == '\x1b' ? 30 : 0)) break;
  }
  return DecodeKeys(bytes);
}

}  // namespace wtop
