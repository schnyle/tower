#pragma once

#include <chrono>
#include <fstream>
#include <optional>
#include <string>

#include "logger.hpp"
#include "parsers/parser.hpp"

template <typename T> struct Poll
{
  T value;
  std::chrono::steady_clock::time_point read_time;
};

template <Parser P> std::optional<Poll<typename P::Data>> collect_from_path(const std::string &path)
{
  const auto poll_time = std::chrono::steady_clock::now();
  std::ifstream file(path);
  if (!file)
  {
    LOG_ERROR("failed to open file", path);
    return std::nullopt;
  }

  if (auto data = P::parse(file))
  {
    return Poll<typename P::Data>{*data, poll_time};
  }

  return std::nullopt;
}

template <SystemParser P> std::optional<Poll<typename P::Data>> collect() { return collect_from_path<P>(P::path()); }

template <ProcParser P> std::optional<Poll<typename P::Data>> collect(int pid)
{
  return collect_from_path<P>(P::path(pid));
}
