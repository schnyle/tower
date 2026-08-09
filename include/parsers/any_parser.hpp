#pragma once

#include <variant>

#include "parsers/cpuinfo.hpp"
#include "parsers/loadavg.hpp"
#include "parsers/meminfo.hpp"
#include "parsers/net_dev.hpp"
#include "parsers/proc_stat.hpp"
#include "parsers/proc_status.hpp"
#include "parsers/stat.hpp"
#include "parsers/vmstat.hpp"

using AnyParser = std::variant<
    StatParser,
    MemInfoParser,
    NetDevParser,
    VmStatParser,
    CpuInfoParser,
    LoadAvgParser,
    ProcStatParser,
    ProcStatusParser>;

using AnySystemParser =
    std::variant<StatParser, MemInfoParser, NetDevParser, VmStatParser, CpuInfoParser, LoadAvgParser>;

using AnyProcParser = std::variant<ProcStatParser, ProcStatusParser>;
