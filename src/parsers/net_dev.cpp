#include <charconv>
#include <istream>
#include <string>
#include <string_view>

#include "parsers/file.hpp"
#include "parsers/net_dev.hpp"

std::optional<NetDev> NetDevParser::parse(std::istream &is)
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
