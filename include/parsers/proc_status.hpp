#pragma once

#include <format>
#include <iosfwd>
#include <optional>

#include "parsers/parser.hpp"

struct ProcStatus
{
  long long vm_rss_kb = 0; // virtual memory resident set size
};

struct ProcStatusParser
{
  using Data = ProcStatus;
  static std::optional<Data> parse(std::istream &);
  static std::string path(int pid) { return std::format("/proc/{}/status", pid); }
};

static_assert(ProcParser<ProcStatusParser>);
