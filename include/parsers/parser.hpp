#pragma once

#include <iosfwd>
#include <optional>

template <typename P>
concept Parser = requires(std::istream &is) {
  typename P::Data;
  { P::parse(is) } -> std::same_as<std::optional<typename P::Data>>;
};

template <typename P>
concept SystemParser = Parser<P> && requires {
  { P::path() } -> std::same_as<std::string>;
};

template <typename P>
concept ProcParser = Parser<P> && requires(int pid) {
  { P::path(pid) } -> std::same_as<std::string>;
};
