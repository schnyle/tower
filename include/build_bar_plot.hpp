#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "canvas.hpp"
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

inline void validate_inputs(
    const Canvas &dest,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count,
    const double ymin,
    const double ymax)
{
  if (row_count == 0)
  {
    throw std::invalid_argument("build_bar_plot: rows must be > 0");
  }

  if (col_count == 0)
  {
    throw std::invalid_argument("build_bar_plot: cols must be > 0");
  }

  if (dest.row_count() < row_offset + row_count)
  {
    const auto msg = std::format(
        "build_bar_plot: row_offset ({}) + row_count ({}) exceeds dest rows ({})",
        row_offset,
        row_count,
        dest.row_count());
    throw std::invalid_argument(msg);
  }

  if (dest.col_count() < col_offset + col_count)
  {
    const auto msg = std::format(
        "build_bar_plot: col_offset ({}) + col_offset ({}) exceeds dest cols ({})",
        col_offset,
        col_count,
        dest.col_count());
    throw std::invalid_argument(msg);
  }

  if (!(ymin < ymax))
  {
    const auto msg = std::format("build_bar_plot: ymin ({}) must be less than ymax ({})", ymin, ymax);
    throw std::invalid_argument(msg);
  }
}

struct StartIndexes
{
  size_t col_index;
  size_t data_index;
};

inline StartIndexes compute_start_indexes(const size_t col_count, const size_t data_size)
{
  if (data_size < col_count)
  {
    return {.col_index = col_count - data_size, .data_index = 0};
  }
  return {.col_index = 0, .data_index = data_size - col_count};
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
inline int value_to_n_buckets(const size_t row_count, const double ymin, const double ymax, const double value)
{
  if (value >= ymax)
  {
    return row_count * BUCKETS_PER_ROW;
  }

  if (value <= ymin)
  {
    return 0;
  }

  const double range_size = (ymax - ymin) / (row_count * BUCKETS_PER_ROW + 1);
  return floor((value - ymin) / range_size);
}

inline std::vector<std::string_view>
value_to_column_glyphs(const size_t row_count, const double ymin, const double ymax, const double value)
{
  const int n_buckets = value_to_n_buckets(row_count, ymin, ymax, value);
  const int full_rows = n_buckets / BUCKETS_PER_ROW;
  const int remaining_rows = n_buckets % BUCKETS_PER_ROW;

  std::vector<std::string_view> glyphs(row_count, " ");
  std::fill(glyphs.end() - full_rows, glyphs.end(), UNICODE_BLOCK_ELEMENTS.back());
  if (remaining_rows > 0)
  {
    glyphs[row_count - full_rows - 1] = UNICODE_BLOCK_ELEMENTS[remaining_rows - 1];
  }

  return glyphs;
}

inline void build_bar_plot(
    Canvas &dest,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count,
    const double ymin,
    const double ymax,
    const Color color,
    const RingBuffer<double> &data_rb)
{
  validate_inputs(dest, row_offset, col_offset, row_count, col_count, ymin, ymax);

  for (size_t row = row_offset; row < row_offset + row_count; ++row)
  {
    std::fill(dest.row_begin(row) + col_offset, dest.row_begin(row) + col_offset + col_count, Cell{" "});
  }

  if (data_rb.size() == 0)
  {
    return;
  }

  auto [col_index, data_index] = compute_start_indexes(col_count, data_rb.size());
  for (; col_index < col_count; ++col_index)
  {
    const auto glyphs = value_to_column_glyphs(row_count, ymin, ymax, data_rb[data_index++]);
    for (size_t row_index = 0; row_index < row_count; ++row_index)
    {
      dest(row_offset + row_index, col_offset + col_index) = Cell{std::string(glyphs[row_index]), color};
    }
  }
};
