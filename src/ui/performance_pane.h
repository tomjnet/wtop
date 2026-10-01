#ifndef WTOP_SRC_UI_PERFORMANCE_PANE_H_
#define WTOP_SRC_UI_PERFORMANCE_PANE_H_

#include <string>
#include <vector>

#include "terminal/screen_buffer.h"
#include "ui/widgets.h"

namespace wtop {

// A large "label over value" statistic, e.g. "Utilization" / "6%".
struct BigStat {
  std::string label;
  std::string value;
  // When set, a colored rule precedes the stat (like Task Manager's read /
  // write legend). Dashed rules mark line series.
  Color accent = Color::Default();
  bool dashed = false;
};

// A "key: value" row of the details table.
struct Detail {
  std::string key;
  std::string value;
};

struct GraphPanel {
  std::string label_left;   // e.g. "% Utilization".
  std::string label_right;  // e.g. "100%".
  std::vector<GraphSeries> series;
  GraphOptions options;
  int weight = 3;  // Share of the vertical space among graphs.
};

// Everything a resource screen shows, in Task Manager's layout: title,
// graphs, an optional composition bar and the statistics block.
struct PaneContent {
  std::string title;
  std::string subtitle;  // Right aligned, e.g. the CPU model.
  Color color;
  std::vector<GraphPanel> graphs;
  std::string bar_label;
  std::vector<BarSegment> bar;
  std::vector<std::vector<BigStat>> stats;  // Rows of big statistics.
  std::vector<Detail> details;
};

void RenderPerformancePane(ScreenBuffer& buffer, const Rect& area,
                           const PaneContent& content);

}  // namespace wtop

#endif  // WTOP_SRC_UI_PERFORMANCE_PANE_H_
