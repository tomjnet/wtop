#include "core/format.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string_view>
#include <utility>

namespace wtop {
namespace {

constexpr std::array<std::string_view, 6> kByteUnits = {"B",  "KB", "MB",
                                                        "GB", "TB", "PB"};
constexpr std::array<std::string_view, 5> kBitUnits = {"bps", "Kbps", "Mbps",
                                                       "Gbps", "Tbps"};

// Values >= 100 have no decimals, smaller ones keep a single decimal so the
// result matches the compact style of Task Manager ("834 MB", "15.2 GB").
std::string CompactNumber(double value) {
  if (value == 0.0) return "0";
  if (value >= 99.95) return std::format("{:.0f}", value);
  return std::format("{:.1f}", value);
}

// Scales `value` by `base` until it fits below 1000 of the chosen unit,
// starting at `min_unit`.
template <std::size_t N>
std::pair<double, std::size_t> Scale(
    double value, double base, std::size_t min_unit,
    const std::array<std::string_view, N>& units) {
  if (!std::isfinite(value) || value < 0.0) value = 0.0;
  std::size_t unit = 0;
  while (unit < min_unit) {
    value /= base;
    ++unit;
  }
  while (value >= 999.95 && unit + 1 < units.size()) {
    value /= base;
    ++unit;
  }
  return {value, unit};
}

}  // namespace

std::string FormatBytes(double bytes) {
  auto [value, unit] = Scale(bytes, 1024.0, 0, kByteUnits);
  if (unit == 0) return std::format("{:.0f} B", value);
  return std::format("{} {}", CompactNumber(value), kByteUnits[unit]);
}

std::string FormatBytesPerSec(double bytes_per_sec) {
  auto [value, unit] = Scale(bytes_per_sec, 1024.0, 1, kByteUnits);
  return std::format("{} {}/s", CompactNumber(value), kByteUnits[unit]);
}

std::string FormatBitsPerSec(double bits_per_sec) {
  auto [value, unit] = Scale(bits_per_sec, 1000.0, 1, kBitUnits);
  return std::format("{} {}", CompactNumber(value), kBitUnits[unit]);
}

std::string FormatDuration(std::uint64_t seconds) {
  const std::uint64_t days = seconds / 86400;
  const std::uint64_t hours = (seconds / 3600) % 24;
  const std::uint64_t minutes = (seconds / 60) % 60;
  const std::uint64_t secs = seconds % 60;
  return std::format("{}:{:02}:{:02}:{:02}", days, hours, minutes, secs);
}

std::string FormatFrequency(double mhz) {
  if (!std::isfinite(mhz) || mhz <= 0.0) return "N/A";
  if (mhz >= 1000.0) return std::format("{:.2f} GHz", mhz / 1000.0);
  return std::format("{:.0f} MHz", mhz);
}

std::string FormatPercent(double percent) {
  if (!std::isfinite(percent)) return "N/A";
  return std::format("{:.0f}%", percent);
}

std::string FormatBytesPair(double used, double total) {
  auto [total_value, unit] = Scale(total, 1024.0, 0, kByteUnits);
  const double used_value = std::max(0.0, used) / std::pow(1024.0, unit);
  const auto fmt = [](double v) {
    return v >= 99.95 ? std::format("{:.0f}", v) : std::format("{:.1f}", v);
  };
  return std::format("{}/{} {}", fmt(used_value), fmt(total_value),
                     kByteUnits[unit]);
}

std::string FormatCelsius(double celsius) {
  if (!std::isfinite(celsius)) return "N/A";
  return std::format("{:.0f} °C", celsius);
}

double NiceCeiling(double value) {
  if (!std::isfinite(value) || value <= 0.0) return 1.0;
  const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
  for (double step : {1.0, 2.0, 5.0, 10.0}) {
    // Small epsilon so exact powers (e.g. 100) map onto themselves.
    if (step * magnitude >= value * (1.0 - 1e-9)) return step * magnitude;
  }
  return 10.0 * magnitude;
}

}  // namespace wtop
