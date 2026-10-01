#ifndef WTOP_SRC_UI_VIEWS_VIEW_H_
#define WTOP_SRC_UI_VIEWS_VIEW_H_

#include <string>

#include "core/ring_buffer.h"
#include "terminal/screen_buffer.h"
#include "ui/performance_pane.h"

namespace wtop {

// What a resource shows in the left navigation list.
struct SidebarEntry {
  std::string title;  // e.g. "Disk 0 (sda)".
  std::string line1;  // e.g. "SSD".
  std::string line2;  // e.g. "3%".
  const History* history = nullptr;
  double max_value = 100.0;
  Color color;
};

// One screen/tab of wtop. Views are thin adapters that turn collector data
// into sidebar entries and pane content; they never read the system.
class View {
 public:
  virtual ~View() = default;

  // Short name shown on the tab bar.
  virtual std::string TabName() const = 0;
  virtual Color color() const = 0;
  virtual SidebarEntry Sidebar() const = 0;
  virtual PaneContent Content() const = 0;
};

}  // namespace wtop

#endif  // WTOP_SRC_UI_VIEWS_VIEW_H_
