#include <charconv>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"

std::optional<RawData::NetDev> RawData::NetDev::parse(std::istream &is)
{
  std::string line;
  std::string_view key, tok;
  long long rx = 0;
  long long tx = 0;
  NetDev net_dev;
  while (std::getline(is, line))
  {
    std::string_view line_sv(line);
    key = parse_next_token(line_sv);
    if (key.empty() || key.find(':') == std::string_view::npos || key == "lo:")
    {
      continue;
    }

    tok = parse_next_token(line_sv);
    std::from_chars(tok.data(), tok.data() + tok.size(), rx);
    net_dev.rx_bytes += rx;

    for (int i = 0; i < 8; ++i)
    {
      tok = parse_next_token(line_sv);
    }

    std::from_chars(tok.data(), tok.data() + tok.size(), tx);
    net_dev.tx_bytes += tx;
  }

  return net_dev;
}

std::optional<RawData::NetDev> RawData::NetDev::collect()
{
  auto file = open_file("/proc/net/dev");
  if (!file)
  {
    return std::nullopt;
  }
  return parse(*file);
}
