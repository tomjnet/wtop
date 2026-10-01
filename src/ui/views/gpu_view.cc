#include <cmath>
#include <format>

#include "core/format.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

std::string Watts(double value) {
  return std::isfinite(value) ? std::format("{:.1f} W", value) : "N/A";
}

class GpuView : public View {
 public:
  GpuView(const GpuCollector& gpus, std::size_t index)
      : gpus_(gpus), index_(index) {}

  std::string TabName() const override {
    return std::format("GPU {}", Gpu().index);
  }
  Color color() const override { return theme::kGpu; }

  SidebarEntry Sidebar() const override {
    const GpuDevice& gpu = Gpu();
    std::string line2 = FormatPercent(gpu.utilization);
    if (std::isfinite(gpu.temperature_c)) {
      line2 += " (" + FormatCelsius(gpu.temperature_c) + ")";
    }
    return SidebarEntry{
        .title = TabName(),
        .line1 = gpu.name,
        .line2 = line2,
        .history = &gpu.utilization_history,
        .max_value = 100.0,
        .color = color(),
    };
  }

  PaneContent Content() const override {
    const GpuDevice& gpu = Gpu();
    const bool has_memory =
        std::isfinite(gpu.memory_total) && gpu.memory_total > 0.0;

    PaneContent content;
    content.title = "GPU";
    content.subtitle = gpu.name;
    content.color = color();
    content.graphs.push_back(GraphPanel{
        .label_left = "Utilization",
        .label_right = "100%",
        .series = {GraphSeries{&gpu.utilization_history, color(), true}},
        .options = GraphOptions{.max_value = 100.0},
        .weight = 3,
    });
    if (has_memory) {
      content.graphs.push_back(GraphPanel{
          .label_left = "Dedicated GPU memory",
          .label_right = FormatBytes(gpu.memory_total),
          .series = {GraphSeries{&gpu.memory_history, color(), true}},
          .options = GraphOptions{.max_value = gpu.memory_total},
          .weight = 1,
      });
    }
    const std::string memory =
        has_memory ? FormatBytesPair(gpu.memory_used, gpu.memory_total) : "N/A";
    std::string power = Watts(gpu.power_w);
    if (std::isfinite(gpu.power_limit_w))
      power += " / " + Watts(gpu.power_limit_w);
    content.stats = {
        {{.label = "Utilization", .value = FormatPercent(gpu.utilization)},
         {.label = "Dedicated GPU memory", .value = memory}},
        {{.label = "Temperature", .value = FormatCelsius(gpu.temperature_c)},
         {.label = "Clock speed", .value = FormatFrequency(gpu.clock_mhz)}},
        {{.label = "Power", .value = power}},
    };
    content.details = {
        {"Vendor", gpu.vendor},
        {"Driver", gpu.driver.empty() ? std::string("N/A") : gpu.driver},
        {"Driver version",
         gpu.driver_version.empty() ? std::string("N/A") : gpu.driver_version},
        {"Physical location",
         gpu.location.empty() ? std::string("N/A") : gpu.location},
        {"Max clock speed", FormatFrequency(gpu.max_clock_mhz)},
        {"Video encode", FormatPercent(gpu.encoder_utilization)},
        {"Video decode", FormatPercent(gpu.decoder_utilization)},
        {"Fan", FormatPercent(gpu.fan_percent)},
        {"Data source", gpu.source},
    };
    return content;
  }

 private:
  const GpuDevice& Gpu() const { return gpus_.gpus()[index_]; }

  const GpuCollector& gpus_;
  std::size_t index_;
};

}  // namespace

std::unique_ptr<View> MakeGpuView(const GpuCollector& gpus, std::size_t index) {
  return std::make_unique<GpuView>(gpus, index);
}

}  // namespace wtop
