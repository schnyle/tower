#include <fstream>
#include <optional>
#include <string_view>

#include "logger.hpp"
#include "parsers/file.hpp"

const std::optional<std::ifstream> read_file(const std::string_view path)
{
  std::ifstream file(path.data());
  if (!file)
  {
    LOG_ERROR("failed to open file ", path);
    return std::nullopt;
  }

  return file;
};

std::optional<KeyValueLine> parse_key_value_line(std::string_view sv, const char delim)
{
  const size_t pos = sv.find(delim);
  if (pos == std::string_view::npos)
  {
    return std::nullopt;
  }

  const std::string_view key = sv.substr(0, pos);

  std::string_view value = sv.substr(pos + 1);
  const size_t start = value.find_first_not_of(" \t");
  if (start == std::string_view::npos)
  {
    return std::nullopt;
  }
  value.remove_prefix(start);

  return KeyValueLine{key, value};
}

std::string_view parse_next_token(std::string_view &sv)
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
