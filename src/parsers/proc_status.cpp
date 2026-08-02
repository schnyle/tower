#include <charconv>
#include <istream>
#include <optional>

#include "parsers/file.hpp"
#include "parsers/proc_status.hpp"

std::optional<ProcStatus> ProcStatusParser::parse(std::istream &is)
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
