#ifndef WTOP_SRC_UI_FUNCTION_BAR_H_
#define WTOP_SRC_UI_FUNCTION_BAR_H_

#include <string>
#include <vector>

#include "terminal/screen_buffer.h"

namespace wtop {

struct FunctionKeyItem {
  std::string key;    // e.g. "F1".
  std::string label;  // e.g. "Help".
};

// Draws an htop-style function key bar ("F1Help  F10Quit") on row `y`.
void RenderFunctionBar(ScreenBuffer& buffer, int y,
                       const std::vector<FunctionKeyItem>& items);

}  // namespace wtop

#endif  // WTOP_SRC_UI_FUNCTION_BAR_H_
