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
