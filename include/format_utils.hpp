#pragma once

#include <array>
#include <format>
#include <string>

inline std::string format_bytes_si(long long bytes)
{
  static const std::array<std::string, 4> UNITS{"B", "kB", "MB", "GB"};

  size_t units_i = 0;
  double bytes_double = bytes;

  while (bytes_double >= 1000 && units_i + 1 < UNITS.size())
  {
    bytes_double /= 1000;
    ++units_i;
  }

  if (bytes_double >= 10)
  {
    return std::format("{:.0f} {}", bytes_double, UNITS[units_i]);
  }
  else
  {
    return std::format("{:.1f} {}", bytes_double, UNITS[units_i]);
  }
}

inline std::string format_bytes_iec(long long bytes)
{
  static const std::array<std::string, 4> UNITS{"B", "KiB", "MiB", "GiB"};

  size_t units_i = 0;
  double bytes_double = bytes;

  while (bytes_double >= 1024 && units_i + 1 < UNITS.size())
  {
    bytes_double /= 1024;
    ++units_i;
  }

  if (bytes_double >= 10)
  {
    return std::format("{:.0f} {}", bytes_double, UNITS[units_i]);
  }
  else
  {
    return std::format("{:.1f} {}", bytes_double, UNITS[units_i]);
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
