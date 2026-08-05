#pragma once

#include <chrono>
#include <fstream>
#include <optional>
#include <string_view>

#include "logger.hpp"
#include "parsers/parser.hpp"

template <typename T> struct Poll
{
  T value;
  std::chrono::steady_clock::time_point read_time;
};

template <Parser P> std::optional<Poll<typename P::Data>> collect(std::string_view path)
{
  const auto poll_time = std::chrono::steady_clock::now();
  std::ifstream file(path.data());
  if (!file)
  {
    LOG_ERROR("failed to open file", path.data());
    return std::nullopt;
  }

  if (auto data = P::parse(file))
  {
    return Poll<typename P::Data>{*data, poll_time};
  }

  return std::nullopt;
}
