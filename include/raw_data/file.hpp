#pragma once

#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include "logger.hpp"

inline std::optional<std::ifstream> open_file(const std::string &path)
{
  std::ifstream file(path);
  if (!file)
  {
    LOG_ERROR("failed to open file: ", path);
    return std::nullopt;
  }
  return file;
}

inline std::string_view strip(std::string_view sv, std::string_view chars = " \t")
{
  const size_t start = sv.find_first_not_of(chars);
  if (start == std::string_view::npos)
  {
    return {};
  }
  sv.remove_prefix(start);
  sv.remove_suffix(sv.size() - sv.find_last_not_of(chars) - 1);
  return sv;
}

struct KeyValueLine
{
  std::string_view key;
  std::string_view value;
};

inline std::optional<KeyValueLine> parse_key_value_line(std::string_view sv, const char delim = ':')
{
  const size_t pos = sv.find(delim);
  if (pos == std::string_view::npos)
  {
    return std::nullopt;
  }

  const std::string_view key = strip(sv.substr(0, pos));

  std::string_view value = strip(sv.substr(pos + 1));
  const size_t start = value.find_first_not_of(" \t");
  if (start == std::string_view::npos)
  {
    return std::nullopt;
  }
  value.remove_prefix(start);

  return KeyValueLine{key, value};
}

inline std::string_view parse_next_token(std::string_view &sv)
{
  const size_t start = sv.find_first_not_of(' ');
  if (start == std::string_view::npos)
  {
    sv = {};
    return sv;
  }

  sv.remove_prefix(start);

  const size_t end = sv.find_first_of(' ');
  const std::string_view token = sv.substr(0, end);
  sv.remove_prefix(end == std::string_view::npos ? sv.size() : end);

  return token;
}
