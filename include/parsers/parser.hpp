#pragma once

#include <iosfwd>
#include <optional>

template <typename P>
concept Parser = requires(std::istream &is) {
  typename P::Data;
  { P::parse(is) } -> std::same_as<std::optional<typename P::Data>>;
};
