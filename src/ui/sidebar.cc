#include "ui/sidebar.h"

#include <algorithm>

#include "ui/theme.h"
#include "ui/widgets.h"

namespace wtop {
namespace {

constexpr int kEntryHeight = 4;  // Three content rows and a spacer.
constexpr int kGraphWidth = 10;

}  // namespace

void RenderSidebar(ScreenBuffer& buffer, const Rect& area,
                   const std::vector<SidebarEntry>& entries, int selected) {
  const int visible = std::max(1, area.height / kEntryHeight);
  const int count = static_cast<int>(entries.size());
  const int first =
      std::clamp(selected - visible + 1, 0, std::max(0, count - visible));

  for (int i = first; i < count && i - first < visible; ++i) {
    const SidebarEntry& entry = entries[static_cast<std::size_t>(i)];
    const int y = area.y + (i - first) * kEntryHeight;
    const bool is_selected = i == selected;
    const Rect row{area.x, y, area.width, kEntryHeight - 1};
    const Color background = is_selected ? theme::kSelected : Color::Default();
    buffer.Fill(row, U' ', Style{theme::kText, background, kNormal});

    if (is_selected) {
      for (int dy = 0; dy < row.height; ++dy) {
        buffer.Set(area.x, y + dy, U'▌',
                   Style{entry.color, background, kNormal});
      }
    }
    const Rect graph{area.x + 2, y, kGraphWidth, row.height};
    buffer.Fill(graph, U' ', Style{theme::kText, theme::kPanel, kNormal});
    DrawGraph(buffer, graph, {GraphSeries{entry.history, entry.color, true}},
              GraphOptions{.max_value = entry.max_value, .grid = false});
    buffer.Tint(graph, theme::kPanel);

    const int text_x = graph.right() + 2;
    const int text_width = area.right() - text_x - 1;
    buffer.DrawText(
        text_x, y, Truncate(entry.title, text_width),
        Style{is_selected ? entry.color : theme::kText, background, kBold});
    buffer.DrawText(text_x, y + 1, Truncate(entry.line1, text_width),
                    Style{theme::kLabel, background, kNormal});
    buffer.DrawText(text_x, y + 2, Truncate(entry.line2, text_width),
                    Style{theme::kLabel, background, kNormal});
  }

  // Hint that more entries exist above or below.
  const Style arrow{theme::kLabel, Color::Default(), kNormal};
  if (first > 0) buffer.Set(area.right() - 2, area.y, U'▲', arrow);
  if (first + visible < count) {
    buffer.Set(area.right() - 2, area.bottom() - 1, U'▼', arrow);
  }
}

}  // namespace wtop
