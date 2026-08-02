#include <charconv>
#include <istream>
#include <optional>

#include "logger.hpp"
#include "parsers/file.hpp"
#include "parsers/proc_stat.hpp"

std::optional<ProcStat> ProcStatParser::parse(std::istream &is)
{
  std::string line;
  if (!std::getline(is, line))
  {
    return std::nullopt;
  }

  std::string_view line_sv = line;

  // /proc/<PID>/stat is a single line with space separated tokens.
  // The first token is the PID.
  // The second token is '(<process name>)', which may have multiple white spaces inside.
  // Hence, we jump to the first ')' character, then process tokens from there.
  const auto pos = line_sv.rfind(')');
  if (pos == std::string_view::npos)
  {
    return std::nullopt;
  }

  line_sv.remove_prefix(pos + 1);

  std::string_view tok;
  for (size_t i = 0; i <= 10; ++i)
  {
    tok = parse_next_token(line_sv);
  }

  std::from_chars_result result;

  long long utime;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), utime);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse utime from /proc/<PID>/stat");
  }

  long long stime;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), stime);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse stime from /proc/<PID>/stat");
  }

  return ProcStat{.utime = utime, .stime = stime};
}
