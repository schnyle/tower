#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "build_bar_plot.hpp"
#include "canvas.hpp"
#include "ring_buffer.hpp"

Canvas make_canvas(const size_t row_count, const size_t col_count, const std::string &fill = " ")
{
  auto c = Canvas(row_count, col_count);
  if (fill != " ")
  {
    for (size_t i = 0; i < row_count; ++i)
    {
      std::fill(c.row_begin(i), c.row_end(i), Cell{fill});
    }
  }
  return c;
}

RingBuffer<double> make_ring_buffer(std::vector<double> values)
{
  RingBuffer<double> res(values.size());
  for (const auto &v : values)
  {
    res.push(v);
  }
  return res;
}

// basic

TEST(BuildBarPlot, CanCallMainFunction)
{
  auto canvas = make_canvas(3, 5);
  build_bar_plot(canvas, 0, 0, 3, 5, 0., 100., RingBuffer<double>(1));
}

// input validation

TEST(BuildBarPlot, ThrowsIfZeroHeight)
{
  auto canvas = make_canvas(3, 5);
  EXPECT_THROW(build_bar_plot(canvas, 0, 0, 0, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfZeroWidth)
{

  auto canvas = make_canvas(3, 5);
  EXPECT_THROW(build_bar_plot(canvas, 0, 0, 3, 0, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfRowRangeExceedsCanvas)
{
  auto canvas = make_canvas(3, 5);
  EXPECT_THROW(build_bar_plot(canvas, 1, 0, 3, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfColRangeExceedsCanvas)
{
  auto canvas = make_canvas(3, 5);
  EXPECT_THROW(build_bar_plot(canvas, 0, 1, 3, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfNotYMinLessThanYMax)
{
  auto canvas = make_canvas(3, 5);
  EXPECT_THROW(build_bar_plot(canvas, 0, 0, 3, 5, 100., 0., RingBuffer<double>(1)), std::invalid_argument);
}

// basic result validation

TEST(BuildBarPlot, ResultIsWhiteSpaceIfNoData)
{
  auto canvas = make_canvas(3, 5, "X");
  build_bar_plot(canvas, 0, 0, 3, 3, 0., 100., RingBuffer<double>(1));
  for (size_t i = 0; i < 3; ++i)
  {
    for (size_t j = 0; j < 3; ++j)
    {
      EXPECT_EQ(canvas(i, j), Cell{" "});
    }
  }
}

TEST(BuildBarPlot, WritesOnlyToAssignedRegion)
{
  auto canvas = make_canvas(5, 7, "X");
  const auto rb = make_ring_buffer({100., 100., 100., 100., 100.});
  build_bar_plot(canvas, 1, 1, 3, 5, 0., 100., rb);

  // interior: every cell in the specified region got written
  for (size_t row = 1; row < 4; ++row)
  {
    for (size_t col = 1; col < 6; ++col)
    {
      EXPECT_EQ(canvas(row, col), Cell{"█"});
    }
  }

  // border: everything just outside the specified region is untouched
  for (size_t col = 0; col < 7; ++col)
  {
    EXPECT_EQ(canvas(0, col), Cell{"X"});
    EXPECT_EQ(canvas(4, 0), Cell{"X"});
  }
  for (size_t row = 0; row < 5; ++row)
  {
    EXPECT_EQ(canvas(row, 0), Cell{"X"});
    EXPECT_EQ(canvas(row, 6), Cell{"X"});
  }
}

// bar plot result validation

struct BuildBarPlotCase
{
  std::string name;
  std::vector<double> values;
  double ymin;
  double ymax;
  size_t row_count;
  size_t col_count;
  std::vector<Cell> expected;
};

class BuildBarPlotParam : public ::testing::TestWithParam<BuildBarPlotCase>
{
};

TEST_P(BuildBarPlotParam, ProducesExpectedGrid)
{
  const auto &c = GetParam();
  auto canvas = make_canvas(c.row_count, c.col_count);
  const auto rb = make_ring_buffer(c.values);
  build_bar_plot(canvas, 0, 0, c.row_count, c.col_count, c.ymin, c.ymax, rb);

  for (size_t row = 0; row < c.row_count; ++row)
  {
    for (size_t col = 0; col < c.col_count; ++col)
    {
      EXPECT_EQ(canvas(row, col), c.expected[row * c.col_count + col]);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    BuildBarPlot,
    BuildBarPlotParam,
    ::testing::Values(
        // single-cell
        BuildBarPlotCase{"SingleCellAtYMaxIsFull", {100}, 0, 100, 1, 1, {Cell{"█"}}},
        BuildBarPlotCase{"SingleCellAboveYMaxIsFull", {101}, 0, 100, 1, 1, {Cell{"█"}}},
        BuildBarPlotCase{"SingleCellAtYMinIsEmpty", {0}, 0, 100, 1, 1, {Cell{" "}}},
        BuildBarPlotCase{"SingleCellBelowYMinIsEmpty", {-1}, 0, 100, 1, 1, {Cell{" "}}},
        BuildBarPlotCase{"SingleCellBucket0IsEmpty", {3.7}, 0, 99, 1, 1, {Cell{" "}}},
        BuildBarPlotCase{"SingleCellBucket1IsOneEigth", {13.6}, 0, 99, 1, 1, {Cell{"▁"}}},
        BuildBarPlotCase{"SingleCellBucket2IsTwoEigths", {25.0}, 0, 99, 1, 1, {Cell{"▂"}}},
        BuildBarPlotCase{"SingleCellBucket3IsThreeEigths", {36.0}, 0, 99, 1, 1, {Cell{"▃"}}},
        BuildBarPlotCase{"SingleCellBucket4IsFourEigths", {50.0}, 0, 99, 1, 1, {Cell{"▄"}}},
        BuildBarPlotCase{"SingleCellBucket5IsFiveEigths", {60.0}, 0, 99, 1, 1, {Cell{"▅"}}},
        BuildBarPlotCase{"SingleCellBucket6IsSixEigths", {70.0}, 0, 99, 1, 1, {Cell{"▆"}}},
        BuildBarPlotCase{"SingleCellBucket7IsSevenEigths", {80.0}, 0, 99, 1, 1, {Cell{"▇"}}},
        BuildBarPlotCase{"SingleCellBucket8IsFull", {90.0}, 0, 99, 1, 1, {Cell{"█"}}},

        // multi row
        BuildBarPlotCase{"MultiRowAtYMaxIsFull", {100.0}, 0, 100, 3, 1, {Cell{"█"}, Cell{"█"}, Cell{"█"}}},
        BuildBarPlotCase{"MultiRowAboveYMaxIsFull", {101.0}, 0, 100, 3, 1, {Cell{"█"}, Cell{"█"}, Cell{"█"}}},
        BuildBarPlotCase{"MultiRowAtYMinIsEmpty", {0.0}, 0, 100, 3, 1, {Cell{" "}, Cell{" "}, Cell{" "}}},
        BuildBarPlotCase{"MultiRowBelowYMinIsEmpty", {-1.0}, 0, 100, 3, 1, {Cell{" "}, Cell{" "}, Cell{" "}}},
        BuildBarPlotCase{"MultiRowFillsOneRowPlusPartial", {50.0}, 0, 99, 3, 1, {Cell{" "}, Cell{"▄"}, Cell{"█"}}},
        BuildBarPlotCase{"MultiRowAtYMinIsBlank", {0.0}, 0, 99, 3, 1, {Cell{" "}, Cell{" "}, Cell{" "}}},
        BuildBarPlotCase{"MultiRowFillsExactlyTwoRows", {67.0}, 0, 99, 3, 1, {Cell{" "}, Cell{"█"}, Cell{"█"}}},

        // multi column
        BuildBarPlotCase{
            "MultiColumnEachColumnGetsCorrectGlyph",
            {0., 50., 100.},
            0,
            100,
            1,
            3,
            {Cell{" "}, Cell{"▄"}, Cell{"█"}}},
        BuildBarPlotCase{
            "MultiColumnRightAlignedIfDataLessThanWidth",
            {50., 100.},
            0,
            100,
            1,
            4,
            {Cell{" "}, Cell{" "}, Cell{"▄"}, Cell{"█"}}},
        BuildBarPlotCase{
            "MultiColumnRightAlignedIfDataGreaterThanWidth",
            {50., 0., 50., 100.},
            0,
            100,
            1,
            2,
            {Cell{"▄"}, Cell{"█"}}}),
    [](const ::testing::TestParamInfo<BuildBarPlotCase> &info) { return info.param.name; });
