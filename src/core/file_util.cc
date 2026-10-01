#include "core/file_util.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

namespace wtop {

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

std::string ReadLine(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::string line;
  if (!file || !std::getline(file, line)) return {};
  return std::string(Trim(line));
}

std::optional<std::int64_t> ReadInt(const std::filesystem::path& path) {
  return ParseNumber<std::int64_t>(ReadLine(path));
}

std::vector<std::string> ListDir(const std::filesystem::path& path) {
  std::vector<std::string> names;
  std::error_code ec;
  for (std::filesystem::directory_iterator it(path, ec), end; !ec && it != end;
       it.increment(ec)) {
    names.push_back(it->path().filename().string());
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::string_view Trim(std::string_view text) {
  constexpr std::string_view kSpace = " \t\r\n\f\v";
  const auto begin = text.find_first_not_of(kSpace);
  if (begin == std::string_view::npos) return {};
  const auto end = text.find_last_not_of(kSpace);
  return text.substr(begin, end - begin + 1);
}

std::vector<std::string_view> SplitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  while (!text.empty()) {
    const auto pos = text.find('\n');
    lines.push_back(text.substr(0, pos));
    if (pos == std::string_view::npos) break;
    text.remove_prefix(pos + 1);
  }
  return lines;
}

std::vector<std::string_view> SplitWhitespace(std::string_view text) {
  std::vector<std::string_view> fields;
  std::size_t i = 0;
  while (i < text.size()) {
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    const std::size_t start = i;
    while (i < text.size() && text[i] != ' ' && text[i] != '\t') ++i;
    if (i > start) fields.push_back(text.substr(start, i - start));
  }
  return fields;
}

std::vector<std::string_view> Split(std::string_view text, char delimiter) {
  std::vector<std::string_view> parts;
  while (true) {
    const auto pos = text.find(delimiter);
    parts.push_back(text.substr(0, pos));
    if (pos == std::string_view::npos) break;
    text.remove_prefix(pos + 1);
  }
  return parts;
}

bool IsAllDigits(std::string_view text) {
  return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) {
    return c >= '0' && c <= '9';
  });
}

}  // namespace wtop
