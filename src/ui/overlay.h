#ifndef WTOP_SRC_UI_OVERLAY_H_
#define WTOP_SRC_UI_OVERLAY_H_

#include <string>
#include <vector>

#include "terminal/screen_buffer.h"

namespace wtop {

struct ChooserItem {
  std::string name;
  std::string summary;
  Color color;
};

// tmux "choose-tree" style list of every tab, drawn centered over the UI.
void RenderChooser(ScreenBuffer& buffer, const std::vector<ChooserItem>& items,
                   int selected);

// Key binding reference, drawn centered over the UI.
void RenderHelp(ScreenBuffer& buffer);

// Shown instead of the UI when the terminal is below the minimum size.
void RenderTooSmall(ScreenBuffer& buffer, int min_width, int min_height);

}  // namespace wtop

#endif  // WTOP_SRC_UI_OVERLAY_H_
