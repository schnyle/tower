#include <iosfwd>
#include <optional>
#include <string_view>

std::string_view strip(std::string_view sv, std::string_view chars = " \t");

const std::optional<std::ifstream> read_file(const std::string_view);

struct KeyValueLine
{
  std::string_view key;
  std::string_view value;
};

std::optional<KeyValueLine> parse_key_value_line(std::string_view, const char = ':');

std::string_view parse_next_token(std::string_view &);
