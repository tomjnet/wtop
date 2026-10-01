#include "ui/overlay.h"

#include <algorithm>
#include <array>
#include <format>
#include <utility>

#include "ui/theme.h"
#include "ui/widgets.h"

namespace wtop {
namespace {

// Clears and frames a centered box, returning its interior.
Rect DrawDialog(ScreenBuffer& buffer, int width, int height,
                const std::string& title) {
  width = std::min(width, buffer.width() - 2);
  height = std::min(height, buffer.height() - 2);
  const Rect box{(buffer.width() - width) / 2, (buffer.height() - height) / 2,
                 width, height};
  buffer.Fill(box, U' ', Style{theme::kText, theme::kPanel, kNormal});
  DrawBox(buffer, box, Style{theme::kBorder, theme::kPanel, kNormal});
  const std::string caption = " " + title + " ";
  buffer.DrawText(box.x + (box.width - DisplayWidth(caption)) / 2, box.y,
                  caption, Style{theme::kText, theme::kPanel, kBold});
  return Rect{box.x + 2, box.y + 1, box.width - 4, box.height - 2};
}

}  // namespace

void RenderChooser(ScreenBuffer& buffer, const std::vector<ChooserItem>& items,
                   int selected) {
  int width = 40;
  for (const ChooserItem& item : items) {
    width = std::max(width,
                     DisplayWidth(item.name) + DisplayWidth(item.summary) + 14);
  }
  const Rect inner = DrawDialog(
      buffer, width, static_cast<int>(items.size()) + 4, "choose tab");
  const int visible = std::max(1, inner.height - 2);
  const int count = static_cast<int>(items.size());
  const int first =
      std::clamp(selected - visible + 1, 0, std::max(0, count - visible));
  for (int i = first; i < count && i - first < visible; ++i) {
    const ChooserItem& item = items[static_cast<std::size_t>(i)];
    const int y = inner.y + 1 + (i - first);
    const bool is_selected = i == selected;
    const Color bg = is_selected ? theme::kSelected : theme::kPanel;
    buffer.Fill(Rect{inner.x, y, inner.width, 1}, U' ',
                Style{theme::kText, bg, kNormal});
    const std::string label = std::format("({}) {}", i, item.name);
    int x = inner.x + 1;
    buffer.Set(inner.x, y, is_selected ? U'▶' : U' ',
               Style{item.color, bg, kBold});
    x += buffer.DrawText(
        x, y, Truncate(label, inner.width - 2),
        Style{is_selected ? item.color : theme::kText, bg, kBold});
    const int summary_x =
        std::max(x + 2, inner.right() - DisplayWidth(item.summary));
    buffer.DrawText(summary_x, y, item.summary,
                    Style{theme::kLabel, bg, kNormal},
                    inner.right() - summary_x);
  }
  buffer.DrawText(
      inner.x, inner.bottom() - 1,
      Truncate("j/k or ↑/↓ move · Enter select · Esc close", inner.width),
      Style{theme::kLabel, theme::kPanel, kNormal});
}

void RenderHelp(ScreenBuffer& buffer) {
  constexpr std::array<std::pair<const char*, const char*>, 14> kBindings = {{
      {"F1", "Show / hide this help"},
      {"F10", "Quit wtop"},
      {"C-b n", "Next tab"},
      {"C-b p", "Previous tab"},
      {"C-b l", "Last used tab"},
      {"C-b 0..9", "Go to tab by number"},
      {"C-b w", "Choose a tab from a list"},
      {"C-b ←/→", "Previous / next tab"},
      {"C-b ?", "Show this help"},
      {"C-b d", "Quit wtop"},
      {"q", "Quit wtop"},
      {"C-c", "Quit wtop"},
      {"", ""},
      {"", "Press any key to close"},
  }};
  const Rect inner = DrawDialog(
      buffer, 46, static_cast<int>(kBindings.size()) + 4, "wtop key bindings");
  int y = inner.y + 1;
  for (const auto& [keys, action] : kBindings) {
    buffer.DrawText(inner.x + 1, y, keys,
                    Style{theme::kPrefix, theme::kPanel, kBold});
    buffer.DrawText(inner.x + 13, y, action,
                    Style{theme::kText, theme::kPanel, kNormal},
                    inner.width - 13);
    ++y;
  }
}

void RenderTooSmall(ScreenBuffer& buffer, int min_width, int min_height) {
  const std::string lines[] = {
      "Terminal too small",
      std::format("{}x{} (need {}x{})", buffer.width(), buffer.height(),
                  min_width, min_height),
  };
  int y = buffer.height() / 2 - 1;
  for (const std::string& line : lines) {
    buffer.DrawText(std::max(0, (buffer.width() - DisplayWidth(line)) / 2), y++,
                    line, theme::kValueStyle);
  }
}

}  // namespace wtop
