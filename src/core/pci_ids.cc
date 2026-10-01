#include "core/pci_ids.h"

#include <array>
#include <format>
#include <optional>

#include "core/file_util.h"

namespace wtop {
namespace {

constexpr std::array<const char*, 3> kPciIdsPaths = {
    "/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids",
    "/usr/share/pci.ids"};

std::optional<std::uint16_t> ParseHex16(std::string_view text) {
  std::uint16_t value = 0;
  const char* end = text.data() + text.size();
  auto [ptr, ec] = std::from_chars(text.data(), end, value, 16);
  if (ec != std::errc() || ptr != end) return std::nullopt;
  return value;
}

std::optional<std::uint16_t> ReadHexFile(const std::filesystem::path& path) {
  const std::string line = ReadLine(path);
  std::string_view text = line;
  if (text.starts_with("0x")) text.remove_prefix(2);
  return ParseHex16(text);
}

}  // namespace

PciName LookupPciName(std::string_view pci_ids_text, std::uint16_t vendor,
                      std::uint16_t device) {
  PciName result;
  bool in_vendor = false;
  for (std::string_view line : SplitLines(pci_ids_text)) {
    if (line.empty() || line[0] == '#') continue;
    if (line[0] != '\t') {
      if (in_vendor) break;  // Left the vendor block without a device match.
      // Device classes follow all vendors; nothing more to find.
      if (line.starts_with("C ")) break;
      if (line.size() > 6 && ParseHex16(line.substr(0, 4)) == vendor) {
        in_vendor = true;
        result.vendor = std::string(Trim(line.substr(4)));
      }
      continue;
    }
    if (!in_vendor || line.size() < 7 || line[1] == '\t') continue;
    if (ParseHex16(line.substr(1, 4)) == device) {
      result.device = std::string(Trim(line.substr(5)));
      break;
    }
  }
  return result;
}

PciName LookupPciName(std::uint16_t vendor, std::uint16_t device) {
  for (const char* path : kPciIdsPaths) {
    const std::string text = ReadFile(path);
    if (!text.empty()) return LookupPciName(text, vendor, device);
  }
  return {};
}

std::string DescribePciDevice(const std::filesystem::path& device_dir) {
  const auto vendor = ReadHexFile(device_dir / "vendor");
  const auto device = ReadHexFile(device_dir / "device");
  if (!vendor || !device) return {};
  const PciName name = LookupPciName(*vendor, *device);
  if (!name.device.empty()) {
    // Many entries carry the marketing name in brackets, which reads best.
    const auto open = name.device.find('[');
    const auto close = name.device.rfind(']');
    if (open != std::string::npos && close != std::string::npos &&
        close > open) {
      return name.device.substr(open + 1, close - open - 1);
    }
    return name.vendor.empty() ? name.device : name.vendor + " " + name.device;
  }
  return std::format("PCI device {:04x}:{:04x}", *vendor, *device);
}

}  // namespace wtop
