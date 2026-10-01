#include "collectors/network_collector.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <string_view>
#include <system_error>

#include "collectors/proc_parsers.h"
#include "core/file_util.h"
#include "core/pci_ids.h"

namespace wtop {
namespace {

namespace fs = std::filesystem;

constexpr int kDetailRefreshInterval = 10;  // Samples between address scans.
constexpr int kArphrdLoopback = 772;        // From <linux/if_arp.h>.

fs::path SysNet(const std::string& name) {
  return fs::path("/sys/class/net") / name;
}

bool IsWireless(const std::string& name) {
  std::error_code ec;
  return fs::exists(SysNet(name) / "wireless", ec) ||
         fs::exists(SysNet(name) / "phy80211", ec);
}

bool HasHardwareDevice(const std::string& name) {
  std::error_code ec;
  return fs::exists(SysNet(name) / "device", ec);
}

struct Addresses {
  std::string ipv4;
  std::string ipv6;
};

std::map<std::string, Addresses> ReadAddresses() {
  std::map<std::string, Addresses> result;
  ifaddrs* list = nullptr;
  if (::getifaddrs(&list) != 0) return result;
  for (const ifaddrs* it = list; it != nullptr; it = it->ifa_next) {
    if (it->ifa_addr == nullptr) continue;
    Addresses& entry = result[it->ifa_name];
    char buffer[INET6_ADDRSTRLEN] = {};
    if (it->ifa_addr->sa_family == AF_INET && entry.ipv4.empty()) {
      const auto* addr = reinterpret_cast<const sockaddr_in*>(it->ifa_addr);
      if (::inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer))) {
        entry.ipv4 = buffer;
      }
    } else if (it->ifa_addr->sa_family == AF_INET6) {
      const auto* addr = reinterpret_cast<const sockaddr_in6*>(it->ifa_addr);
      if (!::inet_ntop(AF_INET6, &addr->sin6_addr, buffer, sizeof(buffer))) {
        continue;
      }
      // Prefer a global address over a link-local (fe80::) one.
      const bool link_local = IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr);
      if (entry.ipv6.empty() || (!link_local && entry.ipv6.starts_with("fe80")))
        entry.ipv6 = buffer;
    }
  }
  ::freeifaddrs(list);
  return result;
}

// Link quality from /proc/net/wireless ("wlan0: 0000   54.  -56.  ...").
int ReadSignalPercent(const std::string& name) {
  const std::string wireless = ReadFile("/proc/net/wireless");
  for (std::string_view line : SplitLines(wireless)) {
    const auto colon = line.find(':');
    if (colon == std::string_view::npos || Trim(line.substr(0, colon)) != name)
      continue;
    const auto fields = SplitWhitespace(line.substr(colon + 1));
    if (fields.size() < 2) return -1;
    std::string_view quality = fields[1];
    if (quality.ends_with('.')) quality.remove_suffix(1);
    const auto value = ParseNumber<double>(quality);
    // Most drivers report quality out of 70.
    if (value) return std::clamp(static_cast<int>(*value * 100 / 70), 0, 100);
  }
  return -1;
}

std::string DriverName(const std::string& name) {
  std::error_code ec;
  const fs::path link =
      fs::read_symlink(SysNet(name) / "device" / "driver", ec);
  return ec ? std::string() : link.filename().string();
}

}  // namespace

NetworkCollector::NetworkCollector() {
  Discover();
  RefreshDetails();
  Update();
  for (NetworkAdapter& adapter : adapters_) {
    adapter.send_history.Clear();
    adapter.receive_history.Clear();
  }
}

void NetworkCollector::Discover() {
  std::vector<std::string> hardware;
  std::vector<std::string> virtual_only;
  for (const std::string& name : ListDir("/sys/class/net")) {
    if (name == "lo" ||
        ReadInt(SysNet(name) / "type").value_or(0) == kArphrdLoopback) {
      continue;
    }
    // Disconnected wired adapters are hidden, like in Task Manager.
    if (ReadLine(SysNet(name) / "operstate") == "down" && !IsWireless(name)) {
      continue;
    }
    (HasHardwareDevice(name) ? hardware : virtual_only).push_back(name);
  }
  // Bridges, veth pairs and tunnels only matter when there is nothing else.
  const std::vector<std::string>& chosen =
      hardware.empty() ? virtual_only : hardware;

  int index = 0;
  for (const std::string& name : chosen) {
    NetworkAdapter adapter;
    adapter.index = index++;
    adapter.name = name;
    const bool wireless = IsWireless(name);
    const int type =
        static_cast<int>(ReadInt(SysNet(name) / "type").value_or(0));
    adapter.kind = wireless ? "Wi-Fi" : (type == 1 ? "Ethernet" : "Network");
    adapter.driver = DriverName(name);
    const fs::path device = SysNet(name) / "device";
    std::error_code ec;
    // Only PCI ids can be resolved through pci.ids; USB, SDIO and virtual
    // bus devices are described by their driver instead.
    if (fs::read_symlink(device / "subsystem", ec).filename() == "pci") {
      adapter.description = DescribePciDevice(device);
    }
    if (adapter.description.empty()) {
      adapter.description = adapter.driver.empty()
                                ? adapter.kind + " adapter"
                                : adapter.kind + " (" + adapter.driver + ")";
    }
    adapters_.push_back(std::move(adapter));
  }
}

void NetworkCollector::RefreshDetails() {
  const auto addresses = ReadAddresses();
  for (NetworkAdapter& adapter : adapters_) {
    const fs::path dir = SysNet(adapter.name);
    adapter.mac = ReadLine(dir / "address");
    adapter.state = ReadLine(dir / "operstate");
    adapter.mtu = static_cast<int>(ReadInt(dir / "mtu").value_or(0));
    adapter.link_speed_mbps =
        static_cast<int>(ReadInt(dir / "speed").value_or(-1));
    if (adapter.kind == "Wi-Fi") {
      adapter.signal_percent = ReadSignalPercent(adapter.name);
    }
    const auto it = addresses.find(adapter.name);
    adapter.ipv4 = it == addresses.end() ? "" : it->second.ipv4;
    adapter.ipv6 = it == addresses.end() ? "" : it->second.ipv6;
  }
}

void NetworkCollector::Update() {
  const auto now = std::chrono::steady_clock::now();
  const double seconds =
      std::chrono::duration<double>(now - last_update_).count();
  last_update_ = now;

  if (++updates_since_refresh_ >= kDetailRefreshInterval) {
    updates_since_refresh_ = 0;
    RefreshDetails();
  }

  const std::vector<NetDevStat> stats = ParseNetDev(ReadFile("/proc/net/dev"));
  for (NetworkAdapter& adapter : adapters_) {
    const auto it = std::find_if(
        stats.begin(), stats.end(),
        [&adapter](const NetDevStat& s) { return s.name == adapter.name; });
    if (it == stats.end()) continue;
    if (adapter.has_previous && seconds > 0.0) {
      const auto rate = [seconds](std::uint64_t now_value, std::uint64_t old) {
        if (now_value < old) return 0.0;  // Counter reset.
        return static_cast<double>(now_value - old) * 8.0 / seconds;
      };
      adapter.send_bits_per_sec = rate(it->tx_bytes, adapter.total_sent);
      adapter.receive_bits_per_sec = rate(it->rx_bytes, adapter.total_received);
    }
    adapter.total_sent = it->tx_bytes;
    adapter.total_received = it->rx_bytes;
    adapter.has_previous = true;
    adapter.send_history.Push(adapter.send_bits_per_sec);
    adapter.receive_history.Push(adapter.receive_bits_per_sec);
  }
}

}  // namespace wtop
