#pragma once

#include <cstdio>
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

std::optional<Stat> get_proc_stat();
