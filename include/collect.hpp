#pragma once

#include <fstream>
#include <optional>
#include <string_view>

#include "logger.hpp"
#include "parsers/parser.hpp"

template <Parser P> std::optional<typename P::Data> collect(std::string_view path)
{

  std::ifstream file(path.data());
  if (!file)
  {
    LOG_ERROR("failed to open file", path.data());
    return std::nullopt;
  }

  if (auto data = P::parse(file))
  {
    return *data;
  }

  return std::nullopt;
}
