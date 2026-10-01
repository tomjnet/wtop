#include "ui/status_line.h"

#include <ctime>
#include <format>

#include "ui/theme.h"
#include "ui/widgets.h"

namespace wtop {
namespace {

std::string ClockText() {
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  ::localtime_r(&now, &local);
  char text[32];
  std::strftime(text, sizeof(text), "%H:%M %d-%b-%y", &local);
  return text;
}

}  // namespace

void RenderStatusLine(ScreenBuffer& buffer, int y, const StatusInfo& info) {
  const int width = buffer.width();
  const Style base{theme::kBlack, theme::kStatus, kNormal};
  buffer.Fill(Rect{0, y, width, 1}, U' ', base);

  int x = buffer.DrawText(0, y, "[wtop] ", base);
  if (info.prefix_pending) {
    x += buffer.DrawText(x, y, " ^B ",
                         Style{theme::kBlack, theme::kPrefix, kBold});
    x += 1;
  }
  x += buffer.DrawText(x, y, info.active_tab + "*",
                       Style{theme::kBlack, theme::kStatus, kBold});

  const std::string right =
      std::format("\"{}\" {} ", info.hostname, ClockText());
  const int right_x = width - DisplayWidth(right);
  const std::string hints =
      info.prefix_pending
          ? "  n:next p:prev l:last 0-9:select w:list ?:help d:quit"
          : "  C-b n/p:switch  C-b w:list  C-b ?:help  q:quit";
  buffer.DrawText(x, y, Truncate(hints, right_x - x - 1), base);
  if (right_x > x) buffer.DrawText(right_x, y, right, base);
}

}  // namespace wtop
