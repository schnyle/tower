#include <charconv>
#include <istream>
#include <string_view>

#include "logger.hpp"
#include "parsers/cpuinfo.hpp"
#include "parsers/file.hpp"

std::optional<CpuInfo> CpuInfoParser::parse(std::istream &is)
{
  bool got_model_name = false;
  bool got_cpu_cores = false;
  std::string line;
  CpuInfo cpu_info;
  while (std::getline(is, line))
  {
    const auto kv_line = parse_key_value_line(line);
    if (!kv_line)
    {
      continue;
    }

    const std::string_view key = kv_line->key;
    const std::string_view value = kv_line->value;
    if (key == "model name")
    {
      cpu_info.model_name = value;
      got_model_name = true;
    }
    else if (key == "cpu cores")
    {
      auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), cpu_info.cpu_cores);
      if (ec != std::errc{})
      {
        LOG_DEBUG("failed to parse cpu cores from /proc/cpuinfo");
        continue;
      }
      got_cpu_cores = true;
    }

    if (got_model_name && got_cpu_cores)
    {
      return cpu_info;
    }
  }

  return std::nullopt;
}
