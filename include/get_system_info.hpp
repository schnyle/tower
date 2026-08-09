#pragma once

#include <string>
#include <sys/utsname.h>
#include <unistd.h>

#include "collect.hpp"
#include "parsers/cpuinfo.hpp"

struct SystemInfo
{
  std::string sysname = "";
  std::string nodename = "";
  std::string release = "";
  std::string version = "";
  std::string machine = "";
  std::string cpu_model = "";
  int cpu_cores = 0;
  int cpu_threads = 0;
};

inline SystemInfo get_system_info()
{
  struct SystemInfo system_info;

  struct utsname u;
  uname(&u);

  system_info.sysname = u.sysname;
  system_info.nodename = u.nodename;
  system_info.release = u.release;
  system_info.version = u.version;
  system_info.machine = u.machine;

  if (const auto cpu_info = collect<CpuInfoParser>())
  {
    system_info.cpu_model = cpu_info->value.model_name;
    system_info.cpu_cores = cpu_info->value.cpu_cores;
  };

  system_info.cpu_threads = sysconf(_SC_NPROCESSORS_ONLN);

  return system_info;
}
