#ifndef WTOP_SRC_COLLECTORS_PROC_PARSERS_H_
#define WTOP_SRC_COLLECTORS_PROC_PARSERS_H_

// Pure parsers for the text formats of /proc, /sys and nvidia-smi. They take
// the file contents as input so they can be tested without a live system.

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wtop {

// One "cpu" line of /proc/stat, in clock ticks.
struct CpuTimes {
  std::uint64_t user = 0;
  std::uint64_t nice = 0;
  std::uint64_t system = 0;
  std::uint64_t idle = 0;
  std::uint64_t iowait = 0;
  std::uint64_t irq = 0;
  std::uint64_t softirq = 0;
  std::uint64_t steal = 0;

  std::uint64_t Idle() const { return idle + iowait; }
  std::uint64_t Total() const {
    return user + nice + system + idle + iowait + irq + softirq + steal;
  }
};

// Busy percentage (0-100) between two samples of the same CPU.
double CpuUtilization(const CpuTimes& previous, const CpuTimes& current);

// Parses /proc/stat. Element 0 is the aggregate "cpu" line, followed by
// cpu0, cpu1, ... in order.
std::vector<CpuTimes> ParseProcStat(std::string_view text);

// Parses /proc/meminfo into bytes, keyed by field name ("MemTotal", ...).
std::map<std::string, std::uint64_t, std::less<>> ParseMeminfo(
    std::string_view text);

// One line of /proc/diskstats.
struct DiskStat {
  std::string name;
  std::uint64_t reads = 0;
  std::uint64_t sectors_read = 0;
  std::uint64_t read_ms = 0;
  std::uint64_t writes = 0;
  std::uint64_t sectors_written = 0;
  std::uint64_t write_ms = 0;
  std::uint64_t io_ms = 0;  // Time the device had I/O in flight.
};

std::vector<DiskStat> ParseDiskstats(std::string_view text);

// One interface of /proc/net/dev.
struct NetDevStat {
  std::string name;
  std::uint64_t rx_bytes = 0;
  std::uint64_t tx_bytes = 0;
};

std::vector<NetDevStat> ParseNetDev(std::string_view text);

// Parses a sysfs CPU list such as "0-3,8,10-11".
std::vector<int> ParseCpuList(std::string_view text);

// Parses sysfs cache sizes such as "32K", "8192K" or "1M" into bytes.
std::optional<std::uint64_t> ParseCacheSize(std::string_view text);

// Extracts the nominal frequency from a CPU model name such as
// "Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz". Returns MHz.
std::optional<double> ParseModelFrequencyMhz(std::string_view model);

// One "processor" block of /proc/cpuinfo, reduced to the fields wtop uses.
struct CpuInfoEntry {
  std::string model_name;
  double mhz = 0.0;
  int physical_id = 0;
  int core_id = -1;
  std::string flags;
};

std::vector<CpuInfoEntry> ParseCpuinfo(std::string_view text);

// One row of `nvidia-smi --query-gpu=... --format=csv,noheader,nounits`
// using the column order in kNvidiaSmiQueryFields. Unavailable values are
// NaN.
struct NvidiaGpuSample {
  int index = 0;
  std::string name;
  std::string driver_version;
  std::string bus_id;
  double utilization = 0.0;
  double memory_utilization = 0.0;
  double encoder_utilization = 0.0;
  double decoder_utilization = 0.0;
  double memory_used_mib = 0.0;
  double memory_total_mib = 0.0;
  double temperature_c = 0.0;
  double power_draw_w = 0.0;
  double power_limit_w = 0.0;
  double clock_mhz = 0.0;
  double max_clock_mhz = 0.0;
  double fan_percent = 0.0;
};

inline constexpr std::string_view kNvidiaSmiQueryFields =
    "index,name,driver_version,pci.bus_id,utilization.gpu,"
    "utilization.memory,utilization.encoder,utilization.decoder,"
    "memory.used,memory.total,temperature.gpu,power.draw,power.limit,"
    "clocks.gr,clocks.max.gr,fan.speed";

std::vector<NvidiaGpuSample> ParseNvidiaSmiCsv(std::string_view text);

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_PROC_PARSERS_H_
