#include "collectors/gpu_collector.h"

#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>

#include "core/file_util.h"
#include "core/pci_ids.h"

namespace wtop {
namespace {

namespace fs = std::filesystem;

constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
constexpr double kMib = 1024.0 * 1024.0;

std::optional<std::string> RunCommand(const std::string& command) {
  FILE* pipe = ::popen(command.c_str(), "r");
  if (pipe == nullptr) return std::nullopt;
  std::string output;
  char buffer[512];
  while (std::size_t n = std::fread(buffer, 1, sizeof(buffer), pipe)) {
    output.append(buffer, n);
  }
  if (::pclose(pipe) != 0) return std::nullopt;
  return output;
}

std::vector<NvidiaGpuSample> QueryNvidiaSmi() {
  const std::string command = std::format(
      "nvidia-smi --query-gpu={} --format=csv,noheader,nounits "
      "</dev/null 2>/dev/null",
      kNvidiaSmiQueryFields);
  const auto output = RunCommand(command);
  if (!output) return {};
  return ParseNvidiaSmiCsv(*output);
}

// "0000:01:00.0" or "00000000:01:00.0" -> "PCI bus 1, device 0, function 0".
std::string DescribePciLocation(std::string_view bus_id) {
  const auto parts = Split(bus_id, ':');
  if (parts.size() < 3) return std::string(bus_id);
  const auto dot = parts[2].find('.');
  if (dot == std::string_view::npos) return std::string(bus_id);
  unsigned bus = 0, device = 0, function = 0;
  std::from_chars(parts[1].data(), parts[1].data() + parts[1].size(), bus, 16);
  std::from_chars(parts[2].data(), parts[2].data() + dot, device, 16);
  std::from_chars(parts[2].data() + dot + 1, parts[2].data() + parts[2].size(),
                  function, 16);
  return std::format("PCI bus {}, device {}, function {}", bus, device,
                     function);
}

std::string VendorName(std::string_view vendor_id) {
  if (vendor_id == "0x10de") return "NVIDIA";
  if (vendor_id == "0x1002") return "AMD";
  if (vendor_id == "0x8086") return "Intel";
  return "GPU";
}

std::optional<double> ReadDouble(const fs::path& path) {
  return ParseNumber<double>(ReadLine(path));
}

// amdgpu: pp_dpm_sclk lists levels like "1: 1800Mhz *" (active one starred).
std::optional<double> ReadActiveDpmClock(const fs::path& path) {
  const std::string text = ReadFile(path);
  for (std::string_view line : SplitLines(text)) {
    if (!line.ends_with('*')) continue;
    const auto fields = SplitWhitespace(line);
    if (fields.size() < 2) continue;
    std::string_view value = fields[1];
    if (value.ends_with("Mhz") || value.ends_with("MHz"))
      value.remove_suffix(3);
    return ParseNumber<double>(value);
  }
  return std::nullopt;
}

}  // namespace

GpuDevice::GpuDevice()
    : utilization(kNan),
      memory_used(kNan),
      memory_total(kNan),
      temperature_c(kNan),
      power_w(kNan),
      power_limit_w(kNan),
      clock_mhz(kNan),
      max_clock_mhz(kNan),
      encoder_utilization(kNan),
      decoder_utilization(kNan),
      fan_percent(kNan) {}

GpuCollector::GpuCollector(std::chrono::milliseconds interval)
    : interval_(interval) {
  // Probe nvidia-smi once synchronously so the GPU list is known up front.
  const std::vector<NvidiaGpuSample> nvidia = QueryNvidiaSmi();
  DiscoverSysfs(/*skip_nvidia=*/!nvidia.empty());
  for (const NvidiaGpuSample& sample : nvidia) {
    GpuDevice gpu;
    gpu.name = sample.name;
    gpu.vendor = "NVIDIA";
    gpu.driver = "nvidia";
    gpu.driver_version = sample.driver_version;
    gpu.location = DescribePciLocation(sample.bus_id);
    gpu.source = "nvidia-smi";
    gpu.nvidia_index = sample.index;
    gpus_.push_back(std::move(gpu));
  }
  for (std::size_t i = 0; i < gpus_.size(); ++i) {
    gpus_[i].index = static_cast<int>(i);
  }
  if (!nvidia.empty()) {
    latest_nvidia_ = nvidia;
    worker_ = std::jthread([this](std::stop_token stop) { PollNvidia(stop); });
  }
}

GpuCollector::~GpuCollector() = default;

void GpuCollector::DiscoverSysfs(bool skip_nvidia) {
  for (const std::string& card : ListDir("/sys/class/drm")) {
    if (!card.starts_with("card") || !IsAllDigits(card.substr(4))) continue;
    const fs::path card_dir = fs::path("/sys/class/drm") / card;
    const fs::path device = card_dir / "device";
    const std::string vendor_id = ReadLine(device / "vendor");
    if (vendor_id.empty()) continue;
    if (skip_nvidia && vendor_id == "0x10de") continue;

    GpuDevice gpu;
    gpu.vendor = VendorName(vendor_id);
    gpu.name = DescribePciDevice(device);
    if (gpu.name.empty()) gpu.name = gpu.vendor + " GPU";
    std::error_code ec;
    gpu.driver = fs::read_symlink(device / "driver", ec).filename().string();
    gpu.driver_version =
        ReadLine(fs::path("/sys/module") / gpu.driver / "version");
    if (gpu.driver_version.empty()) {
      gpu.driver_version = ReadLine("/proc/sys/kernel/osrelease");
    }
    gpu.location =
        DescribePciLocation(fs::canonical(device, ec).filename().string());
    gpu.source = "sysfs";
    gpu.sysfs_card = card_dir.string();
    gpus_.push_back(std::move(gpu));
  }
}

void GpuCollector::UpdateSysfs(GpuDevice& gpu) const {
  const fs::path card(gpu.sysfs_card);
  const fs::path device = card / "device";
  gpu.utilization = ReadDouble(device / "gpu_busy_percent").value_or(kNan);
  if (auto used = ReadDouble(device / "mem_info_vram_used")) {
    gpu.memory_used = *used;
    gpu.memory_total =
        ReadDouble(device / "mem_info_vram_total").value_or(kNan);
  }
  // Intel exposes the current/max graphics clock on the card itself.
  if (auto clock = ReadDouble(card / "gt_cur_freq_mhz")) {
    gpu.clock_mhz = *clock;
    gpu.max_clock_mhz = ReadDouble(card / "gt_max_freq_mhz").value_or(kNan);
  } else if (auto dpm = ReadActiveDpmClock(device / "pp_dpm_sclk")) {
    gpu.clock_mhz = *dpm;
  }
  for (const std::string& hwmon : ListDir(device / "hwmon")) {
    const fs::path dir = device / "hwmon" / hwmon;
    if (auto millideg = ReadDouble(dir / "temp1_input")) {
      gpu.temperature_c = *millideg / 1000.0;
    }
    if (auto microwatt = ReadDouble(dir / "power1_average")) {
      gpu.power_w = *microwatt / 1e6;
    }
    break;
  }
}

void GpuCollector::PollNvidia(std::stop_token stop) {
  std::mutex wait_mutex;
  std::condition_variable_any wake;
  while (!stop.stop_requested()) {
    {
      std::unique_lock lock(wait_mutex);
      // Returns early when a stop is requested.
      wake.wait_for(lock, stop, interval_, [] { return false; });
    }
    if (stop.stop_requested()) break;
    std::vector<NvidiaGpuSample> samples = QueryNvidiaSmi();
    if (samples.empty()) continue;
    std::lock_guard lock(mutex_);
    latest_nvidia_ = std::move(samples);
  }
}

void GpuCollector::Update() {
  std::vector<NvidiaGpuSample> nvidia;
  {
    std::lock_guard lock(mutex_);
    nvidia = latest_nvidia_;
  }
  for (GpuDevice& gpu : gpus_) {
    if (gpu.nvidia_index >= 0) {
      for (const NvidiaGpuSample& s : nvidia) {
        if (s.index != gpu.nvidia_index) continue;
        gpu.utilization = s.utilization;
        gpu.memory_used = s.memory_used_mib * kMib;
        gpu.memory_total = s.memory_total_mib * kMib;
        gpu.temperature_c = s.temperature_c;
        gpu.power_w = s.power_draw_w;
        gpu.power_limit_w = s.power_limit_w;
        gpu.clock_mhz = s.clock_mhz;
        gpu.max_clock_mhz = s.max_clock_mhz;
        gpu.encoder_utilization = s.encoder_utilization;
        gpu.decoder_utilization = s.decoder_utilization;
        gpu.fan_percent = s.fan_percent;
      }
    } else {
      UpdateSysfs(gpu);
    }
    gpu.utilization_history.Push(std::isnan(gpu.utilization) ? 0.0
                                                             : gpu.utilization);
    gpu.memory_history.Push(std::isnan(gpu.memory_used) ? 0.0
                                                        : gpu.memory_used);
  }
}

}  // namespace wtop
