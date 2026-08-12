#include <optional>
#include <unistd.h>

#include "raw_data/raw_data.hpp"

std::optional<RawData::CpuThreads> RawData::CpuThreads::collect() { return CpuThreads{sysconf(_SC_NPROCESSORS_ONLN)}; };
