#include "collectors/memory_collector.h"

#include <algorithm>
#include <string_view>

#include "collectors/proc_parsers.h"
#include "core/file_util.h"

namespace wtop {

MemoryCollector::MemoryCollector() { Read(); }

void MemoryCollector::Update() {
  Read();
  const double percent = stats_.total == 0
                             ? 0.0
                             : 100.0 * static_cast<double>(stats_.in_use) /
                                   static_cast<double>(stats_.total);
  history_.Push(percent);
}

void MemoryCollector::Read() {
  const auto info = ParseMeminfo(ReadFile("/proc/meminfo"));
  const auto get = [&info](std::string_view key) -> std::uint64_t {
    const auto it = info.find(key);
    return it == info.end() ? 0 : it->second;
  };

  MemoryStats s;
  s.total = get("MemTotal");
  s.free = get("MemFree");
  s.available = info.contains("MemAvailable") ? get("MemAvailable")
                                              : s.free + get("Cached");
  s.available = std::min(s.available, s.total);
  s.in_use = s.total - s.available;
  s.buffers = get("Buffers");
  s.shared = get("Shmem");
  s.slab_reclaimable = get("SReclaimable");
  s.slab_unreclaimable = get("SUnreclaim");
  s.cached = get("Cached") + s.buffers + s.slab_reclaimable;
  s.modified = get("Dirty") + get("Writeback");
  const std::uint64_t reclaimable =
      s.available > s.free ? s.available - s.free : 0;
  s.modified = std::min(s.modified, reclaimable);
  s.standby = reclaimable - s.modified;
  s.committed = get("Committed_AS");
  s.swap_total = get("SwapTotal");
  s.swap_used = s.swap_total - std::min(s.swap_total, get("SwapFree"));
  s.commit_limit = s.total + s.swap_total;
  s.page_tables = get("PageTables");
  s.kernel_stack = get("KernelStack");
  stats_ = s;
}

}  // namespace wtop
