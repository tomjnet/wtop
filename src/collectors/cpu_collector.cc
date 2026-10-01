#include "collectors/cpu_collector.h"

#include <set>
#include <string_view>
#include <tuple>
#include <utility>

#include "core/file_util.h"

namespace wtop {
namespace {

constexpr std::string_view kCpuSysfs = "/sys/devices/system/cpu";

bool HasFlag(std::string_view flags, std::string_view flag) {
  for (std::string_view f : SplitWhitespace(flags)) {
    if (f == flag) return true;
  }
  return false;
}

// Names such as "cpu0", "cpu12" (but not "cpufreq" or "cpuidle").
bool IsCpuDirectory(std::string_view name) {
  return name.starts_with("cpu") && IsAllDigits(name.substr(3));
}

}  // namespace

CpuCollector::CpuCollector() {
  ReadStaticInfo();
  ReadCaches();
  previous_ = ParseProcStat(ReadFile("/proc/stat"));
  if (previous_.size() > 1) core_utilization_.assign(previous_.size() - 1, 0);
  current_mhz_ = ReadCurrentMhz();
  ReadProcessCounts();
}

void CpuCollector::Update() {
  std::vector<CpuTimes> current = ParseProcStat(ReadFile("/proc/stat"));
  if (!current.empty() && current.size() == previous_.size()) {
    utilization_ = CpuUtilization(previous_[0], current[0]);
    core_utilization_.resize(current.size() - 1);
    for (std::size_t i = 1; i < current.size(); ++i) {
      core_utilization_[i - 1] = CpuUtilization(previous_[i], current[i]);
    }
  }
  previous_ = std::move(current);
  history_.Push(utilization_);
  current_mhz_ = ReadCurrentMhz();
  ReadProcessCounts();
}

void CpuCollector::ReadStaticInfo() {
  const std::vector<CpuInfoEntry> entries =
      ParseCpuinfo(ReadFile("/proc/cpuinfo"));
  info_.logical_processors = static_cast<int>(entries.size());

  std::set<int> sockets;
  std::set<std::pair<int, int>> cores;
  for (const CpuInfoEntry& entry : entries) {
    sockets.insert(entry.physical_id);
    if (entry.core_id >= 0) cores.emplace(entry.physical_id, entry.core_id);
  }
  info_.sockets = std::max<int>(1, static_cast<int>(sockets.size()));
  info_.cores =
      cores.empty() ? info_.logical_processors : static_cast<int>(cores.size());

  if (!entries.empty()) {
    const CpuInfoEntry& first = entries.front();
    if (!first.model_name.empty()) info_.model = first.model_name;
    if (HasFlag(first.flags, "vmx") || HasFlag(first.flags, "svm")) {
      info_.virtualization = "Supported";
    }
    info_.virtual_machine = HasFlag(first.flags, "hypervisor");
  }

  // Prefer the firmware base frequency, then the nominal speed printed in
  // the model name, then whatever the kernel reports right now.
  const std::string cpu0 = std::string(kCpuSysfs) + "/cpu0/cpufreq/";
  if (auto khz = ReadInt(cpu0 + "base_frequency")) {
    info_.base_mhz = static_cast<double>(*khz) / 1000.0;
  } else if (auto mhz = ParseModelFrequencyMhz(info_.model)) {
    info_.base_mhz = *mhz;
  } else if (auto max_khz = ReadInt(cpu0 + "cpuinfo_max_freq")) {
    info_.base_mhz = static_cast<double>(*max_khz) / 1000.0;
  } else if (!entries.empty()) {
    info_.base_mhz = entries.front().mhz;
  }
}

void CpuCollector::ReadCaches() {
  // Every physical cache appears once per CPU sharing it; dedupe by its
  // level, type and shared CPU list, then sum the sizes per level.
  std::set<std::tuple<int, std::string, std::string>> seen;
  for (const std::string& cpu : ListDir(kCpuSysfs)) {
    if (!IsCpuDirectory(cpu)) continue;
    const std::string cache_dir = std::string(kCpuSysfs) + "/" + cpu + "/cache";
    for (const std::string& index : ListDir(cache_dir)) {
      if (!index.starts_with("index")) continue;
      const std::string dir = cache_dir + "/" + index + "/";
      const int level = static_cast<int>(ReadInt(dir + "level").value_or(0));
      const auto size = ParseCacheSize(ReadLine(dir + "size"));
      if (level == 0 || !size) continue;
      auto key = std::make_tuple(level, ReadLine(dir + "type"),
                                 ReadLine(dir + "shared_cpu_list"));
      if (!seen.insert(std::move(key)).second) continue;
      if (level == 1) info_.l1_cache += *size;
      if (level == 2) info_.l2_cache += *size;
      if (level == 3) info_.l3_cache += *size;
    }
  }
  if (info_.l3_cache == 0 && info_.l2_cache == 0) {
    // Fall back to the single "cache size" figure of /proc/cpuinfo.
    const std::string cpuinfo = ReadFile("/proc/cpuinfo");
    for (std::string_view line : SplitLines(cpuinfo)) {
      if (!line.starts_with("cache size")) continue;
      const auto colon = line.find(':');
      std::string_view value = Trim(line.substr(colon + 1));
      if (value.ends_with(" KB")) value.remove_suffix(3);
      info_.l3_cache = ParseNumber<std::uint64_t>(value).value_or(0) * 1024;
      break;
    }
  }
}

double CpuCollector::ReadCurrentMhz() const {
  double sum = 0.0;
  int count = 0;
  for (const std::string& cpu : ListDir(kCpuSysfs)) {
    if (!IsCpuDirectory(cpu)) continue;
    const auto khz = ReadInt(std::string(kCpuSysfs) + "/" + cpu +
                             "/cpufreq/scaling_cur_freq");
    if (!khz) continue;
    sum += static_cast<double>(*khz) / 1000.0;
    ++count;
  }
  if (count == 0) {
    for (const CpuInfoEntry& entry : ParseCpuinfo(ReadFile("/proc/cpuinfo"))) {
      if (entry.mhz <= 0.0) continue;
      sum += entry.mhz;
      ++count;
    }
  }
  return count > 0 ? sum / count : info_.base_mhz;
}

void CpuCollector::ReadProcessCounts() {
  int processes = 0;
  for (const std::string& name : ListDir("/proc")) {
    if (IsAllDigits(name)) ++processes;
  }
  processes_ = processes;

  // /proc/loadavg: "0.00 0.02 0.00 1/230 1488"; the 4th field holds the
  // number of runnable/total scheduling entities, i.e. threads.
  const std::string loadavg = ReadLine("/proc/loadavg");
  const auto fields = SplitWhitespace(loadavg);
  if (fields.size() >= 4) {
    for (int i = 0; i < 3; ++i) {
      load_average_[i] = ParseNumber<double>(fields[i]).value_or(0.0);
    }
    const auto slash = fields[3].find('/');
    if (slash != std::string_view::npos) {
      threads_ = ParseNumber<int>(fields[3].substr(slash + 1)).value_or(0);
    }
  }

  const std::string uptime_text = ReadLine("/proc/uptime");
  const auto uptime = SplitWhitespace(uptime_text);
  if (!uptime.empty()) {
    uptime_seconds_ = ParseNumber<double>(uptime[0]).value_or(0.0);
  }
}

}  // namespace wtop
