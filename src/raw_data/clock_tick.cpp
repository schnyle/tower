#include <optional>
#include <unistd.h>

#include "raw_data/raw_data.hpp"

std::optional<RawData::ClockTick> RawData::ClockTick::collect() { return ClockTick{sysconf(_SC_CLK_TCK)}; };
