#include "collectors/proc_parsers.h"

#include <cctype>
#include <cmath>
#include <limits>

#include "core/file_util.h"

namespace wtop {
namespace {

std::uint64_t ToU64(std::string_view text) {
  return ParseNumber<std::uint64_t>(text).value_or(0);
}

// nvidia-smi reports unavailable values as "[N/A]" or "[Not Supported]".
double ToDoubleOrNan(std::string_view text) {
  return ParseNumber<double>(text).value_or(
      std::numeric_limits<double>::quiet_NaN());
}

}  // namespace

double CpuUtilization(const CpuTimes& previous, const CpuTimes& current) {
  const std::uint64_t total = current.Total() - previous.Total();
  const std::uint64_t idle = current.Idle() - previous.Idle();
  if (current.Total() < previous.Total() || total == 0 || idle > total) {
    return 0.0;
  }
  return 100.0 * static_cast<double>(total - idle) / static_cast<double>(total);
}

std::vector<CpuTimes> ParseProcStat(std::string_view text) {
  std::vector<CpuTimes> result;
  for (std::string_view line : SplitLines(text)) {
    if (!line.starts_with("cpu")) continue;
    const auto fields = SplitWhitespace(line);
    if (fields.size() < 5) continue;
    CpuTimes times;
    std::uint64_t* slots[] = {&times.user,    &times.nice,   &times.system,
                              &times.idle,    &times.iowait, &times.irq,
                              &times.softirq, &times.steal};
    for (std::size_t i = 0; i < std::size(slots) && i + 1 < fields.size();
         ++i) {
      *slots[i] = ToU64(fields[i + 1]);
    }
    result.push_back(times);
  }
  return result;
}

std::map<std::string, std::uint64_t, std::less<>> ParseMeminfo(
    std::string_view text) {
  std::map<std::string, std::uint64_t, std::less<>> result;
  for (std::string_view line : SplitLines(text)) {
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) continue;
    const auto fields = SplitWhitespace(line.substr(colon + 1));
    if (fields.empty()) continue;
    std::uint64_t value = ToU64(fields[0]);
    if (fields.size() > 1 && fields[1] == "kB") value *= 1024;
    result.emplace(std::string(line.substr(0, colon)), value);
  }
  return result;
}

std::vector<DiskStat> ParseDiskstats(std::string_view text) {
  std::vector<DiskStat> result;
  for (std::string_view line : SplitLines(text)) {
    const auto f = SplitWhitespace(line);
    if (f.size() < 14) continue;
    DiskStat stat;
    stat.name = std::string(f[2]);
    stat.reads = ToU64(f[3]);
    stat.sectors_read = ToU64(f[5]);
    stat.read_ms = ToU64(f[6]);
    stat.writes = ToU64(f[7]);
    stat.sectors_written = ToU64(f[9]);
    stat.write_ms = ToU64(f[10]);
    stat.io_ms = ToU64(f[12]);
    result.push_back(std::move(stat));
  }
  return result;
}

std::vector<NetDevStat> ParseNetDev(std::string_view text) {
  std::vector<NetDevStat> result;
  for (std::string_view line : SplitLines(text)) {
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) continue;
    const auto fields = SplitWhitespace(line.substr(colon + 1));
    if (fields.size() < 9) continue;
    NetDevStat stat;
    stat.name = std::string(Trim(line.substr(0, colon)));
    stat.rx_bytes = ToU64(fields[0]);
    stat.tx_bytes = ToU64(fields[8]);
    result.push_back(std::move(stat));
  }
  return result;
}

std::vector<int> ParseCpuList(std::string_view text) {
  std::vector<int> cpus;
  for (std::string_view part : Split(Trim(text), ',')) {
    part = Trim(part);
    if (part.empty()) continue;
    const auto dash = part.find('-');
    const auto first = ParseNumber<int>(part.substr(0, dash));
    if (!first) continue;
    int last = *first;
    if (dash != std::string_view::npos) {
      last = ParseNumber<int>(part.substr(dash + 1)).value_or(*first);
    }
    for (int cpu = *first; cpu <= last; ++cpu) cpus.push_back(cpu);
  }
  return cpus;
}

std::optional<std::uint64_t> ParseCacheSize(std::string_view text) {
  text = Trim(text);
  if (text.empty()) return std::nullopt;
  std::uint64_t multiplier = 1;
  switch (std::toupper(static_cast<unsigned char>(text.back()))) {
    case 'K':
      multiplier = 1024;
      break;
    case 'M':
      multiplier = 1024 * 1024;
      break;
    case 'G':
      multiplier = 1024 * 1024 * 1024;
      break;
    default:
      break;
  }
  if (multiplier != 1) text.remove_suffix(1);
  const auto value = ParseNumber<std::uint64_t>(text);
  if (!value) return std::nullopt;
  return *value * multiplier;
}

std::optional<double> ParseModelFrequencyMhz(std::string_view model) {
  const auto at = model.rfind('@');
  if (at == std::string_view::npos) return std::nullopt;
  std::string_view rest = Trim(model.substr(at + 1));
  double scale = 0.0;
  if (rest.ends_with("GHz")) {
    scale = 1000.0;
  } else if (rest.ends_with("MHz")) {
    scale = 1.0;
  } else {
    return std::nullopt;
  }
  rest.remove_suffix(3);
  const auto value = ParseNumber<double>(rest);
  if (!value) return std::nullopt;
  return *value * scale;
}

std::vector<CpuInfoEntry> ParseCpuinfo(std::string_view text) {
  std::vector<CpuInfoEntry> entries;
  for (std::string_view line : SplitLines(text)) {
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) continue;
    const std::string_view key = Trim(line.substr(0, colon));
    const std::string_view value = Trim(line.substr(colon + 1));
    if (key == "processor") {
      entries.emplace_back();
      continue;
    }
    if (entries.empty()) continue;
    CpuInfoEntry& entry = entries.back();
    if (key == "model name") {
      entry.model_name = std::string(value);
    } else if (key == "cpu MHz") {
      entry.mhz = ParseNumber<double>(value).value_or(0.0);
    } else if (key == "physical id") {
      entry.physical_id = ParseNumber<int>(value).value_or(0);
    } else if (key == "core id") {
      entry.core_id = ParseNumber<int>(value).value_or(-1);
    } else if (key == "flags" || key == "Features") {
      entry.flags = std::string(value);
    }
  }
  return entries;
}

std::vector<NvidiaGpuSample> ParseNvidiaSmiCsv(std::string_view text) {
  std::vector<NvidiaGpuSample> result;
  for (std::string_view line : SplitLines(text)) {
    const auto f = Split(line, ',');
    if (f.size() < 16) continue;
    NvidiaGpuSample gpu;
    gpu.index = ParseNumber<int>(f[0]).value_or(0);
    gpu.name = std::string(Trim(f[1]));
    gpu.driver_version = std::string(Trim(f[2]));
    gpu.bus_id = std::string(Trim(f[3]));
    double* numbers[] = {&gpu.utilization,         &gpu.memory_utilization,
                         &gpu.encoder_utilization, &gpu.decoder_utilization,
                         &gpu.memory_used_mib,     &gpu.memory_total_mib,
                         &gpu.temperature_c,       &gpu.power_draw_w,
                         &gpu.power_limit_w,       &gpu.clock_mhz,
                         &gpu.max_clock_mhz,       &gpu.fan_percent};
    for (std::size_t i = 0; i < std::size(numbers); ++i) {
      *numbers[i] = ToDoubleOrNan(f[4 + i]);
    }
    result.push_back(std::move(gpu));
  }
  return result;
}

}  // namespace wtop
