#pragma once

#include <cstdio>
#include <optional>

struct MemInfo
{
  long long mem_total_kb;
  long long mem_free_kb;
  long long mem_available_kb;
  long long swap_total_kb;
  long long swap_free_kb;

  void print() const
  {
    printf("MemTotal: %lld kB\n\r", mem_total_kb);
    printf("MemFree: %lld kB\n\r", mem_free_kb);
    printf("MemAvailable: %lld kB\n\r", mem_available_kb);
    printf("SwapTotal: %lld kB\n\r", swap_total_kb);
    printf("SwapFree: %lld kB\n\r", swap_free_kb);
  }
};

std::optional<MemInfo> get_proc_meminfo();
