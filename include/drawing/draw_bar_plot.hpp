#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "canvas.hpp"
#include "ring_buffer.hpp"
#include "windows/window.hpp"

// Unicode block elements used to draw plot bars: eight glyphs representing one eighth,
// two eighths, and so on up to a full block, giving each row of the plot 8 "buckets" of
// fill resolution.
// The blank glyph is deliberately not included here, it's handled separately, so it
// isn't just another bucket in these arrays.
inline constexpr std::array<std::string_view, 8> UNICODE_LOWER_BLOCK_ELEMENTS = {
    "▁",
    "▂",
    "▃",
    "▄",
    "▅",
    "▆",
    "▇",
    "█",
};

// Not a contiguous codepoint run like UNICODE_LOWER_BLOCK_ELEMENTS is: the 1/8, 4/8, and
// 8/8 glyphs live in the original Unicode Block Elements block (U+2580-U+259F), while the
// 2/8, 3/8, 5/8, 6/8, and 7/8 glyphs come from the newer Symbols for Legacy Computing
// block (U+1FB00+, Unicode 13.0/2020).
// Expect weaker terminal font support than UNICODE_LOWER_BLOCK_ELEMENTS.
inline constexpr std::array<std::string_view, 8> UNICODE_UPPER_BLOCK_ELEMENTS = {
    "▔",
    "🮂",
    "🮃",
    "▀",
    "🮄",
    "🮅",
    "🮆",
    "█",
};

static_assert(
    UNICODE_LOWER_BLOCK_ELEMENTS.size() == UNICODE_UPPER_BLOCK_ELEMENTS.size(),
    "upper/lower block element sets must stay the same size; "
    "draw_bar_plot picks between them by index at a shared n_glyphs");

inline void validate_inputs(const Canvas &canvas, const Rect rect, const double ymin, const double ymax)
{
  if (rect.row_count == 0)
  {
    throw std::invalid_argument("draw_bar_plot: rows must be > 0");
  }

  if (rect.col_count == 0)
  {
    throw std::invalid_argument("draw_bar_plot: cols must be > 0");
  }

  if (canvas.row_count() < rect.row_offset + rect.row_count)
  {
    const auto msg = std::format(
        "draw_bar_plot: row_offset ({}) + row_count ({}) exceeds canvas rows ({})",
        rect.row_offset,
        rect.row_count,
        canvas.row_count());
    throw std::invalid_argument(msg);
  }

  if (canvas.col_count() < rect.col_offset + rect.col_count)
  {
    const auto msg = std::format(
        "draw_bar_plot: col_offset ({}) + col_offset ({}) exceeds canvas cols ({})",
        rect.col_offset,
        rect.col_count,
        canvas.col_count());
    throw std::invalid_argument(msg);
  }

  if (!(ymin < ymax))
  {
    const auto msg = std::format("draw_bar_plot: ymin ({}) must be less than ymax ({})", ymin, ymax);
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
// plot upward across all rows, out of n_glyphs "buckets" of fill resolution per row. For
// example, with n_glyphs=8, a value that completely fills 2 rows and then fills 3 more
// buckets of the 3rd row spans 8 + 8 + 3 = 19 buckets total.
//
// NOTE: the "+1" below means the lowest sliver of the range just above ymin still maps
// to 0 buckets (blank), rather than any positive value immediately showing something.
// This is a deliberate simplification for now -- it may be worth making it toggleable
// per-metric later (e.g. a percentage where any nonzero reading should show at least
// one bucket).
inline int value_to_n_buckets(
    const size_t row_count,
    const double ymin,
    const double ymax,
    const double value,
    const size_t n_glyphs)
{
  if (value >= ymax)
  {
    return row_count * n_glyphs;
  }

  if (value <= ymin)
  {
    return 0;
  }

  const double range_size = (ymax - ymin) / (row_count * n_glyphs + 1);
  return floor((value - ymin) / range_size);
}

inline std::vector<std::optional<size_t>> value_to_column_glyph_indexes(
    const size_t row_count,
    const double ymin,
    const double ymax,
    const double value,
    const size_t n_glyphs)
{
  const int n_buckets = value_to_n_buckets(row_count, ymin, ymax, value, n_glyphs);
  const int full_rows = n_buckets / static_cast<int>(n_glyphs);
  const int remaining_rows = n_buckets % static_cast<int>(n_glyphs);

  std::vector<std::optional<size_t>> indexes(row_count, std::nullopt);
  std::fill(indexes.end() - full_rows, indexes.end(), n_glyphs - 1);
  if (remaining_rows > 0)
  {
    indexes[row_count - full_rows - 1] = remaining_rows - 1;
  }

  return indexes;
}

inline void draw_bar_plot(
    Canvas &canvas,
    const Rect rect,
    const double ymin,
    const double ymax,
    const Color color,
    const RingBuffer<double> &data_rb,
    const bool invert = false,
    const bool never_empty = false)
{
  validate_inputs(canvas, rect, ymin, ymax);

  for (size_t row = rect.row_offset; row < rect.row_offset + rect.row_count; ++row)
  {
    std::fill(
        canvas.row_begin(row) + rect.col_offset, canvas.row_begin(row) + rect.col_offset + rect.col_count, Cell{" "});
  }

  if (data_rb.size() == 0)
  {
    return;
  }

  const auto &block_elements = invert ? UNICODE_UPPER_BLOCK_ELEMENTS : UNICODE_LOWER_BLOCK_ELEMENTS;

  auto [col_index, data_index] = compute_start_indexes(rect.col_count, data_rb.size());
  for (; col_index < rect.col_count; ++col_index)
  {
    auto indexes = value_to_column_glyph_indexes(
        rect.row_count, ymin, ymax, data_rb[data_index++], block_elements.size());

    if (never_empty && std::all_of(indexes.cbegin(), indexes.cend(), [](const auto &idx) { return !idx.has_value(); }))
    {
      indexes[rect.row_count - 1] = 0;
    }

    for (size_t row_index = 0; row_index < rect.row_count; ++row_index)
    {
      const size_t row = invert ? rect.row_offset + rect.row_count - row_index - 1 : rect.row_offset + row_index;
      const std::string_view glyph = indexes[row_index] ? block_elements[*indexes[row_index]] : " ";
      canvas(row, rect.col_offset + col_index) = Cell{std::string(glyph), color};
    }
  }
};

inline std::optional<double> find_max_displayed_value(const RingBuffer<double> &rb, const size_t col_count)
{
  if (rb.size() == 0)
  {
    return std::nullopt;
  }

  const auto [col_index, data_index] = compute_start_indexes(col_count, rb.size());
  double max = rb[data_index];
  for (size_t i = data_index + 1; i < rb.size(); ++i)
  {
    max = std::max(max, rb[i]);
  }
  return max;
}
