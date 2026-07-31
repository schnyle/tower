#include <iosfwd>
#include <optional>
#include <string_view>

const std::optional<std::ifstream> read_file(const std::string_view path);
