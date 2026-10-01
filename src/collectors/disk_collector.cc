#include "collectors/disk_collector.h"

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <set>
#include <string_view>
#include <system_error>

#include "core/file_util.h"

namespace wtop {
namespace {

namespace fs = std::filesystem;

constexpr std::uint64_t kSectorSize = 512;  // diskstats always uses 512.
constexpr int kLayoutRefreshInterval = 30;  // Samples between mount scans.

bool IsPhysicalDiskName(std::string_view name) {
  for (std::string_view prefix : {"loop", "ram", "zram", "dm-", "fd", "nbd"}) {
    if (name.starts_with(prefix)) return false;
  }
  return true;
}

// Undoes the octal escaping /proc/mounts applies to spaces and tabs.
std::string UnescapeMountPath(std::string_view text) {
  std::string out;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\\' && i + 3 < text.size()) {
      const auto code = text.substr(i + 1, 3);
      int value = 0;
      auto [ptr, ec] =
          std::from_chars(code.data(), code.data() + code.size(), value, 8);
      if (ec == std::errc() && ptr == code.data() + code.size()) {
        out.push_back(static_cast<char>(value));
        i += 3;
        continue;
      }
    }
    out.push_back(text[i]);
  }
  return out;
}

// Returns the name of the whole disk in `disks` that backs the block device
// described by `sysfs_path` (a /sys/class/block or /sys/dev/block entry).
// Device-mapper devices are followed through their "slaves".
std::string FindParentDisk(const fs::path& sysfs_path,
                           const std::set<std::string>& disks, int depth = 0) {
  std::error_code ec;
  const fs::path real = fs::canonical(sysfs_path, ec);
  if (ec || depth > 4) return {};
  for (const fs::path& part : real) {
    if (disks.contains(part.string())) return part.string();
  }
  for (const std::string& slave : ListDir(real / "slaves")) {
    std::string parent =
        FindParentDisk(fs::path("/sys/class/block") / slave, disks, depth + 1);
    if (!parent.empty()) return parent;
  }
  return {};
}

// Maps a device path ("/dev/sda1", "/dev/mapper/root", "/dev/root") or any
// file to the whole disk holding it.
std::string DiskForPath(const std::string& path,
                        const std::set<std::string>& disks,
                        const std::string& fallback_file = {}) {
  std::error_code ec;
  if (path.starts_with("/dev/")) {
    const fs::path resolved = fs::canonical(path, ec);
    if (!ec) {
      std::string parent = FindParentDisk(
          fs::path("/sys/class/block") / resolved.filename(), disks);
      if (!parent.empty()) return parent;
    }
  }
  // Fall back to the device number of the file system containing the file.
  struct stat st{};
  const std::string& target = fallback_file.empty() ? path : fallback_file;
  if (::stat(target.c_str(), &st) != 0) return {};
  return FindParentDisk(
      std::format("/sys/dev/block/{}:{}", major(st.st_dev), minor(st.st_dev)),
      disks);
}

}  // namespace

DiskCollector::DiskCollector() {
  Discover();
  RefreshLayout();
  // Prime the counters so the first Update() yields real rates.
  Update();
  for (DiskDevice& disk : disks_) {
    disk.active_history.Clear();
    disk.read_history.Clear();
    disk.write_history.Clear();
  }
}

void DiskCollector::Discover() {
  int index = 0;
  for (const std::string& name : ListDir("/sys/block")) {
    if (!IsPhysicalDiskName(name)) continue;
    const std::string dir = "/sys/block/" + name + "/";
    const std::int64_t sectors = ReadInt(dir + "size").value_or(0);
    if (sectors <= 0) continue;
    DiskDevice disk;
    disk.index = index++;
    disk.name = name;
    disk.capacity = static_cast<std::uint64_t>(sectors) * kSectorSize;
    disk.rotational = ReadInt(dir + "queue/rotational").value_or(0) == 1;
    disk.removable = ReadInt(dir + "removable").value_or(0) == 1;
    const std::string vendor = ReadLine(dir + "device/vendor");
    disk.model = ReadLine(dir + "device/model");
    if (disk.model.empty()) disk.model = vendor;
    if (disk.model.empty()) disk.model = name;
    disks_.push_back(std::move(disk));
  }
}

void DiskCollector::RefreshLayout() {
  std::set<std::string> names;
  for (const DiskDevice& disk : disks_) names.insert(disk.name);

  std::map<std::string, std::vector<std::string>> mounts;
  std::map<std::string, std::uint64_t> formatted;
  std::set<std::string> counted_sources;
  std::string system_disk;
  const std::string mounts_text = ReadFile("/proc/mounts");
  for (std::string_view line : SplitLines(mounts_text)) {
    const auto fields = SplitWhitespace(line);
    if (fields.size() < 3 || !fields[0].starts_with("/dev/")) continue;
    const std::string source(fields[0]);
    const std::string mount_point = UnescapeMountPath(fields[1]);
    const std::string disk = DiskForPath(source, names, mount_point);
    if (disk.empty()) continue;
    mounts[disk].push_back(mount_point);
    if (mount_point == "/") system_disk = disk;
    // The same file system can be mounted many times; count it once.
    if (!counted_sources.insert(source).second) continue;
    struct statvfs vfs{};
    if (::statvfs(mount_point.c_str(), &vfs) == 0) {
      formatted[disk] +=
          static_cast<std::uint64_t>(vfs.f_blocks) * vfs.f_frsize;
    }
  }

  std::set<std::string> swap_disks;
  const std::string swaps_text = ReadFile("/proc/swaps");
  const auto swap_lines = SplitLines(swaps_text);
  for (std::size_t i = 1; i < swap_lines.size(); ++i) {
    const auto fields = SplitWhitespace(swap_lines[i]);
    if (fields.empty()) continue;
    const std::string disk = DiskForPath(UnescapeMountPath(fields[0]), names);
    if (!disk.empty()) swap_disks.insert(disk);
  }

  for (DiskDevice& disk : disks_) {
    disk.mount_points = mounts[disk.name];
    disk.formatted = formatted[disk.name];
    disk.system_disk = disk.name == system_disk;
    disk.has_swap = swap_disks.contains(disk.name);
  }
}

void DiskCollector::Update() {
  const auto now = std::chrono::steady_clock::now();
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(now - last_update_).count();
  last_update_ = now;

  if (++updates_since_refresh_ >= kLayoutRefreshInterval) {
    updates_since_refresh_ = 0;
    RefreshLayout();
  }

  const std::vector<DiskStat> stats =
      ParseDiskstats(ReadFile("/proc/diskstats"));
  for (DiskDevice& disk : disks_) {
    const auto it = std::find_if(
        stats.begin(), stats.end(),
        [&disk](const DiskStat& s) { return s.name == disk.name; });
    if (it == stats.end()) continue;
    const DiskStat& cur = *it;
    if (disk.has_previous && elapsed_ms > 0.0) {
      const DiskStat& prev = disk.previous;
      const auto delta = [](std::uint64_t now_value, std::uint64_t old) {
        return now_value >= old ? static_cast<double>(now_value - old) : 0.0;
      };
      const double seconds = elapsed_ms / 1000.0;
      disk.read_bytes_per_sec =
          delta(cur.sectors_read, prev.sectors_read) * kSectorSize / seconds;
      disk.write_bytes_per_sec =
          delta(cur.sectors_written, prev.sectors_written) * kSectorSize /
          seconds;
      disk.active_percent =
          std::min(100.0, 100.0 * delta(cur.io_ms, prev.io_ms) / elapsed_ms);
      const double ios =
          delta(cur.reads, prev.reads) + delta(cur.writes, prev.writes);
      const double io_time =
          delta(cur.read_ms, prev.read_ms) + delta(cur.write_ms, prev.write_ms);
      disk.response_ms = ios > 0.0 ? io_time / ios : 0.0;
    }
    disk.previous = cur;
    disk.has_previous = true;
    disk.active_history.Push(disk.active_percent);
    disk.read_history.Push(disk.read_bytes_per_sec);
    disk.write_history.Push(disk.write_bytes_per_sec);
  }
}

}  // namespace wtop
