#include "ui/tab_bar.h"

#include <format>

#include "ui/theme.h"

namespace wtop {
namespace {

std::string TabLabel(const std::vector<TabInfo>& tabs, std::size_t i) {
  return std::format(" {}:{} ", i, tabs[i].name);
}

}  // namespace

void RenderTabBar(ScreenBuffer& buffer, int y, const std::vector<TabInfo>& tabs,
                  int active) {
  const int width = buffer.width();
  buffer.Fill(Rect{0, y, width, 1}, U' ',
              Style{theme::kText, theme::kPanel, kNormal});
  if (tabs.empty()) return;

  std::vector<int> widths;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    widths.push_back(DisplayWidth(TabLabel(tabs, i)) + 1);  // +1 separator.
  }

  // Scroll so the active tab fits, reserving room for the "‹"/"›" markers.
  const int available = width - 2;
  std::size_t first = 0;
  const auto span = [&](std::size_t from) {
    int total = 0;
    for (std::size_t i = from; i <= static_cast<std::size_t>(active); ++i) {
      total += widths[i];
    }
    return total;
  };
  while (first < static_cast<std::size_t>(active) && span(first) > available) {
    ++first;
  }

  const Style marker{theme::kLabel, theme::kPanel, kBold};
  int x = 0;
  if (first > 0) buffer.Set(x, y, U'‹', marker);
  x = 1;
  std::size_t i = first;
  for (; i < tabs.size(); ++i) {
    if (x + widths[i] > width - 1) break;
    const bool is_active = static_cast<int>(i) == active;
    const Style style =
        is_active ? Style{theme::kBlack, tabs[i].color, kBold}
                  : Style{theme::kTabInactiveText, theme::kTabInactive, 0};
    x += buffer.DrawText(x, y, TabLabel(tabs, i), style);
    ++x;  // Separator keeps the panel background.
  }
  if (i < tabs.size()) buffer.Set(width - 1, y, U'›', marker);
}

}  // namespace wtop
