#pragma once

#include <string>

struct SingleProcInfo
{
  int pid = -1;
  std::string name = "";
  long mem_usage_kb = 0;
  double cpu_usage_pct = 0.;
};
