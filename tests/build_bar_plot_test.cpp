#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "build_bar_plot.hpp"
#include "ring_buffer.hpp"

std::vector<std::vector<std::string>>
make_grid(const size_t row_count, const size_t col_count, const std::string &fill = " ")
{
  return std::vector<std::vector<std::string>>(row_count, std::vector<std::string>(col_count, fill));
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
  auto dest = make_grid(3, 5);
  build_bar_plot(dest, 0, 0, 3, 5, 0., 100., RingBuffer<double>(1));
}

// input validation

TEST(BuildBarPlot, ThrowsIfZeroHeight)
{
  auto dest = make_grid(3, 5);
  EXPECT_THROW(build_bar_plot(dest, 0, 0, 0, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfZeroWidth)
{

  auto dest = make_grid(3, 5);
  EXPECT_THROW(build_bar_plot(dest, 0, 0, 3, 0, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfRowRangeExceedsDest)
{
  auto dest = make_grid(3, 5);
  EXPECT_THROW(build_bar_plot(dest, 1, 0, 3, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfColRangeExceedsDest)
{
  auto dest = make_grid(3, 5);
  EXPECT_THROW(build_bar_plot(dest, 0, 1, 3, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfNotYMinLessThanYMax)
{
  auto dest = make_grid(3, 5);
  EXPECT_THROW(build_bar_plot(dest, 0, 0, 3, 5, 100., 0., RingBuffer<double>(1)), std::invalid_argument);
}

// basic result validation

TEST(BuildBarPlot, ResultIsWhiteSpaceIfNoData)
{
  auto dest = make_grid(3, 5, "X");
  build_bar_plot(dest, 0, 0, 3, 3, 0., 100., RingBuffer<double>(1));
  for (size_t i = 0; i < 3; ++i)
  {
    for (size_t j = 0; j < 3; ++j)
    {
      EXPECT_EQ(dest[i][j], " ");
    }
  }
}

TEST(BuildBarPlot, WritesOnlyToAssignedRegion)
{
  auto dest = make_grid(5, 7, "X");
  const auto rb = make_ring_buffer({100., 100., 100., 100., 100.});
  build_bar_plot(dest, 1, 1, 3, 5, 0., 100., rb);

  // interior: every cell in the specified region got written
  for (size_t row = 1; row < 4; ++row)
  {
    for (size_t col = 1; col < 6; ++col)
    {
      EXPECT_EQ(dest[row][col], "█");
    }
  }

  // border: everything just outside the specified region is untouched
  for (size_t col = 0; col < 7; ++col)
  {
    EXPECT_EQ(dest[0][col], "X");
    EXPECT_EQ(dest[4][col], "X");
  }
  for (size_t row = 0; row < 5; ++row)
  {
    EXPECT_EQ(dest[row][0], "X");
    EXPECT_EQ(dest[row][6], "X");
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
  std::vector<std::vector<std::string>> expected;
};

class BuildBarPlotParam : public ::testing::TestWithParam<BuildBarPlotCase>
{
};

TEST_P(BuildBarPlotParam, ProducesExpectedGrid)
{
  const auto &c = GetParam();
  auto dest = make_grid(c.row_count, c.col_count);
  const auto rb = make_ring_buffer(c.values);
  build_bar_plot(dest, 0, 0, c.row_count, c.col_count, c.ymin, c.ymax, rb);
  EXPECT_EQ(dest, c.expected);
}

INSTANTIATE_TEST_SUITE_P(
    BuildBarPlot,
    BuildBarPlotParam,
    ::testing::Values(
        // single-cell
        BuildBarPlotCase{"SingleCellAtYMaxIsFull", {100}, 0, 100, 1, 1, {{"█"}}},
        BuildBarPlotCase{"SingleCellAboveYMaxIsFull", {101}, 0, 100, 1, 1, {{"█"}}},
        BuildBarPlotCase{"SingleCellAtYMinIsEmpty", {0}, 0, 100, 1, 1, {{" "}}},
        BuildBarPlotCase{"SingleCellBelowYMinIsEmpty", {-1}, 0, 100, 1, 1, {{" "}}},
        BuildBarPlotCase{"SingleCellBucket0IsEmpty", {3.7}, 0, 99, 1, 1, {{" "}}},
        BuildBarPlotCase{"SingleCellBucket1IsOneEigth", {13.6}, 0, 99, 1, 1, {{"▁"}}},
        BuildBarPlotCase{"SingleCellBucket2IsTwoEigths", {25.0}, 0, 99, 1, 1, {{"▂"}}},
        BuildBarPlotCase{"SingleCellBucket3IsThreeEigths", {36.0}, 0, 99, 1, 1, {{"▃"}}},
        BuildBarPlotCase{"SingleCellBucket4IsFourEigths", {50.0}, 0, 99, 1, 1, {{"▄"}}},
        BuildBarPlotCase{"SingleCellBucket5IsFiveEigths", {60.0}, 0, 99, 1, 1, {{"▅"}}},
        BuildBarPlotCase{"SingleCellBucket6IsSixEigths", {70.0}, 0, 99, 1, 1, {{"▆"}}},
        BuildBarPlotCase{"SingleCellBucket7IsSevenEigths", {80.0}, 0, 99, 1, 1, {{"▇"}}},
        BuildBarPlotCase{"SingleCellBucket8IsFull", {90.0}, 0, 99, 1, 1, {{"█"}}},

        // multi row
        BuildBarPlotCase{"MultiRowAtYMaxIsFull", {100.0}, 0, 100, 3, 1, {{"█"}, {"█"}, {"█"}}},
        BuildBarPlotCase{"MultiRowAboveYMaxIsFull", {101.0}, 0, 100, 3, 1, {{"█"}, {"█"}, {"█"}}},
        BuildBarPlotCase{"MultiRowAtYMinIsEmpty", {0.0}, 0, 100, 3, 1, {{" "}, {" "}, {" "}}},
        BuildBarPlotCase{"MultiRowBelowYMinIsEmpty", {-1.0}, 0, 100, 3, 1, {{" "}, {" "}, {" "}}},
        BuildBarPlotCase{"MultiRowFillsOneRowPlusPartial", {50.0}, 0, 99, 3, 1, {{" "}, {"▄"}, {"█"}}},
        BuildBarPlotCase{"MultiRowAtYMinIsBlank", {0.0}, 0, 99, 3, 1, {{" "}, {" "}, {" "}}},
        BuildBarPlotCase{"MultiRowFillsExactlyTwoRows", {67.0}, 0, 99, 3, 1, {{" "}, {"█"}, {"█"}}},

        // multi column
        BuildBarPlotCase{"MultiColumnEachColumnGetsCorrectGlyph", {0., 50., 100.}, 0, 100, 1, 3, {{" ", "▄", "█"}}},
        BuildBarPlotCase{"MultiColumnRightAlignedIfDataLessThanWidth", {50., 100.}, 0, 100, 1, 4, {{" ", " ", "▄", "█"}}},
        BuildBarPlotCase{
            "MultiColumnRightAlignedIfDataGreaterThanWidth",
            {50., 0., 50., 100.},
            0,
            100,
            1,
            2,
            {{"▄", "█"}}}),
    [](const ::testing::TestParamInfo<BuildBarPlotCase> &info) { return info.param.name; });
