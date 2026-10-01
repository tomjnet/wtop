#ifndef WTOP_SRC_UI_TAB_BAR_H_
#define WTOP_SRC_UI_TAB_BAR_H_

#include <string>
#include <vector>

#include "terminal/screen_buffer.h"

namespace wtop {

struct TabInfo {
  std::string name;
  Color color;
};

// Draws an editor-style tab line ("0:CPU  1:Memory  2:Disk 0 (sda) ...") on
// row `y`. The active tab is highlighted in its resource color and the line
// scrolls horizontally to keep it visible.
void RenderTabBar(ScreenBuffer& buffer, int y, const std::vector<TabInfo>& tabs,
                  int active);

}  // namespace wtop

#endif  // WTOP_SRC_UI_TAB_BAR_H_
