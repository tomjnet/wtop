#include "app.h"

#include <unistd.h>

#include <algorithm>
#include <format>
#include <iostream>
#include <thread>

#include "terminal/terminal.h"
#include "ui/function_bar.h"
#include "ui/overlay.h"
#include "ui/performance_pane.h"
#include "ui/sidebar.h"
#include "ui/status_line.h"
#include "ui/tab_bar.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

// The first sample comes quickly so the screen fills in right away.
constexpr std::chrono::milliseconds kFirstSampleDelay{300};
// The sidebar is only shown when the pane still has room for its stats.
constexpr int kSidebarMinScreenWidth = 110;

std::string Hostname() {
  char name[256] = {};
  if (::gethostname(name, sizeof(name) - 1) != 0) return "localhost";
  return name;
}

}  // namespace

App::App(const Options& options)
    : options_(options), gpus_(options.interval), hostname_(Hostname()) {
  views_.push_back(MakeCpuView(cpu_));
  views_.push_back(MakeMemoryView(memory_));
  for (std::size_t i = 0; i < disks_.disks().size(); ++i) {
    views_.push_back(MakeDiskView(disks_, i));
  }
  for (std::size_t i = 0; i < network_.adapters().size(); ++i) {
    views_.push_back(MakeNetworkView(network_, i));
  }
  for (std::size_t i = 0; i < gpus_.gpus().size(); ++i) {
    views_.push_back(MakeGpuView(gpus_, i));
  }
  active_ = std::clamp(options.initial_tab, 0, TabCount() - 1);
  last_active_ = active_;
}

void App::Sample() {
  cpu_.Update();
  memory_.Update();
  disks_.Update();
  network_.Update();
  gpus_.Update();
}

void App::Draw(ScreenBuffer& buffer) const {
  buffer.Clear();
  if (buffer.width() < kMinWidth || buffer.height() < kMinHeight) {
    RenderTooSmall(buffer, kMinWidth, kMinHeight);
    return;
  }

  std::vector<TabInfo> tabs;
  for (const auto& view : views_) {
    tabs.push_back(TabInfo{view->TabName(), view->color()});
  }
  RenderTabBar(buffer, 0, tabs, active_);

  const View& view = *views_[static_cast<std::size_t>(active_)];
  RenderFunctionBar(buffer, buffer.height() - 1,
                    {{"F1", "Help"}, {"F10", "Quit"}});
  RenderStatusLine(
      buffer, buffer.height() - 2,
      StatusInfo{
          .hostname = hostname_,
          .active_tab = std::format("{}:{}", active_, view.TabName()),
          .prefix_pending = prefix_pending_,
      });

  // Body between the tab bar and the status + function key lines.
  const Rect body{0, 1, buffer.width(), buffer.height() - 3};
  Rect pane{body.x + 2, body.y + 1, body.width - 4, body.height - 2};
  if (body.width >= kSidebarMinScreenWidth) {
    std::vector<SidebarEntry> entries;
    for (const auto& v : views_) entries.push_back(v->Sidebar());
    RenderSidebar(buffer,
                  Rect{body.x, body.y + 1, kSidebarWidth, body.height - 1},
                  entries, active_);
    const int divider_x = body.x + kSidebarWidth;
    for (int y = body.y; y < body.bottom(); ++y) {
      buffer.Set(divider_x, y, U'│', theme::kBorderStyle);
    }
    pane.x = divider_x + 3;
    pane.width = body.right() - pane.x - 2;
  }
  RenderPerformancePane(buffer, pane, view.Content());

  if (overlay_ == Overlay::kChooser) {
    std::vector<ChooserItem> items;
    for (const auto& v : views_) {
      const SidebarEntry entry = v->Sidebar();
      items.push_back(ChooserItem{v->TabName(), entry.line1, v->color()});
    }
    RenderChooser(buffer, items, chooser_index_);
  } else if (overlay_ == Overlay::kHelp) {
    RenderHelp(buffer);
  }
}

void App::Select(int index) {
  if (index < 0 || index >= TabCount() || index == active_) return;
  last_active_ = active_;
  active_ = index;
}

void App::HandleKey(const Key& key) {
  // htop-style function keys work in every state.
  if (key.IsChar(kCtrlC) || key.code == KeyCode::kF10) {
    quit_ = true;
    return;
  }
  if (key.code == KeyCode::kF1) {
    prefix_pending_ = false;
    overlay_ = overlay_ == Overlay::kHelp ? Overlay::kNone : Overlay::kHelp;
    return;
  }
  if (overlay_ == Overlay::kHelp) {
    overlay_ = Overlay::kNone;
    return;
  }
  if (overlay_ == Overlay::kChooser) {
    HandleChooserKey(key);
    return;
  }
  if (prefix_pending_) {
    prefix_pending_ = false;
    HandlePrefixedKey(key);
    return;
  }
  if (key.IsChar(kCtrlB)) {
    prefix_pending_ = true;
  } else if (key.IsChar('q')) {
    quit_ = true;
  }
}

void App::HandlePrefixedKey(const Key& key) {
  const int count = TabCount();
  switch (key.code) {
    case KeyCode::kLeft:
      Select((active_ + count - 1) % count);
      return;
    case KeyCode::kRight:
      Select((active_ + 1) % count);
      return;
    case KeyCode::kChar:
      break;
    default:
      return;
  }
  const char c = key.ch;
  if (c >= '0' && c <= '9') {
    Select(c - '0');
  } else if (c == 'n') {
    Select((active_ + 1) % count);
  } else if (c == 'p') {
    Select((active_ + count - 1) % count);
  } else if (c == 'l') {
    Select(last_active_);
  } else if (c == 'w') {
    overlay_ = Overlay::kChooser;
    chooser_index_ = active_;
  } else if (c == '?') {
    overlay_ = Overlay::kHelp;
  } else if (c == 'd') {
    quit_ = true;
  }
}

void App::HandleChooserKey(const Key& key) {
  const int count = TabCount();
  switch (key.code) {
    case KeyCode::kUp:
      chooser_index_ = (chooser_index_ + count - 1) % count;
      return;
    case KeyCode::kDown:
    case KeyCode::kTab:
      chooser_index_ = (chooser_index_ + 1) % count;
      return;
    case KeyCode::kHome:
      chooser_index_ = 0;
      return;
    case KeyCode::kEnd:
      chooser_index_ = count - 1;
      return;
    case KeyCode::kEnter:
      Select(chooser_index_);
      overlay_ = Overlay::kNone;
      return;
    case KeyCode::kEscape:
      overlay_ = Overlay::kNone;
      return;
    default:
      break;
  }
  if (key.code != KeyCode::kChar) return;
  if (key.ch == 'k') {
    chooser_index_ = (chooser_index_ + count - 1) % count;
  } else if (key.ch == 'j') {
    chooser_index_ = (chooser_index_ + 1) % count;
  } else if (key.ch >= '0' && key.ch <= '9' && key.ch - '0' < count) {
    Select(key.ch - '0');
    overlay_ = Overlay::kNone;
  } else if (key.ch == 'q') {
    overlay_ = Overlay::kNone;
  }
}

int App::Run() {
  Terminal terminal;
  if (!terminal.ok()) {
    std::cerr << "wtop: standard input and output must be a terminal\n";
    return 1;
  }

  using Clock = std::chrono::steady_clock;
  ScreenBuffer front;
  ScreenBuffer back;
  bool dirty = true;
  bool full_repaint = true;
  auto next_sample =
      Clock::now() + std::min(kFirstSampleDelay, options_.interval);

  while (!quit_ && !Terminal::QuitRequested()) {
    auto now = Clock::now();
    if (now >= next_sample) {
      Sample();
      next_sample += options_.interval;
      if (next_sample <= now) next_sample = now + options_.interval;
      dirty = true;
    }
    if (Terminal::ConsumeResize()) {
      dirty = true;
      full_repaint = true;
    }
    if (dirty) {
      const TerminalSize size = terminal.Size();
      back.Resize(size.columns, size.rows);
      Draw(back);
      terminal.Write(back.Render(front, full_repaint));
      std::swap(front, back);
      dirty = false;
      full_repaint = false;
    }

    now = Clock::now();
    const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::max(Clock::duration::zero(), next_sample - now));
    for (const Key& key : ReadKeys(wait)) {
      HandleKey(key);
      dirty = true;
    }
  }
  return 0;
}

int App::PrintSnapshot(int width, int height) {
  // The collectors took their baseline when constructed.
  std::this_thread::sleep_for(options_.interval);
  Sample();
  ScreenBuffer buffer(width, height);
  const int count = TabCount();
  for (int i = 0; i < count; ++i) {
    active_ = i;
    Draw(buffer);
    std::cout << buffer.ToPlainText();
    if (i + 1 < count) std::cout << std::string(width, '=') << '\n';
  }
  return 0;
}

}  // namespace wtop
