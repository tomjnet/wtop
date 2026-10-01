#include <algorithm>
#include <format>

#include "core/format.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

// Throughput graphs start at 100 Kbps and grow in 1-2-5 steps.
double ThroughputScale(const NetworkAdapter& adapter) {
  const double peak =
      std::max(adapter.send_history.Max(), adapter.receive_history.Max());
  return NiceCeiling(std::max(peak, 100e3));
}

std::string LinkSpeed(int mbps) {
  if (mbps <= 0) return "N/A";
  if (mbps >= 1000 && mbps % 1000 == 0)
    return std::format("{} Gbps", mbps / 1000);
  if (mbps >= 1000) return std::format("{:.1f} Gbps", mbps / 1000.0);
  return std::format("{} Mbps", mbps);
}

class NetworkView : public View {
 public:
  NetworkView(const NetworkCollector& network, std::size_t index)
      : network_(network), index_(index) {}

  std::string TabName() const override {
    return std::format("{} ({})", Adapter().kind, Adapter().name);
  }
  Color color() const override { return theme::kNetwork; }

  SidebarEntry Sidebar() const override {
    const NetworkAdapter& adapter = Adapter();
    return SidebarEntry{
        .title = adapter.kind,
        .line1 = adapter.name,
        .line2 = std::format("S: {} R: {}",
                             FormatBitsPerSec(adapter.send_bits_per_sec),
                             FormatBitsPerSec(adapter.receive_bits_per_sec)),
        .history = &adapter.receive_history,
        .max_value = ThroughputScale(adapter),
        .color = color(),
    };
  }

  PaneContent Content() const override {
    const NetworkAdapter& adapter = Adapter();
    const Color send_color = color().Mix(theme::kText, 0.45);
    const double scale = ThroughputScale(adapter);

    PaneContent content;
    content.title = adapter.kind;
    content.subtitle = adapter.description;
    content.color = color();
    content.graphs.push_back(GraphPanel{
        .label_left = "Throughput",
        .label_right = FormatBitsPerSec(scale),
        .series = {GraphSeries{&adapter.receive_history, color(), true},
                   GraphSeries{&adapter.send_history, send_color, false}},
        .options = GraphOptions{.max_value = scale},
    });
    content.stats = {
        {{.label = "Send",
          .value = FormatBitsPerSec(adapter.send_bits_per_sec),
          .accent = send_color,
          .dashed = true}},
        {{.label = "Receive",
          .value = FormatBitsPerSec(adapter.receive_bits_per_sec),
          .accent = color()}},
    };
    const auto or_na = [](const std::string& value) {
      return value.empty() ? std::string("N/A") : value;
    };
    content.details = {
        {"Adapter name", adapter.name},
        {"Connection type", adapter.kind},
        {"State", or_na(adapter.state)},
        {"Link speed", LinkSpeed(adapter.link_speed_mbps)},
        {"IPv4 address", or_na(adapter.ipv4)},
        {"IPv6 address", or_na(adapter.ipv6)},
        {"MAC address", or_na(adapter.mac)},
    };
    if (adapter.signal_percent >= 0) {
      content.details.push_back(
          {"Signal strength", std::format("{}%", adapter.signal_percent)});
    }
    content.details.push_back(
        {"Total sent", FormatBytes(static_cast<double>(adapter.total_sent))});
    content.details.push_back(
        {"Total received",
         FormatBytes(static_cast<double>(adapter.total_received))});
    return content;
  }

 private:
  const NetworkAdapter& Adapter() const { return network_.adapters()[index_]; }

  const NetworkCollector& network_;
  std::size_t index_;
};

}  // namespace

std::unique_ptr<View> MakeNetworkView(const NetworkCollector& network,
                                      std::size_t index) {
  return std::make_unique<NetworkView>(network, index);
}

}  // namespace wtop
