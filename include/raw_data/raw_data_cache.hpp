#pragma once

#include <chrono>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include "raw_data/raw_data.hpp"

class ElapsedTimeTracker
{
public:
  void update(std::chrono::steady_clock::time_point now)
  {
    elapsed_ = previous_ ? std::optional{std::chrono::duration<double>(now - *previous_)} : std::nullopt;
    previous_ = now;
  }

  std::optional<std::chrono::duration<double>> elapsed() const { return elapsed_; }

  void reset()
  {
    previous_ = std::nullopt;
    elapsed_ = std::nullopt;
  }

private:
  std::optional<std::chrono::steady_clock::time_point> previous_;
  std::optional<std::chrono::duration<double>> elapsed_;
};

struct RawDataEntry
{
  RawData::Any value;
  ElapsedTimeTracker elapsed_time_tracker;
};

class RawDataCache
{
public:
  template <RawDataKind T> void store(T value)
  {
    auto &entry = entries_[std::type_index(typeid(T))];
    entry.elapsed_time_tracker.update(std::chrono::steady_clock::now());
    entry.value = std::move(value);
  }

  template <RawDataKind T> std::optional<T> get() const
  {
    const auto it = entries_.find(std::type_index(typeid(T)));
    if (it == entries_.cend())
    {
      return std::nullopt;
    }
    return std::get<T>(it->second.value);
  }

  template <RawDataKind T> std::optional<std::chrono::duration<double>> elapsed() const
  {
    const auto it = entries_.find(std::type_index(typeid(T)));
    if (it == entries_.cend())
    {
      return std::nullopt;
    }
    return it->second.elapsed_time_tracker.elapsed();
  }

  template <RawDataKind T> void reset() { entries_.erase(std::type_index(typeid(T))); }

private:
  std::unordered_map<std::type_index, RawDataEntry> entries_;
};
