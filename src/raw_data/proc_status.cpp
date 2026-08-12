#include <charconv>
#include <format>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"

std::optional<RawData::ProcStatus> RawData::ProcStatus::parse(std::istream &is)
{
  std::string line;
  ProcStatus proc_status;
  while (std::getline(is, line))
  {
    const auto kv_line = parse_key_value_line(line);
    if (!kv_line)
    {
      continue;
    }

    const std::string_view key = kv_line->key;
    const std::string_view value = kv_line->value;
    if (key == "VmRSS")
    {
      std::from_chars(value.data(), value.data() + value.size(), proc_status.vm_rss_kb);
    }
  }

  return proc_status;
}

std::optional<RawData::ProcStatus> RawData::ProcStatus::collect(int pid)
{
  auto file = open_file(std::format("/proc/{}/status", pid));
  if (!file)
  {
    return std::nullopt;
  }
  return parse(*file);
}
