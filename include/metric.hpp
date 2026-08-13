#pragma once

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "raw_data/raw_data.hpp"
#include "raw_data/raw_data_cache.hpp"
#include "ring_buffer.hpp"

template <typename T>
class DeltaTracker
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

class Metric
{
public:
  Metric(const RawDataCache &raw_data_cache) : raw_data_cache_(raw_data_cache) {};
  virtual ~Metric() = default;

  RingBuffer<double> &rb() { return rb_; }

  virtual std::vector<RawData::Any> required_raw_data() const = 0;
  virtual void update() = 0;
  virtual void reset() {}

protected:
  const RawDataCache &raw_data_cache() const { return raw_data_cache_; }

private:
  RingBuffer<double> rb_{50000};
  const RawDataCache &raw_data_cache_;
};

template <RawDataKind T, auto MemberPtr>
class GaugeMetric : public Metric
{
public:
  using Metric::Metric;

  std::vector<RawData::Any> required_raw_data() const override { return {T{}}; }

  void update() override
  {
    const auto data = raw_data_cache().template get<T>();
    if (!data)
    {
      rb().repeat_last();
      return;
    }

    rb().push((*data).*MemberPtr);
  }

  void reset() override { rb().clear(); }
};

template <RawDataKind T, auto MemberPtr>
inline std::unique_ptr<Metric> make_gauge_metric(const RawDataCache &cache)
{
  return std::make_unique<GaugeMetric<T, MemberPtr>>(cache);
}

template <RawDataKind T, auto MemberPtr>
class DeltaRateMetric : public Metric
{
public:
  using FieldType = std::remove_cvref_t<decltype(std::declval<T>().*MemberPtr)>;
  using Metric::Metric;

  std::vector<RawData::Any> required_raw_data() const override { return {T{}}; }

  void update() override
  {
    const auto data = raw_data_cache().template get<T>();
    if (!data)
    {
      rb().repeat_last();
      return;
    }

    const auto elapsed = raw_data_cache().template elapsed<T>();
    if (!elapsed)
    {
      rb().repeat_last();
      return;
    }

    const auto delta = delta_tracker_.delta((*data).*MemberPtr);
    if (!delta)
    {
      rb().repeat_last();
      return;
    }

    rb().push(*delta / elapsed->count());
  }

  void reset() override
  {
    rb().clear();
    delta_tracker_.reset();
  }

private:
  DeltaTracker<FieldType> delta_tracker_;
};

template <RawDataKind T, auto MemberPtr>
inline std::unique_ptr<Metric> make_delta_rate_metric(const RawDataCache &cache)
{
  return std::make_unique<DeltaRateMetric<T, MemberPtr>>(cache);
}

class CpuLoadMetric : public Metric
{
public:
  using Metric::Metric;

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

private:
  DeltaTracker<RawData::Stat::CpuTimes> cpu_times_delta_tracker_;
};

inline std::unique_ptr<Metric> make_cpu_load_metric(const RawDataCache &cache)
{
  return std::make_unique<CpuLoadMetric>(cache);
}

class ProcCpuUsageMetric : public Metric
{
public:
  using Metric::Metric;

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

private:
  DeltaTracker<unsigned long> jiffies_delta_tracker_;
};

inline std::unique_ptr<Metric> make_proc_cpu_usage_metric(const RawDataCache &cache)
{
  return std::make_unique<ProcCpuUsageMetric>(cache);
}
