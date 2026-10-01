#ifndef WTOP_SRC_COLLECTORS_CPU_COLLECTOR_H_
#define WTOP_SRC_COLLECTORS_CPU_COLLECTOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "collectors/proc_parsers.h"
#include "core/ring_buffer.h"

namespace wtop {

// Hardware facts that do not change while wtop runs.
struct CpuStaticInfo {
  std::string model = "Unknown CPU";
  double base_mhz = 0.0;
  int sockets = 1;
  int cores = 0;
  int logical_processors = 0;
  std::string virtualization = "Not supported";
  bool virtual_machine = false;
  std::uint64_t l1_cache = 0;
  std::uint64_t l2_cache = 0;
  std::uint64_t l3_cache = 0;
};

// Samples /proc/stat, /proc/cpuinfo, /proc/loadavg and /proc/uptime.
class CpuCollector {
 public:
  CpuCollector();

  // Takes a new sample and appends utilization to the history.
  void Update();

  const CpuStaticInfo& info() const { return info_; }
  double utilization() const { return utilization_; }
  const std::vector<double>& core_utilization() const {
    return core_utilization_;
  }
  double current_mhz() const { return current_mhz_; }
  int processes() const { return processes_; }
  int threads() const { return threads_; }
  double uptime_seconds() const { return uptime_seconds_; }
  const double* load_average() const { return load_average_; }
  const History& history() const { return history_; }

 private:
  void ReadStaticInfo();
  void ReadCaches();
  double ReadCurrentMhz() const;
  void ReadProcessCounts();

  CpuStaticInfo info_;
  std::vector<CpuTimes> previous_;
  double utilization_ = 0.0;
  std::vector<double> core_utilization_;
  double current_mhz_ = 0.0;
  int processes_ = 0;
  int threads_ = 0;
  double uptime_seconds_ = 0.0;
  double load_average_[3] = {0.0, 0.0, 0.0};
  History history_;
};

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_CPU_COLLECTOR_H_
