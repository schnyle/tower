#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <variant>

template <typename T>
concept SystemRawData = requires() {
  { T::collect() } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept ProcessRawData = requires(int pid) {
  { T::collect(pid) } -> std::same_as<std::optional<T>>;
};

namespace RawData
{

struct ClockTick
{
  long value;

  static std::optional<ClockTick> collect();
};

struct CpuInfo
{
  std::string model_name;
  int cpu_cores;

  static std::optional<CpuInfo> collect();
  static std::optional<CpuInfo> parse(std::istream &);
};

struct CpuThreads
{
  long value;

  static std::optional<CpuThreads> collect();
};

struct KernelInfo
{
  std::string sysname = "";
  std::string nodename = "";
  std::string release = "";
  std::string version = "";
  std::string machine = "";

  static std::optional<KernelInfo> collect();
};

struct LoadAvg
{
  double load_avg_1_min;
  double load_avg_5_min;
  double load_avg_15_min;
  int runnable_procs;
  int total_procs;
  int last_pid_created;

  static std::optional<LoadAvg> collect();
  static std::optional<LoadAvg> parse(std::istream &is);
};

struct MemInfo
{
  long long mem_total_kb;
  long long mem_free_kb;
  long long mem_available_kb;
  long long swap_total_kb;
  long long swap_free_kb;

  static std::optional<MemInfo> collect();
  static std::optional<MemInfo> parse(std::istream &is);
};

struct NetDev
{
  long long rx_bytes = 0;
  long long tx_bytes = 0;

  static std::optional<NetDev> collect();
  static std::optional<NetDev> parse(std::istream &is);
};

struct Stat
{
  struct CpuTimes
  {
    long user = -1;
    long nice = -1;
    long system = -1;
    long idle = -1;
    long iowait = -1;
    long irq = -1;
    long softirq = -1;
    long steal = -1;
    long guest = -1;
    long guest_nice = -1;
  } cpu_times;
  long interrupts_serviced_count;
  long context_switches_count;
  long boot_time;
  long runnable_procs_count;
  long blocked_procs_count;

  static std::optional<Stat> collect();
  static std::optional<Stat> parse(std::istream &is);
};

Stat::CpuTimes operator-(const Stat::CpuTimes &, const Stat::CpuTimes &);

struct VmStat
{
  long unsigned pgfault;
  long unsigned pgmajfault;

  static std::optional<VmStat> collect();
  static std::optional<VmStat> parse(std::istream &is);
};

struct ProcStat
{
  long unsigned minflt = 0;
  long unsigned cminflt = 0;
  long unsigned majflt = 0;
  long unsigned cmajflt = 0;
  long unsigned utime = 0;
  long unsigned stime = 0;

  static std::optional<ProcStat> collect(int pid);
  static std::optional<ProcStat> parse(std::istream &is);
};

struct ProcStatus
{
  long long vm_rss_kb = 0; // virtual memory resident set size

  static std::optional<ProcStatus> collect(int pid);
  static std::optional<ProcStatus> parse(std::istream &is);
};

using Any = std::
    variant<ClockTick, CpuThreads, CpuInfo, KernelInfo, LoadAvg, MemInfo, NetDev, Stat, VmStat, ProcStat, ProcStatus>;

} // namespace RawData

static_assert(SystemRawData<RawData::ClockTick>);
static_assert(SystemRawData<RawData::CpuInfo>);
static_assert(SystemRawData<RawData::CpuThreads>);
static_assert(SystemRawData<RawData::KernelInfo>);
static_assert(SystemRawData<RawData::LoadAvg>);
static_assert(SystemRawData<RawData::MemInfo>);
static_assert(SystemRawData<RawData::NetDev>);
static_assert(SystemRawData<RawData::Stat>);
static_assert(SystemRawData<RawData::VmStat>);
static_assert(ProcessRawData<RawData::ProcStat>);
static_assert(ProcessRawData<RawData::ProcStatus>);
