#ifndef WTOP_SRC_COLLECTORS_DISK_COLLECTOR_H_
#define WTOP_SRC_COLLECTORS_DISK_COLLECTOR_H_

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "collectors/proc_parsers.h"
#include "core/ring_buffer.h"

namespace wtop {

// A physical (whole) block device such as "sda" or "nvme0n1".
struct DiskDevice {
  int index = 0;
  std::string name;
  std::string model;
  std::uint64_t capacity = 0;   // Bytes, from /sys/block/<dev>/size.
  std::uint64_t formatted = 0;  // Bytes of mounted file systems on it.
  bool rotational = false;
  bool removable = false;
  bool system_disk = false;  // Holds the "/" file system.
  bool has_swap = false;
  std::vector<std::string> mount_points;

  double active_percent = 0.0;
  double response_ms = 0.0;
  double read_bytes_per_sec = 0.0;
  double write_bytes_per_sec = 0.0;
  History active_history;
  History read_history;
  History write_history;

  DiskStat previous;
  bool has_previous = false;
};

// Samples /proc/diskstats and describes disks from /sys/block, /proc/mounts
// and /proc/swaps.
class DiskCollector {
 public:
  DiskCollector();

  void Update();

  const std::vector<DiskDevice>& disks() const { return disks_; }

 private:
  void Discover();
  void RefreshLayout();

  std::vector<DiskDevice> disks_;
  std::chrono::steady_clock::time_point last_update_;
  int updates_since_refresh_ = 0;
};

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_DISK_COLLECTOR_H_
