#pragma once

#include <chrono>
#include <optional>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <variant>

#include "raw_data/raw_data.hpp"

template <typename T, typename Variant> struct is_variant_alternative;

template <typename T, typename... Ts>
struct is_variant_alternative<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
{
};

template <typename T>
concept RawDataMember = is_variant_alternative<T, RawData::Any>::value;

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
  template <RawDataMember T> void store(T value)
  {
    auto &entry = entries_[std::type_index(typeid(T))];
    entry.elapsed_time_tracker.update(std::chrono::steady_clock::now());
    entry.value = std::move(value);
  }

  template <RawDataMember T> std::optional<T> get() const
  {
    const auto it = entries_.find(std::type_index(typeid(T)));
    if (it == entries_.cend())
    {
      return std::nullopt;
    }
    return std::get<T>(it->second.value);
  }

  template <RawDataMember T> std::optional<std::chrono::duration<double>> elapsed() const
  {
    const auto it = entries_.find(std::type_index(typeid(T)));
    if (it == entries_.cend())
    {
      return std::nullopt;
    }
    return it->second.elapsed_time_tracker.elapsed();
  }

  template <RawDataMember T> void reset() { entries_.erase(std::type_index(typeid(T))); }

private:
  std::unordered_map<std::type_index, RawDataEntry> entries_;
};
