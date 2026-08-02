#pragma once

#include <iosfwd>
#include <optional>

struct ProcStat
{
  long long utime = 0;
  long long stime = 0;
};

struct ProcStatParser
{
  using Data = ProcStat;
  static std::optional<Data> parse(std::istream &);
};
