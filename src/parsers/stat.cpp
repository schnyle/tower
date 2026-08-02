#include <charconv>
#include <istream>
#include <optional>
#include <string>
#include <string_view>

#include "parsers/file.hpp"
#include "parsers/stat.hpp"

std::optional<Stat> StatParser::parse(std::istream &is)
{
  std::string line;
  std::string_view key, tok;
  Stat stat;
  while (std::getline(is, line))
  {
    std::string_view line_sv(line);
    key = parse_next_token(line_sv);
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
        const std::string_view tok = parse_next_token(line_sv);
        if (tok.empty())
        {
          break;
        }
        std::from_chars(tok.data(), tok.data() + tok.size(), *field);
      }
    }
    else if (key == "intr")
    {
      tok = parse_next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.interrupts_serviced_count);
      }
    }
    else if (key == "ctxt")
    {
      tok = parse_next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.context_switches_count);
      }
    }
    else if (key == "btime")
    {
      tok = parse_next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.boot_time);
      }
    }
    else if (key == "procs_running")
    {
      tok = parse_next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.runnable_procs_count);
      }
    }
    else if (key == "procs_blocked")
    {
      tok = parse_next_token(line_sv);
      if (!tok.empty())
      {
        std::from_chars(tok.data(), tok.data() + tok.size(), stat.blocked_procs_count);
      }
    }
  }

  return stat;
}
