#ifndef WTOP_SRC_UI_STATUS_LINE_H_
#define WTOP_SRC_UI_STATUS_LINE_H_

#include <string>

#include "terminal/screen_buffer.h"

namespace wtop {

struct StatusInfo {
  std::string hostname;
  std::string active_tab;  // e.g. "2:Disk 0 (sda)".
  bool prefix_pending = false;
};

// Draws a tmux-style status line on row `y`: session name, prefix indicator
// and key hints on the left, host name and clock on the right.
void RenderStatusLine(ScreenBuffer& buffer, int y, const StatusInfo& info);

}  // namespace wtop

#endif  // WTOP_SRC_UI_STATUS_LINE_H_
