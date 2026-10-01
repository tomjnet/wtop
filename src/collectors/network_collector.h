#ifndef WTOP_SRC_COLLECTORS_NETWORK_COLLECTOR_H_
#define WTOP_SRC_COLLECTORS_NETWORK_COLLECTOR_H_

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "core/ring_buffer.h"

namespace wtop {

struct NetworkAdapter {
  int index = 0;
  std::string name;         // Interface name, e.g. "wlan0".
  std::string kind;         // "Wi-Fi", "Ethernet" or "Network".
  std::string description;  // Hardware or driver name.
  std::string driver;
  std::string mac;
  std::string state;  // operstate, e.g. "up".
  std::string ipv4;
  std::string ipv6;
  int link_speed_mbps = -1;
  int mtu = 0;
  int signal_percent = -1;  // Wi-Fi link quality, when known.

  double send_bits_per_sec = 0.0;
  double receive_bits_per_sec = 0.0;
  std::uint64_t total_sent = 0;
  std::uint64_t total_received = 0;
  History send_history;
  History receive_history;

  bool has_previous = false;
};

// Samples /proc/net/dev and describes adapters using /sys/class/net and
// getifaddrs().
class NetworkCollector {
 public:
  NetworkCollector();

  void Update();

  const std::vector<NetworkAdapter>& adapters() const { return adapters_; }

 private:
  void Discover();
  void RefreshDetails();

  std::vector<NetworkAdapter> adapters_;
  std::chrono::steady_clock::time_point last_update_;
  int updates_since_refresh_ = 0;
};

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_NETWORK_COLLECTOR_H_
