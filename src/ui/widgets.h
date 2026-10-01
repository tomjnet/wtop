#ifndef WTOP_SRC_UI_WIDGETS_H_
#define WTOP_SRC_UI_WIDGETS_H_

#include <string>
#include <string_view>
#include <vector>

#include "core/ring_buffer.h"
#include "terminal/screen_buffer.h"

namespace wtop {

// Draws a single-line box around `rect` (the border occupies its edges).
void DrawBox(ScreenBuffer& buffer, const Rect& rect, const Style& style);

// Draws text so that it ends just before column `right`.
void DrawTextRight(ScreenBuffer& buffer, int right, int y,
                   std::string_view text, const Style& style);

// Shortens `text` to `width` columns, ending in "…" when cut.
std::string Truncate(std::string_view text, int width);

struct GraphSeries {
  const History* data = nullptr;
  Color color;
  // Filled series are drawn as an area; the others as a line on top.
  bool filled = true;
};

struct GraphOptions {
  double max_value = 100.0;
  bool grid = true;
};

// Draws a braille chart of the given histories inside `rect` (no border).
// The newest sample sits at the right edge and the full width spans the
// history capacity, so the graph scrolls like Task Manager's.
void DrawGraph(ScreenBuffer& buffer, const Rect& rect,
               const std::vector<GraphSeries>& series,
               const GraphOptions& options);

struct BarSegment {
  double fraction = 0.0;  // Of the whole bar, 0..1.
  Color color;
};

// Draws adjacent colored segments filling `rect` from the left, separated
// by thin divider columns like Task Manager's memory composition bar.
void DrawSegmentBar(ScreenBuffer& buffer, const Rect& rect,
                    const std::vector<BarSegment>& segments);

}  // namespace wtop

#endif  // WTOP_SRC_UI_WIDGETS_H_
