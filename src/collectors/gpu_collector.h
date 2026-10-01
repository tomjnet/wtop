#ifndef WTOP_SRC_COLLECTORS_GPU_COLLECTOR_H_
#define WTOP_SRC_COLLECTORS_GPU_COLLECTOR_H_

#include <chrono>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "collectors/proc_parsers.h"
#include "core/ring_buffer.h"

namespace wtop {

// Unknown numeric values are NaN.
struct GpuDevice {
  int index = 0;
  std::string name;
  std::string vendor;
  std::string driver;
  std::string driver_version;
  std::string location;  // e.g. "PCI bus 1, device 0, function 0".
  std::string source;    // "nvidia-smi" or "sysfs".

  double utilization;
  double memory_used;   // Bytes.
  double memory_total;  // Bytes.
  double temperature_c;
  double power_w;
  double power_limit_w;
  double clock_mhz;
  double max_clock_mhz;
  double encoder_utilization;
  double decoder_utilization;
  double fan_percent;

  History utilization_history;
  History memory_history;  // Bytes used.

  // Where the numbers come from.
  int nvidia_index = -1;
  std::string sysfs_card;  // e.g. "/sys/class/drm/card0".

  GpuDevice();
};

// Reports GPUs found through nvidia-smi and the DRM sysfs interface
// (amdgpu, i915, xe). nvidia-smi can take a noticeable time to answer, so it
// runs on a background thread and Update() only copies its latest result.
class GpuCollector {
 public:
  explicit GpuCollector(std::chrono::milliseconds interval);
  ~GpuCollector();

  GpuCollector(const GpuCollector&) = delete;
  GpuCollector& operator=(const GpuCollector&) = delete;

  void Update();

  const std::vector<GpuDevice>& gpus() const { return gpus_; }

 private:
  void DiscoverSysfs(bool skip_nvidia);
  void UpdateSysfs(GpuDevice& gpu) const;
  void PollNvidia(std::stop_token stop);

  std::chrono::milliseconds interval_;
  std::vector<GpuDevice> gpus_;

  std::mutex mutex_;
  std::vector<NvidiaGpuSample> latest_nvidia_;  // Guarded by mutex_.

  std::jthread worker_;  // Last member: stops before the rest is destroyed.
};

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_GPU_COLLECTOR_H_
