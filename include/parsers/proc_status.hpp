#pragma once

#include <iosfwd>
#include <optional>

struct ProcStatus
{
  long long vm_rss_kb = 0; // virtual memory resident set size
};

struct ProcStatusParser
{
  using Data = ProcStatus;
  static std::optional<Data> parse(std::istream &);
};
