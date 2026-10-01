#include <format>

#include "core/format.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

class CpuView : public View {
 public:
  explicit CpuView(const CpuCollector& cpu) : cpu_(cpu) {}

  std::string TabName() const override { return "CPU"; }
  Color color() const override { return theme::kCpu; }

  SidebarEntry Sidebar() const override {
    return SidebarEntry{
        .title = "CPU",
        .line1 = std::format("{} {}", FormatPercent(cpu_.utilization()),
                             FormatFrequency(cpu_.current_mhz())),
        .line2 = std::format("{} cores, {} threads", cpu_.info().cores,
                             cpu_.info().logical_processors),
        .history = &cpu_.history(),
        .max_value = 100.0,
        .color = color(),
    };
  }

  PaneContent Content() const override {
    const CpuStaticInfo& info = cpu_.info();
    PaneContent content;
    content.title = "CPU";
    content.subtitle = info.model;
    content.color = color();
    content.graphs.push_back(GraphPanel{
        .label_left = "% Utilization",
        .label_right = "100%",
        .series = {GraphSeries{&cpu_.history(), color(), true}},
        .options = GraphOptions{.max_value = 100.0},
    });
    const double* load = cpu_.load_average();
    content.stats = {
        {{.label = "Utilization", .value = FormatPercent(cpu_.utilization())},
         {.label = "Speed", .value = FormatFrequency(cpu_.current_mhz())}},
        {{.label = "Processes", .value = std::to_string(cpu_.processes())},
         {.label = "Threads", .value = std::to_string(cpu_.threads())},
         {.label = "Load average",
          .value =
              std::format("{:.2f} {:.2f} {:.2f}", load[0], load[1], load[2])}},
        {{.label = "Up time",
          .value = FormatDuration(
              static_cast<std::uint64_t>(cpu_.uptime_seconds()))}},
    };
    content.details = {
        {"Base speed", FormatFrequency(info.base_mhz)},
        {"Sockets", std::to_string(info.sockets)},
        {"Cores", std::to_string(info.cores)},
        {"Logical processors", std::to_string(info.logical_processors)},
        {"Virtualization", info.virtualization},
    };
    if (info.virtual_machine)
      content.details.push_back({"Virtual machine", "Yes"});
    const auto cache = [](std::uint64_t bytes) {
      return bytes == 0 ? std::string("N/A")
                        : FormatBytes(static_cast<double>(bytes));
    };
    content.details.push_back({"L1 cache", cache(info.l1_cache)});
    content.details.push_back({"L2 cache", cache(info.l2_cache)});
    content.details.push_back({"L3 cache", cache(info.l3_cache)});
    return content;
  }

 private:
  const CpuCollector& cpu_;
};

}  // namespace

std::unique_ptr<View> MakeCpuView(const CpuCollector& cpu) {
  return std::make_unique<CpuView>(cpu);
}

}  // namespace wtop
