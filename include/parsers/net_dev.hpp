#pragma once

#include <cstdio>
#include <iosfwd>
#include <optional>

struct NetDev
{
  long long rx_bytes = 0;
  long long tx_bytes = 0;

  void print() const
  {
    printf("RX: %lld B\n\r", rx_bytes);
    printf("TX: %lld B\n\r", tx_bytes);
  }
};

struct NetDevParser
{
  using Data = NetDev;
  static std::optional<Data> parse(std::istream &);
};
