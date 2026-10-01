#ifndef WTOP_SRC_UI_SIDEBAR_H_
#define WTOP_SRC_UI_SIDEBAR_H_

#include <vector>

#include "terminal/screen_buffer.h"
#include "ui/views/view.h"

namespace wtop {

inline constexpr int kSidebarWidth = 34;

// Draws Task Manager's left-hand resource list: a mini graph and two lines
// of summary per resource. Scrolls to keep `selected` visible.
void RenderSidebar(ScreenBuffer& buffer, const Rect& area,
                   const std::vector<SidebarEntry>& entries, int selected);

}  // namespace wtop

#endif  // WTOP_SRC_UI_SIDEBAR_H_
