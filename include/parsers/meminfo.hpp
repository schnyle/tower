#pragma once

#include <cstdio>
#include <iosfwd>
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
    printf("MemTotal: %.2f gB\n\r", (double)mem_total_kb / 1024 / 1024);
    printf("MemFree: %.2f gB\n\r", (double)mem_free_kb / 1024 / 1024);
    printf("MemAvailable: %.2f gB\n\r", (double)mem_available_kb / 1024 / 1024);
    printf("SwapTotal: %.2f gB\n\r", (double)swap_total_kb / 1024 / 1024);
    printf("SwapFree: %.2f gB\n\r", (double)swap_free_kb / 1024 / 1024);
  }
};

struct MemInfoParser
{
  using Data = MemInfo;
  static std::optional<Data> parse(std::istream &);
};
