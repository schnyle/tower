#include <charconv>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "net_dev.hpp"

// duplicate in stat.cpp
inline std::string_view next_token(std::string_view &sv)
{
  const size_t start = sv.find_first_not_of(' ');
  if (start == std::string_view::npos)
  {
    sv = {};
    return sv;
  }

  sv.remove_prefix(start);

  const size_t end = sv.find_first_of(' ');
  const std::string_view token = sv.substr(0, end);
  sv.remove_prefix(end == std::string_view::npos ? sv.size() : end);

  return token;
}

std::optional<NetDev> get_proc_net_dev()
{
  static const std::string NET_DEV_PATH = "/proc/net/dev";

  std::ifstream file(NET_DEV_PATH);
  if (!file)
  {
    std::cerr << "failed to open file " << NET_DEV_PATH << "\n";
    return std::nullopt;
  }

  std::string line;
  std::string_view key, tok;
  long long rx = 0;
  long long tx = 0;
  NetDev net_dev;
  while (std::getline(file, line))
  {
    std::string_view line_sv(line);
    key = next_token(line_sv);
    if (key.empty() || key.find(':') == std::string_view::npos || key == "lo:")
    {
      continue;
    }

    tok = next_token(line_sv);
    std::from_chars(tok.data(), tok.data() + tok.size(), rx);
    net_dev.rx_bytes += rx;

    for (int i = 0; i < 8; ++i)
    {
      tok = next_token(line_sv);
    }

    std::from_chars(tok.data(), tok.data() + tok.size(), tx);
    net_dev.tx_bytes += tx;
  }

  return net_dev;
}
