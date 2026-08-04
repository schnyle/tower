#pragma once

#include <iosfwd>
#include <optional>
#include <string>

struct CpuInfo
{
  std::string model_name;
  int cpu_cores;
};

struct CpuInfoParser
{
  using Data = CpuInfo;
  static std::optional<Data> parse(std::istream &);
};
