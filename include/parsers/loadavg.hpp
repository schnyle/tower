#pragma once

#include <cstdio>
#include <iosfwd>
#include <optional>

struct LoadAvg
{
  double load_avg_1_min;
  double load_avg_5_min;
  double load_avg_15_min;
  int runnable_procs;
  int total_procs;
  int last_pid_created;

  void print() const
  {
    printf("Load avg (1 min): %.2f\n\r", load_avg_1_min);
    printf("Load avg (5 min): %.2f\n\r", load_avg_5_min);
    printf("Load avg (15 min): %.2f\n\r", load_avg_15_min);
    printf("Runnable procs: %d\n\r", runnable_procs);
    printf("Total procs: %d\n\r", total_procs);
    printf("Last PID created: %d\n\r", last_pid_created);
  }
};

struct LoadAvgParser
{
  using Data = LoadAvg;
  static std::optional<Data> parse(std::istream &);
};
