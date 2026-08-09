#pragma once

#include <iosfwd>
#include <optional>
#include <string>

#include "parsers/parser.hpp"

struct CpuInfo
{
  std::string model_name;
  int cpu_cores;
};

struct CpuInfoParser
{
  using Data = CpuInfo;
  static std::optional<Data> parse(std::istream &);
  static std::string path() { return "/proc/cpuinfo"; }
};

static_assert(SystemParser<CpuInfoParser>);
