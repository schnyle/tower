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

RingBuffer<double> make_ring_buffer(std::initializer_list<double> values)
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

// single cell result validation

TEST(BuildBarPlot, SingleCellValueAtYMaxIsFullBlock)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({100});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], "█");
}

TEST(BuildBarPlot, SingleCellValueAboveYMaxIsFullBlock)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({101});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], "█");
}

TEST(BuildBarPlot, SingleCellValueAtYMinIsEmptyBlock)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueBelowYMinIsEmptyBlock)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({-1});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueInBucket0IsBlank)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({3.7});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueInBucket1IsOneEighth)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({13.6});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▁");
}

TEST(BuildBarPlot, SingleCellValueInBucket2IsTwoEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({25.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▂");
}

TEST(BuildBarPlot, SingleCellValueInBucket3IsThreeEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({36.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▃");
}

TEST(BuildBarPlot, SingleCellValueInBucket4IsFourEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({50.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▄");
}

TEST(BuildBarPlot, SingleCellValueInBucket5IsFiveEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({60.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▅");
}

TEST(BuildBarPlot, SingleCellValueInBucket6IsSixEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({70.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▆");
}

TEST(BuildBarPlot, SingleCellValueInBucket7IsSevenEighths)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({80.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "▇");
}

TEST(BuildBarPlot, SingleCellValueInBucket8IsFullBlock)
{
  auto dest = make_grid(1, 1);
  const auto rb = make_ring_buffer({90.0});
  build_bar_plot(dest, 0, 0, 1, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], "█");
}

// multi-row result validation

TEST(BuildBarPlot, MultiRowValueAtYMaxIsFullBlock)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({100});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], "█");
  EXPECT_EQ(dest[1][0], "█");
  EXPECT_EQ(dest[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAboveYMaxIsFullBlock)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({101});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], "█");
  EXPECT_EQ(dest[1][0], "█");
  EXPECT_EQ(dest[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAtYMinIsEmptyBlock)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({0});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[1][0], " ");
  EXPECT_EQ(dest[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueBelowYMinIsEmptyBlock)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({-1});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[1][0], " ");
  EXPECT_EQ(dest[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueFillsOneRowPlusPartial)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({50.0});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[1][0], "▄");
  EXPECT_EQ(dest[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAtYMinIsBlank)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({0.0});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[1][0], " ");
  EXPECT_EQ(dest[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueFillsExactlyTwoRowsWithNoPartial)
{
  auto dest = make_grid(3, 1);
  const auto rb = make_ring_buffer({67.0});
  build_bar_plot(dest, 0, 0, 3, 1, 0, 99, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[1][0], "█");
  EXPECT_EQ(dest[2][0], "█");
}

// multi-column result validation

TEST(BuildBarPlot, MultiColumnEachColumnGetsCorrectGlyph)
{
  auto dest = make_grid(1, 3);
  const auto rb = make_ring_buffer({0., 50., 100.});
  build_bar_plot(dest, 0, 0, 1, 3, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[0][1], "▄");
  EXPECT_EQ(dest[0][2], "█");
}

TEST(BuildBarPlot, MultiColumnRightAlignedIfDataLessThanWidth)
{
  auto dest = make_grid(1, 4);
  const auto rb = make_ring_buffer({50., 100.});
  build_bar_plot(dest, 0, 0, 1, 4, 0, 100, rb);
  EXPECT_EQ(dest[0][0], " ");
  EXPECT_EQ(dest[0][1], " ");
  EXPECT_EQ(dest[0][2], "▄");
  EXPECT_EQ(dest[0][3], "█");
}

TEST(BuildBarPlot, MultiColumnRightAlignedIfDataGreaterThanWidth)
{
  auto dest = make_grid(1, 2);
  const auto rb = make_ring_buffer({50., 0., 50., 100.});
  build_bar_plot(dest, 0, 0, 1, 2, 0, 100, rb);
  EXPECT_EQ(dest[0][0], "▄");
  EXPECT_EQ(dest[0][1], "█");
}

// // TODO
// //   - refactor using parametrized google tests (TEST_P)
