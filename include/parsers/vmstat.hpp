// https://www.man7.org/linux/man-pages/man5/proc_vmstat.5.html

#pragma once

#include <iosfwd>
#include <optional>
#include <string>

#include "parsers/parser.hpp"

struct VmStat
{
  long unsigned pgfault;
  long unsigned pgmajfault;
};

struct VmStatParser
{
  using Data = VmStat;
  static std::optional<Data> parse(std::istream &);
  static std::string path() { return "/proc/vmstat"; }
};

static_assert(SystemParser<VmStatParser>);
