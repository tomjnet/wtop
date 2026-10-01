#ifndef WTOP_SRC_CORE_FORMAT_H_
#define WTOP_SRC_CORE_FORMAT_H_

#include <cstdint>
#include <string>

namespace wtop {

// Formats a byte count with binary multiples, e.g. "15.2 GB", "834 MB".
std::string FormatBytes(double bytes);

// Formats a byte rate with a KB/s floor, e.g. "0 KB/s", "208 KB/s".
std::string FormatBytesPerSec(double bytes_per_sec);

// Formats a bit rate with decimal multiples, e.g. "800 Kbps", "1.2 Mbps".
std::string FormatBitsPerSec(double bits_per_sec);

// Formats seconds as "D:HH:MM:SS", e.g. "6:17:47:15".
std::string FormatDuration(std::uint64_t seconds);

// Formats a frequency given in MHz, e.g. "3.70 GHz" or "800 MHz".
std::string FormatFrequency(double mhz);

// Formats a rounded percentage, e.g. "6%". NaN yields "N/A".
std::string FormatPercent(double percent);

// Formats a used/total pair sharing the unit of the total, e.g.
// "0.5/15.9 GB".
std::string FormatBytesPair(double used, double total);

// Formats a temperature in Celsius, e.g. "43 °C". NaN yields "N/A".
std::string FormatCelsius(double celsius);

// Returns the smallest value of the form {1, 2, 5} * 10^k that is >= value.
// Used to pick readable graph scales. Returns 1 for non-positive input.
double NiceCeiling(double value);

}  // namespace wtop

#endif  // WTOP_SRC_CORE_FORMAT_H_
