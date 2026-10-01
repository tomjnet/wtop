#ifndef WTOP_SRC_UI_VIEWS_VIEWS_H_
#define WTOP_SRC_UI_VIEWS_VIEWS_H_

#include <cstddef>
#include <memory>

#include "collectors/cpu_collector.h"
#include "collectors/disk_collector.h"
#include "collectors/gpu_collector.h"
#include "collectors/memory_collector.h"
#include "collectors/network_collector.h"
#include "ui/views/view.h"

namespace wtop {

// The collectors must outlive the returned views.
std::unique_ptr<View> MakeCpuView(const CpuCollector& cpu);
std::unique_ptr<View> MakeMemoryView(const MemoryCollector& memory);
std::unique_ptr<View> MakeDiskView(const DiskCollector& disks,
                                   std::size_t index);
std::unique_ptr<View> MakeNetworkView(const NetworkCollector& network,
                                      std::size_t index);
std::unique_ptr<View> MakeGpuView(const GpuCollector& gpus, std::size_t index);

}  // namespace wtop

#endif  // WTOP_SRC_UI_VIEWS_VIEWS_H_
