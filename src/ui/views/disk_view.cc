#include <algorithm>
#include <format>

#include "core/format.h"
#include "ui/theme.h"
#include "ui/views/views.h"

namespace wtop {
namespace {

// Transfer graphs start at 100 KB/s and grow in 1-2-5 steps of KB/s.
double TransferScale(const DiskDevice& disk) {
  const double peak =
      std::max(disk.read_history.Max(), disk.write_history.Max()) / 1024.0;
  return NiceCeiling(std::max(peak, 100.0)) * 1024.0;
}

std::string JoinMounts(const std::vector<std::string>& mounts) {
  if (mounts.empty()) return "Not mounted";
  std::string out;
  for (const std::string& mount : mounts) {
    if (!out.empty()) out += ", ";
    out += mount;
  }
  return out;
}

class DiskView : public View {
 public:
  DiskView(const DiskCollector& disks, std::size_t index)
      : disks_(disks), index_(index) {}

  std::string TabName() const override { return Title(); }
  Color color() const override { return theme::kDisk; }

  SidebarEntry Sidebar() const override {
    const DiskDevice& disk = Disk();
    return SidebarEntry{
        .title = Title(),
        .line1 = Type(disk),
        .line2 = FormatPercent(disk.active_percent),
        .history = &disk.active_history,
        .max_value = 100.0,
        .color = color(),
    };
  }

  PaneContent Content() const override {
    const DiskDevice& disk = Disk();
    const Color write_color = color().Mix(theme::kText, 0.45);
    const double scale = TransferScale(disk);

    PaneContent content;
    content.title = Title();
    content.subtitle = disk.model;
    content.color = color();
    content.graphs.push_back(GraphPanel{
        .label_left = "Active time",
        .label_right = "100%",
        .series = {GraphSeries{&disk.active_history, color(), true}},
        .options = GraphOptions{.max_value = 100.0},
        .weight = 3,
    });
    content.graphs.push_back(GraphPanel{
        .label_left = "Disk transfer rate",
        .label_right = FormatBytesPerSec(scale),
        .series = {GraphSeries{&disk.read_history, color(), true},
                   GraphSeries{&disk.write_history, write_color, false}},
        .options = GraphOptions{.max_value = scale},
        .weight = 1,
    });
    content.stats = {
        {{.label = "Active time", .value = FormatPercent(disk.active_percent)},
         {.label = "Average response time",
          .value = std::format("{:.1f} ms", disk.response_ms)}},
        {{.label = "Read speed",
          .value = FormatBytesPerSec(disk.read_bytes_per_sec),
          .accent = color()},
         {.label = "Write speed",
          .value = FormatBytesPerSec(disk.write_bytes_per_sec),
          .accent = write_color,
          .dashed = true}},
    };
    content.details = {
        {"Capacity", FormatBytes(static_cast<double>(disk.capacity))},
        {"Formatted", disk.formatted == 0
                          ? std::string("N/A")
                          : FormatBytes(static_cast<double>(disk.formatted))},
        {"System disk", disk.system_disk ? "Yes" : "No"},
        {"Swap", disk.has_swap ? "Yes" : "No"},
        {"Type", Type(disk)},
        {"Device", "/dev/" + disk.name},
        {"Mount points", JoinMounts(disk.mount_points)},
    };
    return content;
  }

 private:
  const DiskDevice& Disk() const { return disks_.disks()[index_]; }

  std::string Title() const {
    const DiskDevice& disk = Disk();
    return std::format("Disk {} ({})", disk.index, disk.name);
  }

  static std::string Type(const DiskDevice& disk) {
    std::string type = disk.rotational ? "HDD" : "SSD";
    if (disk.name.starts_with("nvme")) type = "NVMe SSD";
    if (disk.removable) type += " (removable)";
    return type;
  }

  const DiskCollector& disks_;
  std::size_t index_;
};

}  // namespace

std::unique_ptr<View> MakeDiskView(const DiskCollector& disks,
                                   std::size_t index) {
  return std::make_unique<DiskView>(disks, index);
}

}  // namespace wtop
