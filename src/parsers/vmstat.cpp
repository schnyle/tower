#include <charconv>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "logger.hpp"
#include "parsers/file.hpp"
#include "parsers/vmstat.hpp"

std::optional<VmStat> VmStatParser::parse(std::istream &is)
{
  std::string line;
  VmStat vm_stat;
  while (std::getline(is, line))
  {
    const auto kv_line = parse_key_value_line(line, ' ');
    if (!kv_line)
    {
      continue;
    }

    const std::string_view key = kv_line->key;
    const std::string_view value = kv_line->value;
    if (key == "pgfault")
    {
      auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), vm_stat.pgfault);
      if (ec != std::errc{} || ptr != value.data() + value.size())
      {
        LOG_DEBUG("failed to parse pgfault from /proc/vmstat");
      }
    }
    else if (key == "pgmajfault")
    {
      auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), vm_stat.pgmajfault);
      if (ec != std::errc{} || ptr != value.data() + value.size())
      {
        LOG_DEBUG("failed to parse pgmajfault from /proc/vmstat");
      }
    }
  }

  return vm_stat;
}
