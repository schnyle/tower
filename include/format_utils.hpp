#pragma once

#include <array>
#include <format>
#include <string>

inline std::string format_memory_size(long long kb)
{
  static const std::array<std::string, 3> UNITS{"K", "M", "G"};

  size_t units_i = 0;
  double kb_double = kb;

  while (kb_double >= 1024 && units_i < UNITS.size())
  {
    kb_double /= 1024;
    ++units_i;
  }

  if (kb_double >= 10)
  {
    return std::format("{:.0f}{}", kb_double, UNITS[units_i]);
  }
  else
  {
    return std::format("{:.1f}{}", kb_double, UNITS[units_i]);
  }
}

inline std::string format_percent(double pct)
{
  if (pct >= 10)
  {
    return std::format("{:.0f}%", pct);
  }
  else
  {
    return std::format("{:.1f}%", pct);
  }
}
