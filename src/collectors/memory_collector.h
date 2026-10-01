#ifndef WTOP_SRC_COLLECTORS_MEMORY_COLLECTOR_H_
#define WTOP_SRC_COLLECTORS_MEMORY_COLLECTOR_H_

#include <cstdint>

#include "core/ring_buffer.h"

namespace wtop {

// Memory figures in bytes, derived from /proc/meminfo.
struct MemoryStats {
  std::uint64_t total = 0;
  std::uint64_t free = 0;
  std::uint64_t available = 0;
  std::uint64_t in_use = 0;    // total - available.
  std::uint64_t cached = 0;    // Page cache + buffers + reclaimable slab.
  std::uint64_t modified = 0;  // Dirty + writeback pages.
  std::uint64_t standby = 0;   // Reclaimable cache that is not modified.
  std::uint64_t shared = 0;
  std::uint64_t buffers = 0;
  std::uint64_t committed = 0;
  std::uint64_t commit_limit = 0;  // RAM + swap.
  std::uint64_t slab_reclaimable = 0;
  std::uint64_t slab_unreclaimable = 0;
  std::uint64_t page_tables = 0;
  std::uint64_t kernel_stack = 0;
  std::uint64_t swap_total = 0;
  std::uint64_t swap_used = 0;
};

class MemoryCollector {
 public:
  MemoryCollector();

  void Update();

  const MemoryStats& stats() const { return stats_; }
  // In-use percentage history (0-100).
  const History& history() const { return history_; }

 private:
  void Read();

  MemoryStats stats_;
  History history_;
};

}  // namespace wtop

#endif  // WTOP_SRC_COLLECTORS_MEMORY_COLLECTOR_H_
