#ifndef WTOP_SRC_APP_H_
#define WTOP_SRC_APP_H_

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "collectors/cpu_collector.h"
#include "collectors/disk_collector.h"
#include "collectors/gpu_collector.h"
#include "collectors/memory_collector.h"
#include "collectors/network_collector.h"
#include "terminal/input.h"
#include "terminal/screen_buffer.h"
#include "ui/views/view.h"

namespace wtop {

struct Options {
  std::chrono::milliseconds interval{1000};
  int initial_tab = 0;
};

class App {
 public:
  static constexpr int kMinWidth = 60;
  static constexpr int kMinHeight = 20;

  explicit App(const Options& options);

  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // Runs the interactive UI until the user quits. Returns the exit code.
  int Run();

  // Waits one interval, samples, then prints every tab as plain text at the
  // given size.
  int PrintSnapshot(int width, int height);

 private:
  enum class Overlay { kNone, kChooser, kHelp };

  void Sample();
  void Draw(ScreenBuffer& buffer) const;
  void HandleKey(const Key& key);
  void HandlePrefixedKey(const Key& key);
  void HandleChooserKey(const Key& key);
  void Select(int index);
  int TabCount() const { return static_cast<int>(views_.size()); }

  Options options_;
  CpuCollector cpu_;
  MemoryCollector memory_;
  DiskCollector disks_;
  NetworkCollector network_;
  GpuCollector gpus_;
  std::vector<std::unique_ptr<View>> views_;
  std::string hostname_;

  int active_ = 0;
  int last_active_ = 0;
  bool prefix_pending_ = false;
  Overlay overlay_ = Overlay::kNone;
  int chooser_index_ = 0;
  bool quit_ = false;
};

}  // namespace wtop

#endif  // WTOP_SRC_APP_H_
