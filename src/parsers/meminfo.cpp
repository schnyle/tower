#include <charconv>
#include <cstdio>
#include <istream>
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "parsers/meminfo.hpp"

struct MemInfoItem
{
  std::string_view key;
  long long value;
};

std::optional<MemInfoItem> parse_meminfo_line(std::string_view sv)
{
  const size_t pos = sv.find(':');
  if (pos == std::string_view::npos)
  {
    return std::nullopt;
  }

  const std::string_view key = sv.substr(0, pos);

  std::string_view value_sv = sv.substr(pos + 1);
  const size_t start = value_sv.find_first_not_of(" \t");
  if (start == std::string_view::npos)
  {
    return std::nullopt;
  }
  value_sv.remove_prefix(start);

  long long value = 0;
  if (std::from_chars(value_sv.data(), value_sv.data() + value_sv.size(), value).ec != std::errc{})
  {
    return std::nullopt;
  }

  return MemInfoItem{key, value};
}

std::optional<MemInfo> MemInfoParser::parse(std::istream &is)
{
  static const std::set<std::string> KEYS = {"MemTotal", "MemFree", "MemAvailable", "SwapTotal", "SwapFree"};

  std::string line;
  MemInfo meminfo;
  while (std::getline(is, line))
  {
    const auto item = parse_meminfo_line(line);
    if (!item)
    {
      continue;
    }

    const auto &[key, value] = *item;
    if (key == "MemTotal")
    {
      meminfo.mem_total_kb = value;
    }
    else if (key == "MemFree")
    {
      meminfo.mem_free_kb = value;
    }
    else if (key == "MemAvailable")
    {
      meminfo.mem_available_kb = value;
    }
    else if (key == "SwapTotal")
    {
      meminfo.swap_total_kb = value;
    }
    else if (key == "SwapFree")
    {
      meminfo.swap_free_kb = value;
    }
  }

  return meminfo;
}
