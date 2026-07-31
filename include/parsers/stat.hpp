#pragma once

#include <cstdio>
#include <istream>
#include <optional>

struct CpuTimes
{
  long user = -1;
  long nice = -1;
  long system = -1;
  long idle = -1;
  long iowait = -1;
  long irq = -1;
  long softirq = -1;
  long steal = -1;
  long guest = -1;
  long guest_nice = -1;
};

inline CpuTimes operator-(const CpuTimes &lhs, const CpuTimes &rhs)
{
  CpuTimes res;
  res.user = lhs.user - rhs.user;
  res.nice = lhs.nice - rhs.nice;
  res.system = lhs.system - rhs.system;
  res.idle = lhs.idle - rhs.idle;
  res.iowait = lhs.iowait - rhs.iowait;
  res.irq = lhs.irq - rhs.irq;
  res.softirq = lhs.softirq - rhs.softirq;
  res.steal = lhs.steal - rhs.steal;
  res.guest = lhs.guest - rhs.guest;
  res.guest_nice = lhs.guest_nice - rhs.guest_nice;

  return res;
}

struct Stat
{
  CpuTimes cpu_times;
  long interrupts_serviced_count;
  long context_switches_count;
  long boot_time;
  long runnable_procs_count;
  long blocked_procs_count;

  void print() const
  {
    printf("CPU UserTime: %ld\n\r", cpu_times.user);
    printf("Interrupts serviced: %ld\n\r", interrupts_serviced_count);
    printf("Context switches: %ld\n\r", context_switches_count);
    printf("Boot time: %ld\n\r", boot_time);
    printf("Runnable proccesses: %ld\n\r", runnable_procs_count);
    printf("Blocked processed: %ld\n\r", blocked_procs_count);
  };
};

struct StatParser
{
  using Data = Stat;
  static std::optional<Data> parse(std::istream &);
};
