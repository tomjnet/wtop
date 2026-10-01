#include <format>

#include "core/format.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

class MemoryView : public View {
 public:
  explicit MemoryView(const MemoryCollector& memory) : memory_(memory) {}

  std::string TabName() const override { return "Memory"; }
  Color color() const override { return theme::kMemory; }

  SidebarEntry Sidebar() const override {
    const MemoryStats& s = memory_.stats();
    return SidebarEntry{
        .title = "Memory",
        .line1 = std::format("{} ({})", FormatBytesPair(s.in_use, s.total),
                             FormatPercent(Percent(s.in_use, s.total))),
        .line2 =
            std::format("Swap {}", FormatBytesPair(s.swap_used, s.swap_total)),
        .history = &memory_.history(),
        .max_value = 100.0,
        .color = color(),
    };
  }

  PaneContent Content() const override {
    const MemoryStats& s = memory_.stats();
    const auto bytes = [](std::uint64_t value) {
      return FormatBytes(static_cast<double>(value));
    };
    const auto fraction = [&s](std::uint64_t value) {
      return s.total == 0
                 ? 0.0
                 : static_cast<double>(value) / static_cast<double>(s.total);
    };

    PaneContent content;
    content.title = "Memory";
    content.subtitle = bytes(s.total);
    content.color = color();
    content.graphs.push_back(GraphPanel{
        .label_left = "Memory usage",
        .label_right = bytes(s.total),
        .series = {GraphSeries{&memory_.history(), color(), true}},
        .options = GraphOptions{.max_value = 100.0},
    });
    content.bar_label =
        "Memory composition  (in use | modified | standby | free)";
    content.bar = {
        {fraction(s.in_use), color()},
        {fraction(s.modified), color().Mix(theme::kNetwork, 0.5)},
        {fraction(s.standby), color().Mix(theme::kBlack, 0.55)},
    };
    content.stats = {
        {{.label = "In use", .value = bytes(s.in_use)},
         {.label = "Available", .value = bytes(s.available)}},
        {{.label = "Committed",
          .value = FormatBytesPair(s.committed, s.commit_limit)},
         {.label = "Cached", .value = bytes(s.cached)}},
        {{.label = "Slab (reclaimable)", .value = bytes(s.slab_reclaimable)},
         {.label = "Slab (unreclaimable)",
          .value = bytes(s.slab_unreclaimable)}},
    };
    content.details = {
        {"Total", bytes(s.total)},
        {"Free", bytes(s.free)},
        {"Modified", bytes(s.modified)},
        {"Standby", bytes(s.standby)},
        {"Buffers", bytes(s.buffers)},
        {"Shared", bytes(s.shared)},
        {"Page tables", bytes(s.page_tables)},
        {"Kernel stack", bytes(s.kernel_stack)},
        {"Swap", s.swap_total == 0
                     ? std::string("None")
                     : FormatBytesPair(s.swap_used, s.swap_total)},
    };
    return content;
  }

 private:
  static double Percent(std::uint64_t part, std::uint64_t whole) {
    return whole == 0
               ? 0.0
               : 100.0 * static_cast<double>(part) / static_cast<double>(whole);
  }

  const MemoryCollector& memory_;
};

}  // namespace

std::unique_ptr<View> MakeMemoryView(const MemoryCollector& memory) {
  return std::make_unique<MemoryView>(memory);
}

}  // namespace wtop
