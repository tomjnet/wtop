#include "ui/function_bar.h"

#include "ui/theme.h"

namespace wtop {
namespace {

// htop pads every label to six columns so the keys line up.
constexpr int kLabelWidth = 6;

}  // namespace

void RenderFunctionBar(ScreenBuffer& buffer, int y,
                       const std::vector<FunctionKeyItem>& items) {
  const Style key_style{theme::kText, Color::Default(), kBold};
  const Style label_style{theme::kBlack, theme::kFunctionBar, kNormal};
  buffer.Fill(Rect{0, y, buffer.width(), 1}, U' ', label_style);
  int x = 0;
  for (const FunctionKeyItem& item : items) {
    x += buffer.DrawText(x, y, item.key, key_style);
    std::string label = item.label;
    if (static_cast<int>(label.size()) < kLabelWidth) {
      label.resize(kLabelWidth, ' ');
    }
    x += buffer.DrawText(x, y, label, label_style);
  }
}

}  // namespace wtop
