#include "ui/widgets.h"

#include <algorithm>
#include <cmath>

#include "ui/theme.h"

namespace wtop {
namespace {

constexpr char32_t kBrailleBase = 0x2800;

// Bit of a braille dot at column `dx` (0-1) and row `dy` (0-3, top first).
constexpr std::uint8_t BrailleBit(int dx, int dy) {
  constexpr std::uint8_t kBits[2][4] = {{0x01, 0x02, 0x04, 0x40},
                                        {0x08, 0x10, 0x20, 0x80}};
  return kBits[dx][dy];
}

// Height, in braille dots, of each dot column of the graph, or -1 where the
// history does not reach back that far.
std::vector<int> SampleColumns(const History& history, int dot_columns,
                               int dot_rows, double max_value) {
  std::vector<int> heights(static_cast<std::size_t>(dot_columns), -1);
  const std::size_t size = history.size();
  if (size == 0 || dot_columns <= 0 || max_value <= 0.0) return heights;
  const double span = static_cast<double>(History::capacity() - 1);
  for (int px = 0; px < dot_columns; ++px) {
    // Age in samples of this column; the right edge is age 0.
    const double age = dot_columns == 1
                           ? 0.0
                           : (dot_columns - 1 - px) * span / (dot_columns - 1);
    if (age > static_cast<double>(size - 1)) continue;
    const double newest_index = static_cast<double>(size - 1) - age;
    const auto lo = static_cast<std::size_t>(std::floor(newest_index));
    const std::size_t hi = std::min(lo + 1, size - 1);
    const double t = newest_index - static_cast<double>(lo);
    const double value = history[lo] * (1.0 - t) + history[hi] * t;
    const double ratio = std::clamp(value / max_value, 0.0, 1.0);
    int dots = static_cast<int>(std::lround(ratio * dot_rows));
    if (dots == 0 && value > 0.0) dots = 1;  // Keep small activity visible.
    heights[static_cast<std::size_t>(px)] = dots;
  }
  return heights;
}

// Positions of `divisions - 1` interior grid lines across `length` cells.
std::vector<int> GridPositions(int length, int divisions) {
  std::vector<int> positions;
  for (int k = 1; k < divisions; ++k) {
    positions.push_back(static_cast<int>(std::lround(
                            static_cast<double>(length) * k / divisions)) -
                        1);
  }
  return positions;
}

}  // namespace

void DrawBox(ScreenBuffer& buffer, const Rect& rect, const Style& style) {
  if (rect.width < 2 || rect.height < 2) return;
  const int right = rect.right() - 1;
  const int bottom = rect.bottom() - 1;
  for (int x = rect.x + 1; x < right; ++x) {
    buffer.Set(x, rect.y, U'─', style);
    buffer.Set(x, bottom, U'─', style);
  }
  for (int y = rect.y + 1; y < bottom; ++y) {
    buffer.Set(rect.x, y, U'│', style);
    buffer.Set(right, y, U'│', style);
  }
  buffer.Set(rect.x, rect.y, U'┌', style);
  buffer.Set(right, rect.y, U'┐', style);
  buffer.Set(rect.x, bottom, U'└', style);
  buffer.Set(right, bottom, U'┘', style);
}

void DrawTextRight(ScreenBuffer& buffer, int right, int y,
                   std::string_view text, const Style& style) {
  buffer.DrawText(right - DisplayWidth(text), y, text, style);
}

std::string Truncate(std::string_view text, int width) {
  if (width <= 0) return {};
  const std::u32string cps = DecodeUtf8(text);
  if (static_cast<int>(cps.size()) <= width) return std::string(text);
  std::string out;
  for (int i = 0; i < width - 1; ++i) AppendUtf8(cps[i], out);
  out += "…";
  return out;
}

void DrawGraph(ScreenBuffer& buffer, const Rect& rect,
               const std::vector<GraphSeries>& series,
               const GraphOptions& options) {
  if (rect.empty()) return;
  const int dot_columns = rect.width * 2;
  const int dot_rows = rect.height * 4;
  const std::size_t cell_count =
      static_cast<std::size_t>(rect.width) * rect.height;

  // Per cell: combined dot pattern and the series that owns its color.
  std::vector<std::uint8_t> bits(cell_count, 0);
  std::vector<int> owner(cell_count, -1);
  std::vector<bool> owner_full(cell_count, false);

  for (std::size_t s = 0; s < series.size(); ++s) {
    if (series[s].data == nullptr) continue;
    const std::vector<int> heights = SampleColumns(*series[s].data, dot_columns,
                                                   dot_rows, options.max_value);
    std::vector<std::uint8_t> own(cell_count, 0);
    for (int px = 0; px < dot_columns; ++px) {
      const int h = heights[static_cast<std::size_t>(px)];
      if (h < 0) continue;
      int from = 0;
      int to = h;  // Dot rows [from, to) counted from the bottom.
      if (!series[s].filled) {
        if (h == 0) continue;
        // Join the top dot to the previous column's top dot so steep lines
        // stay continuous.
        const int prev =
            px > 0 ? heights[static_cast<std::size_t>(px - 1)] : -1;
        const int prev_top = prev > 0 ? prev - 1 : h - 1;
        from = std::min(prev_top, h - 1);
        to = std::max(prev_top, h - 1) + 1;
      }
      for (int dy = from; dy < to; ++dy) {
        const int row = rect.height - 1 - dy / 4;
        const int col = px / 2;
        own[static_cast<std::size_t>(row) * rect.width + col] |=
            BrailleBit(px % 2, 3 - dy % 4);
      }
    }
    for (std::size_t i = 0; i < cell_count; ++i) {
      if (own[i] == 0) continue;
      bits[i] |= own[i];
      owner[i] = static_cast<int>(s);
      owner_full[i] = series[s].filled && own[i] == 0xFF;
    }
  }

  std::vector<int> grid_rows;
  std::vector<int> grid_cols;
  if (options.grid) {
    // Roughly one grid line every 4 rows and 8 columns, like Task Manager's
    // 10 x 12 grid on a large window.
    const int row_divisions = std::clamp(rect.height / 4, 1, 10);
    const int col_divisions = std::clamp(rect.width / 8, 1, 12);
    grid_rows = GridPositions(rect.height, row_divisions);
    grid_cols = GridPositions(rect.width, col_divisions);
  }
  const Style grid_style{theme::kGrid, Color::Default(), kNormal};

  for (int y = 0; y < rect.height; ++y) {
    const bool grid_row =
        std::find(grid_rows.begin(), grid_rows.end(), y) != grid_rows.end();
    for (int x = 0; x < rect.width; ++x) {
      const std::size_t i = static_cast<std::size_t>(y) * rect.width + x;
      if (bits[i] != 0) {
        Color color = series[static_cast<std::size_t>(owner[i])].color;
        // Solid interior cells get a darker shade so the top edge reads as
        // the line of the chart, like Task Manager's filled area.
        if (owner_full[i]) color = color.Mix(theme::kBlack, 0.45);
        buffer.Set(rect.x + x, rect.y + y, kBrailleBase + bits[i],
                   Style{color, Color::Default(), kNormal});
        continue;
      }
      const bool grid_col =
          std::find(grid_cols.begin(), grid_cols.end(), x) != grid_cols.end();
      char32_t ch = U' ';
      if (grid_row && grid_col) {
        ch = U'┼';
      } else if (grid_row) {
        ch = U'─';
      } else if (grid_col) {
        ch = U'│';
      }
      buffer.Set(rect.x + x, rect.y + y, ch, grid_style);
    }
  }
}

void DrawSegmentBar(ScreenBuffer& buffer, const Rect& rect,
                    const std::vector<BarSegment>& segments) {
  if (rect.empty()) return;
  double start = 0.0;
  for (const BarSegment& segment : segments) {
    const double end = std::min(1.0, start + std::max(0.0, segment.fraction));
    const int x0 = rect.x + static_cast<int>(std::lround(start * rect.width));
    const int x1 = rect.x + static_cast<int>(std::lround(end * rect.width));
    for (int x = x0; x < x1; ++x) {
      for (int y = rect.y; y < rect.bottom(); ++y) {
        buffer.Set(x, y, U'█', Style{segment.color, Color::Default(), 0});
      }
    }
    // Divider after each non-empty segment.
    if (x1 > x0 && x1 < rect.right()) {
      for (int y = rect.y; y < rect.bottom(); ++y) {
        buffer.Set(x1, y, U'▏', Style{theme::kBorder, Color::Default(), 0});
      }
    }
    start = end;
  }
}

}  // namespace wtop
