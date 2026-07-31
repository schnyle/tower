#include <charconv>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "parsers/file.hpp"
#include "parsers/stat.hpp"

std::string_view next_token(std::string_view &sv)
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

std::optional<Stat> StatParser::parse(std::istream &is)
{
  std::string line;
  std::string_view key, tok;
  Stat stat;
  while (std::getline(is, line))
  {
    std::string_view line_sv(line);
    key = next_token(line_sv);
    if (key.empty())
    {
      continue;
    }

    if (key == "cpu")
    {
      long *fields[] = {
          &stat.cpu_times.user,
          &stat.cpu_times.nice,
          &stat.cpu_times.system,
          &stat.cpu_times.idle,
          &stat.cpu_times.iowait,
          &stat.cpu_times.irq,
          &stat.cpu_times.softirq,
          &stat.cpu_times.steal,
          &stat.cpu_times.guest,
          &stat.cpu_times.guest_nice,
      };

      for (long *field : fields)
      {
        const std::string_view tok = next_token(line_sv);
        if (tok.empty())
        {
          break;
        }
        std::from_chars(tok.data(), tok.data() + tok.size(), *field);
      }
    }
    else if (key == "intr")
    {
      tok = next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.interrupts_serviced_count);
      }
    }
    else if (key == "ctxt")
    {
      tok = next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.context_switches_count);
      }
    }
    else if (key == "btime")
    {
      tok = next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.boot_time);
      }
    }
    else if (key == "procs_running")
    {
      tok = next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.runnable_procs_count);
      }
    }
    else if (key == "procs_blocked")
    {
      tok = next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.blocked_procs_count);
      }
    }
  }

  return stat;
}
