#pragma once

#include <format>
#include <optional>
#include <string>
#include <vector>

#include "canvas.hpp"
#include "format_utils.hpp"
#include "raw_data/raw_data.hpp"
#include "raw_data/raw_data_cache.hpp"
#include "ring_buffer.hpp"

template <typename T> class DeltaTracker
{
public:
  std::optional<T> delta(T current_value)
  {
    std::optional<T> delta;
    if (previous_)
    {
      delta = current_value - *previous_;
    }
    previous_ = current_value;
    return delta;
  }

  void reset() { previous_ = std::nullopt; }

private:
  std::optional<T> previous_;
};

class BarPlotMetric
{
public:
  BarPlotMetric(std::string name, Color color, const RawDataCache &raw_data_cache)
      : name_(name), color_(color), raw_data_cache_(raw_data_cache) {};
  virtual ~BarPlotMetric() = default;

  virtual double initial_ymin() const = 0;
  virtual double initial_ymax() const = 0;
  virtual bool dynamic_ymax() const { return false; }
  virtual std::vector<RawData::Any> required_raw_data() const = 0;
  virtual void update() = 0;
  virtual void reset() {}
  virtual std::string format_value(const double value) const = 0;

  const std::string &name() const { return name_; }
  Color color() const { return color_; }
  RingBuffer<double> &rb() { return rb_; }

protected:
  const RawDataCache &raw_data_cache() const { return raw_data_cache_; }

private:
  std::string name_;
  const Color color_;
  RingBuffer<double> rb_{50000};
  const RawDataCache &raw_data_cache_;
};

class CpuLoadMetric : public BarPlotMetric
{
public:
  CpuLoadMetric(const RawDataCache &raw_data_cache) : BarPlotMetric("CPU Load", Color::red(), raw_data_cache) {}

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 100.; }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::Stat{}}; }

  void update() override
  {
    const auto stat = raw_data_cache().get<RawData::Stat>();
    if (!stat)
    {
      rb().repeat_last();
      return;
    }

    const auto cpu_times_delta = cpu_times_delta_tracker_.delta(stat->cpu_times);
    if (!cpu_times_delta)
    {
      rb().repeat_last();
      return;
    }

    long busy_time = cpu_times_delta->user + cpu_times_delta->nice + cpu_times_delta->system + cpu_times_delta->irq +
                     cpu_times_delta->softirq + cpu_times_delta->steal;
    long idle_time = cpu_times_delta->idle + cpu_times_delta->iowait;
    double usage = static_cast<double>(busy_time) / static_cast<double>(busy_time + idle_time) * 100;
    rb().push(usage);
  }

  std::string format_value(const double value) const override { return std::format("{:.1f}%", value); };

private:
  DeltaTracker<RawData::Stat::CpuTimes> cpu_times_delta_tracker_;
};

class AvailableMemoryMetric : public BarPlotMetric
{
public:
  AvailableMemoryMetric(const RawDataCache &raw_data_cache)
      : BarPlotMetric("Available Memory", Color::green(), raw_data_cache)
  {
  }

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override
  {
    // TODO: error handling
    return raw_data_cache().get<RawData::MemInfo>()->mem_total_kb;
  }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::MemInfo{}}; }

  void update() override
  {
    const auto &mem_info = raw_data_cache().get<RawData::MemInfo>();
    if (!mem_info)
    {
      rb().repeat_last();
      return;
    }

    rb().push(mem_info->mem_available_kb);
  }

  std::string format_value(const double value) const override { return format_bytes_iec(value * 1024); };
};

class MajorPageFaultsMetric : public BarPlotMetric
{
public:
  MajorPageFaultsMetric(const RawDataCache &raw_data_cache)
      : BarPlotMetric("Major Page Faults", Color(100, 50, 200), raw_data_cache)
  {
  }

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 100.; }
  bool dynamic_ymax() const override { return true; }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::VmStat{}}; }

  void update() override
  {
    const auto &vm_stat = raw_data_cache().get<RawData::VmStat>();
    const auto &elapsed = raw_data_cache().elapsed<RawData::VmStat>();
    if (!vm_stat || !elapsed)
    {
      rb().repeat_last();
      return;
    }

    const auto &delta = major_page_faults_delta_tracker_.delta(vm_stat->pgmajfault);
    if (!delta)
    {
      rb().repeat_last();
      return;
    }

    const double rate = *delta / elapsed->count();
    rb().push(rate);
  }

  std::string format_value(const double value) const override { return std::format("{:.1f} faults/sec", value); }

private:
  DeltaTracker<unsigned long> major_page_faults_delta_tracker_;
};

class DownloadMetric : public BarPlotMetric
{
public:
  DownloadMetric(const RawDataCache &raw_data_cache) : BarPlotMetric("Download", Color::blue(), raw_data_cache) {}

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 10000.; }
  bool dynamic_ymax() const override { return true; }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::NetDev{}}; }

  void update() override
  {
    const auto &net_dev = raw_data_cache().get<RawData::NetDev>();
    if (!net_dev)
    {
      rb().repeat_last();
      return;
    }

    const auto &rx_delta = rx_bytes_delta_tracker_.delta(net_dev->rx_bytes);
    const auto &elapsed = raw_data_cache().elapsed<RawData::NetDev>();
    if (!rx_delta || !elapsed)
    {
      rb().repeat_last();
      return;
    }

    const double download_rate = *rx_delta / elapsed->count();
    rb().push(download_rate);
  }

  std::string format_value(const double value) const override { return std::format("{}/s", format_bytes_si(value)); }

private:
  DeltaTracker<long long> rx_bytes_delta_tracker_;
};

class UploadMetric : public BarPlotMetric
{
public:
  UploadMetric(const RawDataCache &raw_data_cache) : BarPlotMetric("Upload", Color::purple(), raw_data_cache) {}

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 10000.; }
  bool dynamic_ymax() const override { return true; }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::NetDev{}}; }

  void update() override
  {
    const auto &net_dev = raw_data_cache().get<RawData::NetDev>();
    if (!net_dev)
    {
      rb().repeat_last();
      return;
    }

    const auto &tx_delta = tx_bytes_delta_tracker_.delta(net_dev->tx_bytes);
    const auto &elapsed = raw_data_cache().elapsed<RawData::NetDev>();
    if (!tx_delta || !elapsed)
    {
      rb().repeat_last();
      return;
    }

    const double upload_rate = *tx_delta / elapsed->count();
    rb().push(upload_rate);
  }

  std::string format_value(const double value) const override { return std::format("{}/s", format_bytes_si(value)); }

private:
  DeltaTracker<long long> tx_bytes_delta_tracker_;
};

class ProcCpuUsageMetric : public BarPlotMetric
{
public:
  ProcCpuUsageMetric(const RawDataCache &raw_data_cache)
      : BarPlotMetric("Process CPU Usage", Color(150, 47, 200), raw_data_cache)
  {
  }

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 100.; }
  std::vector<RawData::Any> required_raw_data() const override
  {
    return {RawData::ProcStat{}, RawData::ClockTick{}, RawData::CpuThreads{}};
  }

  void update() override
  {
    const auto &proc_stat = raw_data_cache().get<RawData::ProcStat>();
    const auto &elapsed = raw_data_cache().elapsed<RawData::ProcStat>();
    const auto &clock_tick = raw_data_cache().get<RawData::ClockTick>();
    const auto &cpu_threads = raw_data_cache().get<RawData::CpuThreads>();
    if (!proc_stat || !elapsed || !clock_tick || !cpu_threads)
    {
      rb().repeat_last();
      return;
    }

    const unsigned long current_jiffies = proc_stat->utime + proc_stat->stime;
    const auto delta = jiffies_delta_tracker_.delta(current_jiffies);
    if (!delta)
    {
      rb().repeat_last();
      return;
    }

    const double usage_pct = *delta / (elapsed->count() * clock_tick->value) * 100 / cpu_threads->value;
    rb().push(usage_pct);
  }

  void reset() override
  {
    rb().clear();
    jiffies_delta_tracker_.reset();
  }

  std::string format_value(const double value) const override { return std::format("{:.1f}%", value); }

private:
  DeltaTracker<unsigned long> jiffies_delta_tracker_;
};

class ProcMemUsageMetric : public BarPlotMetric
{
public:
  ProcMemUsageMetric(const RawDataCache &raw_data_cache)
      : BarPlotMetric("Process Memory Usage", Color(47, 200, 150), raw_data_cache)
  {
  }

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override
  {
    // TODO: error handling
    return raw_data_cache().get<RawData::MemInfo>()->mem_total_kb;
  }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::MemInfo{}, RawData::ProcStatus{}}; }

  void update() override
  {
    const auto &proc_status = raw_data_cache().get<RawData::ProcStatus>();
    if (!proc_status)
    {
      rb().repeat_last();
      return;
    }

    rb().push(proc_status->vm_rss_kb);
  }

  void reset() override { rb().clear(); }

  std::string format_value(const double value) const override { return format_bytes_iec(value * 1024); };
};

class ProcMajorPageFaultsMetric : public BarPlotMetric
{
public:
  ProcMajorPageFaultsMetric(const RawDataCache &raw_data_cache)
      : BarPlotMetric("Process Major Page Faults", Color(14, 27, 19), raw_data_cache)
  {
  }

  double initial_ymin() const override { return 0.; }
  double initial_ymax() const override { return 100.; }
  bool dynamic_ymax() const override { return true; }
  std::vector<RawData::Any> required_raw_data() const override { return {RawData::ProcStat{}}; }

  void update() override
  {
    const auto &proc_stat = raw_data_cache().get<RawData::ProcStat>();
    const auto &elapsed = raw_data_cache().elapsed<RawData::ProcStat>();
    if (!proc_stat || !elapsed)
    {
      rb().repeat_last();
      return;
    }

    const auto &delta = major_page_faults_delta_tracker_.delta(proc_stat->majflt);
    if (!delta)
    {
      rb().repeat_last();
      return;
    }

    const double rate = *delta / elapsed->count();
    rb().push(rate);
  }

  void reset() override
  {
    rb().clear();
    major_page_faults_delta_tracker_.reset();
  }

  std::string format_value(const double value) const override { return std::format("{:.1f} faults/sec", value); }

private:
  DeltaTracker<unsigned long> major_page_faults_delta_tracker_;
};
