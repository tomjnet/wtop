#include "terminal/screen_buffer.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace wtop {
namespace {

void AppendSgr(const Style& style, std::string& out) {
  out += "\x1b[0";
  if (style.attributes & kBold) out += ";1";
  if (style.attributes & kDim) out += ";2";
  if (style.attributes & kItalic) out += ";3";
  if (style.attributes & kUnderline) out += ";4";
  if (style.attributes & kReverse) out += ";7";
  if (!style.fg.is_default) {
    out += std::format(";38;2;{};{};{}", style.fg.r, style.fg.g, style.fg.b);
  }
  if (!style.bg.is_default) {
    out += std::format(";48;2;{};{};{}", style.bg.r, style.bg.g, style.bg.b);
  }
  out += 'm';
}

}  // namespace

Color Color::Mix(const Color& other, double t) const {
  if (is_default || other.is_default) return t < 0.5 ? *this : other;
  const auto lerp = [t](std::uint8_t a, std::uint8_t b) {
    return static_cast<std::uint8_t>(std::lround(a + (b - a) * t));
  };
  return Rgb(lerp(r, other.r), lerp(g, other.g), lerp(b, other.b));
}

std::u32string DecodeUtf8(std::string_view text) {
  std::u32string out;
  out.reserve(text.size());
  std::size_t i = 0;
  while (i < text.size()) {
    const auto byte = static_cast<unsigned char>(text[i]);
    int length = 0;
    char32_t cp = 0;
    if (byte < 0x80) {
      length = 1;
      cp = byte;
    } else if ((byte & 0xE0) == 0xC0) {
      length = 2;
      cp = byte & 0x1F;
    } else if ((byte & 0xF0) == 0xE0) {
      length = 3;
      cp = byte & 0x0F;
    } else if ((byte & 0xF8) == 0xF0) {
      length = 4;
      cp = byte & 0x07;
    }
    bool valid = length > 0 && i + length <= text.size();
    for (int k = 1; valid && k < length; ++k) {
      const auto next = static_cast<unsigned char>(text[i + k]);
      if ((next & 0xC0) != 0x80) {
        valid = false;
      } else {
        cp = (cp << 6) | (next & 0x3F);
      }
    }
    if (!valid) {
      out.push_back(U'�');
      ++i;
      continue;
    }
    out.push_back(cp);
    i += static_cast<std::size_t>(length);
  }
  return out;
}

void AppendUtf8(char32_t cp, std::string& out) {
  if (cp < 0x80) {
    out.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp < 0x10000) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

int DisplayWidth(std::string_view text) {
  return static_cast<int>(DecodeUtf8(text).size());
}

ScreenBuffer::ScreenBuffer(int width, int height) { Resize(width, height); }

void ScreenBuffer::Resize(int width, int height) {
  width_ = std::max(0, width);
  height_ = std::max(0, height);
  cells_.assign(static_cast<std::size_t>(width_) * height_, Cell{});
}

void ScreenBuffer::Clear(const Style& style) {
  std::fill(cells_.begin(), cells_.end(), Cell{U' ', style});
}

void ScreenBuffer::Set(int x, int y, char32_t ch, const Style& style) {
  if (!Contains(x, y)) return;
  cells_[Index(x, y)] = Cell{ch, style};
}

int ScreenBuffer::DrawText(int x, int y, std::string_view text,
                           const Style& style, int max_width) {
  if (y < 0 || y >= height_) return 0;
  max_width = std::min(max_width, width_ - x);
  int written = 0;
  for (char32_t cp : DecodeUtf8(text)) {
    if (written >= max_width) break;
    if (cp == U'\t') cp = U' ';
    if (cp < 0x20) continue;
    Set(x + written, y, cp, style);
    ++written;
  }
  return written;
}

void ScreenBuffer::Fill(const Rect& rect, char32_t ch, const Style& style) {
  for (int y = rect.y; y < rect.bottom(); ++y) {
    for (int x = rect.x; x < rect.right(); ++x) Set(x, y, ch, style);
  }
}

void ScreenBuffer::Tint(const Rect& rect, const Color& bg) {
  for (int y = std::max(0, rect.y); y < std::min(height_, rect.bottom()); ++y) {
    for (int x = std::max(0, rect.x); x < std::min(width_, rect.right()); ++x) {
      cells_[Index(x, y)].style.bg = bg;
    }
  }
}

std::string ScreenBuffer::Render(const ScreenBuffer& previous,
                                 bool full) const {
  full = full || previous.width_ != width_ || previous.height_ != height_;
  std::string out;
  out.reserve(full ? cells_.size() * 4 : 1024);
  if (full) out += "\x1b[0m\x1b[2J";

  bool style_known = false;
  Style current;
  int cursor_x = -1;
  int cursor_y = -1;
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const Cell& cell = cells_[Index(x, y)];
      if (!full && previous.cells_[Index(x, y)] == cell) continue;
      if (y != cursor_y || x != cursor_x) {
        out += std::format("\x1b[{};{}H", y + 1, x + 1);
      }
      if (!style_known || cell.style != current) {
        AppendSgr(cell.style, out);
        current = cell.style;
        style_known = true;
      }
      AppendUtf8(cell.ch, out);
      cursor_x = x + 1;
      cursor_y = y;
    }
  }
  if (!out.empty()) out += "\x1b[0m";
  return out;
}

std::string ScreenBuffer::ToPlainText() const {
  std::string out;
  for (int y = 0; y < height_; ++y) {
    std::string line;
    for (int x = 0; x < width_; ++x) AppendUtf8(At(x, y).ch, line);
    const auto end = line.find_last_not_of(' ');
    line.erase(end == std::string::npos ? 0 : end + 1);
    out += line;
    out += '\n';
  }
  return out;
}

}  // namespace wtop
