// https://www.man7.org/linux/man-pages/man5/proc_pid_stat.5.html

#pragma once

#include <iosfwd>
#include <optional>

struct ProcStat
{
  long unsigned minflt = 0;
  long unsigned cminflt = 0;
  long unsigned majflt = 0;
  long unsigned cmajflt = 0;
  long unsigned utime = 0;
  long unsigned stime = 0;
};

struct ProcStatParser
{
  using Data = ProcStat;
  static std::optional<Data> parse(std::istream &);
};
