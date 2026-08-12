// https://www.man7.org/linux/man-pages/man5/proc_pid_stat.5.html

#include <charconv>
#include <format>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "logger.hpp"
#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"

std::optional<RawData::ProcStat> RawData::ProcStat::parse(std::istream &is)
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
  for (size_t i = 0; i <= 6; ++i)
  {
    tok = parse_next_token(line_sv);
  }

  std::from_chars_result result;

  long unsigned minflt;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), minflt);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse minflt from /proc/<PID>/stat");
  }

  long unsigned cminflt;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), cminflt);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse cminflt from /proc/<PID>/stat");
  }

  long unsigned majflt;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), majflt);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse majflt from /proc/<PID>/stat");
  }

  long unsigned cmajflt;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), cmajflt);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse cmajflt from /proc/<PID>/stat");
  }

  long unsigned utime;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), utime);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse utime from /proc/<PID>/stat");
  }

  long unsigned stime;
  tok = parse_next_token(line_sv);
  result = std::from_chars(tok.data(), tok.data() + tok.size(), stime);
  if (result.ec != std::errc{} || result.ptr != tok.data() + tok.size())
  {
    LOG_DEBUG("failed to parse stime from /proc/<PID>/stat");
  }

  return ProcStat{
      .minflt = minflt, .cminflt = cminflt, .majflt = majflt, .cmajflt = cmajflt, .utime = utime, .stime = stime};
}

std::optional<RawData::ProcStat> RawData::ProcStat::collect(int pid)
{
  auto file = open_file(std::format("/proc/{}/stat", pid));
  if (!file)
  {
    return std::nullopt;
  }
  return parse(*file);
}
