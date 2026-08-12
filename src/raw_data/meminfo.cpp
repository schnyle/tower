#include <charconv>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"

long long sv_to_ll(std::string_view sv)
{
  long long value;
  std::from_chars(sv.data(), sv.data() + sv.size(), value);
  return value;
}

std::optional<RawData::MemInfo> RawData::MemInfo::parse(std::istream &is)
{
  std::string line;
  MemInfo meminfo;
  while (std::getline(is, line))
  {
    const auto kv_line = parse_key_value_line(line);
    if (!kv_line)
    {
      continue;
    }

    const std::string_view key = kv_line->key;
    const std::string_view value = kv_line->value;
    if (key == "MemTotal")
    {
      meminfo.mem_total_kb = sv_to_ll(value);
    }
    else if (key == "MemFree")
    {
      meminfo.mem_free_kb = sv_to_ll(value);
    }
    else if (key == "MemAvailable")
    {
      meminfo.mem_available_kb = sv_to_ll(value);
    }
    else if (key == "SwapTotal")
    {
      meminfo.swap_total_kb = sv_to_ll(value);
    }
    else if (key == "SwapFree")
    {
      meminfo.swap_free_kb = sv_to_ll(value);
    }
  }

  return meminfo;
}

std::optional<RawData::MemInfo> RawData::MemInfo::collect()
{
  auto file = open_file("/proc/meminfo");
  if (!file)
  {
    return std::nullopt;
  }
  return parse(*file);
}
