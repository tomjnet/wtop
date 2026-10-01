#include "ui/performance_pane.h"

#include <algorithm>
#include <numeric>

#include "ui/theme.h"

namespace wtop {
namespace {

constexpr int kStatColumnMinWidth = 12;
constexpr int kStatColumnGap = 3;
constexpr int kBlockGap = 4;  // Between the big stats and the details.

struct StatsLayout {
  std::vector<int> column_widths;
  int stats_width = 0;
  int stats_height = 0;
  int key_width = 0;
  int details_width = 0;
  bool side_by_side = true;
  int height = 0;
};

int StatWidth(const BigStat& stat) {
  const int accent = stat.accent.is_default ? 0 : 2;
  return accent + std::max(DisplayWidth(stat.label), DisplayWidth(stat.value));
}

StatsLayout ComputeStatsLayout(const PaneContent& content, int width) {
  StatsLayout layout;
  for (const auto& row : content.stats) {
    if (layout.column_widths.size() < row.size()) {
      layout.column_widths.resize(row.size(), kStatColumnMinWidth);
    }
    for (std::size_t i = 0; i < row.size(); ++i) {
      layout.column_widths[i] =
          std::max(layout.column_widths[i], StatWidth(row[i]) + kStatColumnGap);
    }
  }
  layout.stats_width = std::accumulate(layout.column_widths.begin(),
                                       layout.column_widths.end(), 0);
  // Each row is a label line and a value line, rows separated by a blank.
  const int rows = static_cast<int>(content.stats.size());
  layout.stats_height = rows == 0 ? 0 : rows * 3 - 1;

  int value_width = 0;
  for (const Detail& detail : content.details) {
    layout.key_width = std::max(layout.key_width, DisplayWidth(detail.key) + 2);
    value_width = std::max(value_width, DisplayWidth(detail.value));
  }
  layout.details_width = layout.key_width + value_width;
  const int details_height = static_cast<int>(content.details.size());

  layout.side_by_side =
      layout.stats_width + kBlockGap + layout.details_width <= width ||
      content.details.empty() || content.stats.empty();
  if (layout.side_by_side) {
    layout.height = std::max(layout.stats_height, details_height);
  } else {
    layout.height = layout.stats_height + 1 + details_height;
  }
  return layout;
}

void DrawStats(ScreenBuffer& buffer, const Rect& area,
               const PaneContent& content, const StatsLayout& layout) {
  int y = area.y;
  for (const auto& row : content.stats) {
    int x = area.x;
    for (std::size_t i = 0; i < row.size(); ++i) {
      const BigStat& stat = row[i];
      int text_x = x;
      if (!stat.accent.is_default) {
        const Style rule{stat.accent, Color::Default(), kNormal};
        const char32_t glyph = stat.dashed ? U'┆' : U'│';
        buffer.Set(x, y, glyph, rule);
        buffer.Set(x, y + 1, glyph, rule);
        text_x += 2;
      }
      const int max_width = area.right() - text_x;
      buffer.DrawText(text_x, y, stat.label, theme::kLabelStyle, max_width);
      buffer.DrawText(text_x, y + 1, stat.value, theme::kValueStyle, max_width);
      x += layout.column_widths[i];
    }
    y += 3;
  }

  int details_x = area.x;
  int details_y = area.y;
  if (layout.side_by_side && !content.stats.empty()) {
    details_x = area.x + layout.stats_width + kBlockGap - kStatColumnGap;
  } else if (!content.stats.empty()) {
    details_y = area.y + layout.stats_height + 1;
  }
  for (const Detail& detail : content.details) {
    if (details_y >= area.bottom()) break;
    buffer.DrawText(details_x, details_y, detail.key + ":", theme::kLabelStyle,
                    area.right() - details_x);
    const int value_x = details_x + layout.key_width;
    buffer.DrawText(value_x, details_y,
                    Truncate(detail.value, area.right() - value_x),
                    theme::kTextStyle);
    ++details_y;
  }
}

}  // namespace

void RenderPerformancePane(ScreenBuffer& buffer, const Rect& area,
                           const PaneContent& content) {
  if (area.width < 20 || area.height < 6) return;

  // Title row.
  buffer.DrawText(area.x, area.y, content.title,
                  Style{content.color, Color::Default(), kBold});
  const int title_width = DisplayWidth(content.title);
  const std::string subtitle =
      Truncate(content.subtitle, area.width - title_width - 2);
  DrawTextRight(buffer, area.right(), area.y, subtitle, theme::kTextStyle);

  // Vertical budget: title + gap, graphs, optional bar, then stats.
  const int header = area.height >= 28 ? 2 : 1;
  const bool has_bar = !content.bar.empty();
  const int graph_count = static_cast<int>(content.graphs.size());
  // Per graph: label row, top/bottom border and the time axis row.
  constexpr int kGraphChrome = 4;
  constexpr int kMinGraphRows = 3;

  StatsLayout stats = ComputeStatsLayout(content, area.width);
  int bar_height = has_bar ? 5 : 0;  // Label + boxed bar of two rows.
  int stats_height = stats.height > 0 ? stats.height + 1 : 0;
  const auto graph_rows = [&] {
    return area.height - header - bar_height - stats_height -
           graph_count * kGraphChrome;
  };
  // Give up the least important parts first when space is tight.
  if (graph_rows() < graph_count * kMinGraphRows && has_bar) bar_height = 0;
  if (graph_rows() < graph_count * kMinGraphRows) stats_height = 0;
  const int total_graph_rows = std::max(graph_count, graph_rows());

  int total_weight = 0;
  for (const GraphPanel& graph : content.graphs) total_weight += graph.weight;

  int y = area.y + header;
  int remaining_rows = total_graph_rows;
  for (int g = 0; g < graph_count; ++g) {
    const GraphPanel& graph = content.graphs[static_cast<std::size_t>(g)];
    int rows = g + 1 == graph_count
                   ? remaining_rows
                   : std::max(1, total_graph_rows * graph.weight /
                                     std::max(1, total_weight));
    rows = std::min(rows, remaining_rows);
    remaining_rows -= rows;

    buffer.DrawText(area.x, y, graph.label_left, theme::kLabelStyle);
    DrawTextRight(buffer, area.right(), y, graph.label_right,
                  theme::kLabelStyle);
    const Rect box{area.x, y + 1, area.width, rows + 2};
    DrawBox(buffer, box,
            Style{content.color.Mix(theme::kBlack, 0.35), Color::Default(),
                  kNormal});
    DrawGraph(buffer, Rect{box.x + 1, box.y + 1, box.width - 2, rows},
              graph.series, graph.options);
    const int axis_y = box.bottom();
    buffer.DrawText(area.x, axis_y, "60 seconds", theme::kLabelStyle);
    DrawTextRight(buffer, area.right(), axis_y, "0", theme::kLabelStyle);
    y = axis_y + 1;
  }

  if (bar_height > 0) {
    buffer.DrawText(area.x, y, content.bar_label, theme::kLabelStyle);
    const Rect box{area.x, y + 1, area.width, bar_height - 1};
    DrawBox(buffer, box,
            Style{content.color.Mix(theme::kBlack, 0.35), Color::Default(),
                  kNormal});
    DrawSegmentBar(buffer,
                   Rect{box.x + 1, box.y + 1, box.width - 2, box.height - 2},
                   content.bar);
    y += bar_height;
  }

  if (stats_height > 0) {
    DrawStats(buffer, Rect{area.x, y + 1, area.width, area.bottom() - y - 1},
              content, stats);
  }
}

}  // namespace wtop
