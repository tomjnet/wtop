#ifndef WTOP_SRC_TERMINAL_SCREEN_BUFFER_H_
#define WTOP_SRC_TERMINAL_SCREEN_BUFFER_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wtop {

// A 24-bit color, or the terminal's default color.
struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  bool is_default = true;

  static constexpr Color Rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return Color{r, g, b, false};
  }
  static constexpr Color Default() { return Color{}; }

  // Linear blend towards `other`; t = 0 keeps this color, t = 1 is `other`.
  Color Mix(const Color& other, double t) const;

  bool operator==(const Color&) const = default;
};

enum Attribute : std::uint8_t {
  kNormal = 0,
  kBold = 1 << 0,
  kDim = 1 << 1,
  kItalic = 1 << 2,
  kUnderline = 1 << 3,
  kReverse = 1 << 4,
};

struct Style {
  Color fg;
  Color bg;
  std::uint8_t attributes = kNormal;

  bool operator==(const Style&) const = default;
};

struct Cell {
  char32_t ch = U' ';
  Style style;

  bool operator==(const Cell&) const = default;
};

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  int right() const { return x + width; }
  int bottom() const { return y + height; }
  bool empty() const { return width <= 0 || height <= 0; }
};

// Decodes UTF-8 into code points. Invalid bytes become U+FFFD.
std::u32string DecodeUtf8(std::string_view text);
void AppendUtf8(char32_t code_point, std::string& out);

// Number of terminal columns `text` occupies (every code point counts as
// one column, which holds for the glyphs wtop draws).
int DisplayWidth(std::string_view text);

// An off-screen grid of styled cells. Frames are drawn into a buffer and
// then converted to the minimal escape sequence stream that turns the
// previous frame into this one.
class ScreenBuffer {
 public:
  ScreenBuffer() = default;
  ScreenBuffer(int width, int height);

  void Resize(int width, int height);
  void Clear(const Style& style = {});

  int width() const { return width_; }
  int height() const { return height_; }

  bool Contains(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
  }
  const Cell& At(int x, int y) const { return cells_[Index(x, y)]; }

  // Writes one cell; out-of-bounds writes are ignored.
  void Set(int x, int y, char32_t ch, const Style& style);

  // Draws UTF-8 text on one row, clipped to `max_width` columns and the
  // buffer. Returns the number of columns written.
  int DrawText(int x, int y, std::string_view text, const Style& style,
               int max_width = 1 << 30);

  // Fills a rectangle with a character.
  void Fill(const Rect& rect, char32_t ch, const Style& style);

  // Changes the background of every cell in `rect`, keeping glyphs.
  void Tint(const Rect& rect, const Color& bg);

  // Escape sequences turning `previous` into this frame. When the sizes
  // differ (or `full` is set) the whole screen is repainted.
  std::string Render(const ScreenBuffer& previous, bool full) const;

  // The frame as plain text lines, without any styling.
  std::string ToPlainText() const;

 private:
  std::size_t Index(int x, int y) const {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
  }

  int width_ = 0;
  int height_ = 0;
  std::vector<Cell> cells_;
};

}  // namespace wtop

#endif  // WTOP_SRC_TERMINAL_SCREEN_BUFFER_H_
