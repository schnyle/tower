#pragma once

#include <cstdio>
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

std::optional<NetDev> get_proc_net_dev();
