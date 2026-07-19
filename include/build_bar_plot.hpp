#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "ring_buffer.hpp"

// We use Unicode block elements to draw plot bars: eight glyphs representing one eighth,
// two eighths, and so on up to a full block. Each row of the plot therefore has 8
// "buckets" of fill resolution. The blank glyph is deliberately not included here -- it's
// handled separately, so it isn't just another bucket in this array.
inline constexpr std::array<std::string_view, 8> UNICODE_BLOCK_ELEMENTS = {
    "▁",
    "▂",
    "▃",
    "▄",
    "▅",
    "▆",
    "▇",
    "█",
};
inline constexpr int BUCKETS_PER_ROW = UNICODE_BLOCK_ELEMENTS.size();

inline void validate_inputs(const size_t n_rows, const size_t n_cols, const double ymin, const double ymax)
{
  if (n_rows == 0)
  {
    throw std::invalid_argument("build_bar_plot: rows must be > 0");
  }

  if (n_cols == 0)
  {
    throw std::invalid_argument("build_bar_plot: cols must be > 0");
  }

  if (!(ymin < ymax))
  {
    throw std::invalid_argument(std::format("build_bar_plot: ymin ({}) must be less than ymax ({})", ymin, ymax));
  }
}

struct StartIndexes
{
  size_t col_index;
  size_t data_index;
};

inline StartIndexes compute_start_indexes(const size_t n_cols, const size_t data_size)
{
  if (data_size < n_cols)
  {
    return {.col_index = n_cols - data_size, .data_index = 0};
  }
  return {.col_index = 0, .data_index = data_size - n_cols};
}

// Computes the total number of buckets a value fills, counting from the bottom of the
// plot upward across all rows. For example, a value that completely fills 2 rows and
// then fills 3 more buckets of the 3rd row spans 8 + 8 + 3 = 19 buckets total.
//
// NOTE: the "+1" below means the lowest sliver of the range just above ymin still maps
// to 0 buckets (blank), rather than any positive value immediately showing something.
// This is a deliberate simplification for now -- it may be worth making it toggleable
// per-metric later (e.g. a percentage where any nonzero reading should show at least
// one bucket).
inline int value_to_n_buckets(const size_t n_rows, const double ymin, const double ymax, const double value)
{
  if (value >= ymax)
  {
    return n_rows * BUCKETS_PER_ROW;
  }

  if (value <= ymin)
  {
    return 0;
  }

  const double range_size = (ymax - ymin) / (n_rows * BUCKETS_PER_ROW + 1);
  return floor((value - ymin) / range_size);
}

inline std::vector<std::string_view>
value_to_column_glyphs(const size_t n_rows, const double ymin, const double ymax, const double value)
{
  const int n_buckets = value_to_n_buckets(n_rows, ymin, ymax, value);
  const int full_rows = n_buckets / BUCKETS_PER_ROW;
  const int remaining_rows = n_buckets % BUCKETS_PER_ROW;

  std::vector<std::string_view> glyphs(n_rows, " ");
  std::fill(glyphs.end() - full_rows, glyphs.end(), UNICODE_BLOCK_ELEMENTS.back());
  if (remaining_rows > 0)
  {
    glyphs[n_rows - full_rows - 1] = UNICODE_BLOCK_ELEMENTS[remaining_rows - 1];
  }

  return glyphs;
}

inline std::vector<std::vector<std::string>> build_bar_plot(
    const size_t n_rows,
    const size_t n_cols,
    const double ymin,
    const double ymax,
    const RingBuffer<double> &data_rb)
{
  validate_inputs(n_rows, n_cols, ymin, ymax);

  std::vector<std::vector<std::string>> res(n_rows, std::vector<std::string>(n_cols, " "));
  if (data_rb.size() == 0)
  {
    return res;
  }

  auto [col_index, data_index] = compute_start_indexes(n_cols, data_rb.size());
  for (; col_index < n_cols; ++col_index)
  {
    const auto glyphs = value_to_column_glyphs(n_rows, ymin, ymax, data_rb[data_index++]);
    for (size_t row = 0; row < n_rows; ++row)
    {
      res[row][col_index] = glyphs[row];
    }
  }

  return res;
};
