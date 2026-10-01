#ifndef WTOP_SRC_CORE_FILE_UTIL_H_
#define WTOP_SRC_CORE_FILE_UTIL_H_

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wtop {

// Reads the whole file. Returns an empty string when it cannot be read.
// Works for procfs/sysfs files, which report a size of zero.
std::string ReadFile(const std::filesystem::path& path);

// Reads the first line of a file with surrounding whitespace removed.
std::string ReadLine(const std::filesystem::path& path);

// Reads a file holding a single integer (common in sysfs).
std::optional<std::int64_t> ReadInt(const std::filesystem::path& path);

// Returns the entry names of a directory, or an empty list on error.
std::vector<std::string> ListDir(const std::filesystem::path& path);

std::string_view Trim(std::string_view text);
std::vector<std::string_view> SplitLines(std::string_view text);
std::vector<std::string_view> SplitWhitespace(std::string_view text);
std::vector<std::string_view> Split(std::string_view text, char delimiter);

// Returns true when `text` is non-empty and only holds ASCII digits.
bool IsAllDigits(std::string_view text);

// Parses a number that must span all of `text` (after trimming).
template <typename T>
std::optional<T> ParseNumber(std::string_view text) {
  text = Trim(text);
  T value{};
  const char* end = text.data() + text.size();
  auto [ptr, ec] = std::from_chars(text.data(), end, value);
  if (ec != std::errc() || ptr != end || text.empty()) return std::nullopt;
  return value;
}

}  // namespace wtop

#endif  // WTOP_SRC_CORE_FILE_UTIL_H_
