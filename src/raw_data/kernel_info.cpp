#include <optional>
#include <sys/utsname.h>

#include "raw_data/raw_data.hpp"

std::optional<RawData::KernelInfo> RawData::KernelInfo::collect()
{
  struct utsname u;
  uname(&u);
  return KernelInfo{
      .sysname = u.sysname,
      .nodename = u.nodename,
      .release = u.release,
      .version = u.version,
      .machine = u.machine,
  };
};
