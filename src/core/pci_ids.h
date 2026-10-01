#ifndef WTOP_SRC_CORE_PCI_IDS_H_
#define WTOP_SRC_CORE_PCI_IDS_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace wtop {

struct PciName {
  std::string vendor;  // e.g. "Intel Corporation"
  std::string device;  // e.g. "Wi-Fi 6 AX200"
};

// Looks up a PCI vendor/device pair in the text of a pci.ids database.
PciName LookupPciName(std::string_view pci_ids_text, std::uint16_t vendor,
                      std::uint16_t device);

// Looks up a PCI device using the system pci.ids database, if installed.
PciName LookupPciName(std::uint16_t vendor, std::uint16_t device);

// Reads "vendor" and "device" of a sysfs PCI device directory and resolves
// a human readable name. Falls back to "vendor:device" hex ids.
std::string DescribePciDevice(const std::filesystem::path& device_dir);

}  // namespace wtop

#endif  // WTOP_SRC_CORE_PCI_IDS_H_
