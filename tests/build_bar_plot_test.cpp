#include <gtest/gtest.h>
#include <initializer_list>
#include <stdexcept>

#include "build_bar_plot.hpp"
#include "ring_buffer.hpp"

RingBuffer<double> make_ring_buffer(std::initializer_list<double> values)
{
  RingBuffer<double> res(values.size());
  for (const auto &v : values)
  {
    res.push(v);
  }
  return res;
}

TEST(BuildBarPlot, CanCallMainFunction) { build_bar_plot(3, 5, 0., 100., RingBuffer<double>(1)); }

TEST(BuildBarPlot, ThrowsIfZeroWidth)
{

  EXPECT_THROW(build_bar_plot(3, 0, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfZeroHeight)
{

  EXPECT_THROW(build_bar_plot(0, 5, 0., 100., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ThrowsIfNotYMinLessThanYMax)
{
  EXPECT_THROW(build_bar_plot(3, 5, 100., 0., RingBuffer<double>(1)), std::invalid_argument);
}

TEST(BuildBarPlot, ResultHasCorrectShape)
{
  const auto res = build_bar_plot(3, 5, 0., 100., RingBuffer<double>(1));
  EXPECT_EQ(res.size(), 3u);
  for (const auto &row : res)
  {
    EXPECT_EQ(row.size(), 5u);
  }
}

TEST(BuildBarPlot, ResultIsWhiteSpaceIfNoData)
{
  const auto res = build_bar_plot(3, 3, 0., 100., RingBuffer<double>(1));
  for (size_t i = 0; i < 3; ++i)
  {
    for (size_t j = 0; j < 3; ++j)
    {
      EXPECT_EQ(res[i][j], " ");
    }
  }
}

TEST(BuildBarPlot, SingleCellValueAtYMaxIsFullBlock)
{

  const auto rb = make_ring_buffer({100});
  const auto res = build_bar_plot(1, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], "█");
}

TEST(BuildBarPlot, SingleCellValueAboveYMaxIsFullBlock)
{

  const auto rb = make_ring_buffer({101});
  const auto res = build_bar_plot(1, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], "█");
}

TEST(BuildBarPlot, SingleCellValueAtYMinIsEmptyBlock)
{

  const auto rb = make_ring_buffer({0});
  const auto res = build_bar_plot(1, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueBelowYMinIsEmptyBlock)
{

  const auto rb = make_ring_buffer({-1});
  const auto res = build_bar_plot(1, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueInBucket0IsBlank)
{
  const auto rb = make_ring_buffer({3.7});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], " ");
}

TEST(BuildBarPlot, SingleCellValueInBucket1IsOneEighth)
{
  const auto rb = make_ring_buffer({13.6});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▁");
}

TEST(BuildBarPlot, SingleCellValueInBucket2IsTwoEighths)
{
  const auto rb = make_ring_buffer({25.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▂");
}

TEST(BuildBarPlot, SingleCellValueInBucket3IsThreeEighths)
{
  const auto rb = make_ring_buffer({36.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▃");
}

TEST(BuildBarPlot, SingleCellValueInBucket4IsFourEighths)
{
  const auto rb = make_ring_buffer({50.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▄");
}

TEST(BuildBarPlot, SingleCellValueInBucket5IsFiveEighths)
{
  const auto rb = make_ring_buffer({60.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▅");
}

TEST(BuildBarPlot, SingleCellValueInBucket6IsSixEighths)
{
  const auto rb = make_ring_buffer({70.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▆");
}

TEST(BuildBarPlot, SingleCellValueInBucket7IsSevenEighths)
{
  const auto rb = make_ring_buffer({80.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "▇");
}

TEST(BuildBarPlot, SingleCellValueInBucket8IsFullBlock)
{
  const auto rb = make_ring_buffer({90.0});
  const auto res = build_bar_plot(1, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAtYMaxIsFullBlock)
{

  const auto rb = make_ring_buffer({100});
  const auto res = build_bar_plot(3, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], "█");
  EXPECT_EQ(res[1][0], "█");
  EXPECT_EQ(res[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAboveYMaxIsFullBlock)
{

  const auto rb = make_ring_buffer({101});
  const auto res = build_bar_plot(3, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], "█");
  EXPECT_EQ(res[1][0], "█");
  EXPECT_EQ(res[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAtYMinIsEmptyBlock)
{

  const auto rb = make_ring_buffer({0});
  const auto res = build_bar_plot(3, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[1][0], " ");
  EXPECT_EQ(res[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueBelowYMinIsEmptyBlock)
{

  const auto rb = make_ring_buffer({-1});
  const auto res = build_bar_plot(3, 1, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[1][0], " ");
  EXPECT_EQ(res[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueFillsOneRowPlusPartial)
{
  const auto rb = make_ring_buffer({50.0});
  const auto res = build_bar_plot(3, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[1][0], "▄");
  EXPECT_EQ(res[2][0], "█");
}

TEST(BuildBarPlot, MultiRowValueAtYMinIsBlank)
{
  const auto rb = make_ring_buffer({0.0});
  const auto res = build_bar_plot(3, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[1][0], " ");
  EXPECT_EQ(res[2][0], " ");
}

TEST(BuildBarPlot, MultiRowValueFillsExactlyTwoRowsWithNoPartial)
{
  const auto rb = make_ring_buffer({67.0});
  const auto res = build_bar_plot(3, 1, 0, 99, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[1][0], "█");
  EXPECT_EQ(res[2][0], "█");
}

TEST(BuildBarPlot, MultiColumnEachColumnGetsCorrectGlyph)
{
  const auto rb = make_ring_buffer({0., 50., 100.});
  const auto res = build_bar_plot(1, 3, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[0][1], "▄");
  EXPECT_EQ(res[0][2], "█");
}

TEST(BuildBarPlot, MultiColumnRightAlignedIfDataLessThanWidth)
{
  const auto rb = make_ring_buffer({50., 100.});
  const auto res = build_bar_plot(1, 4, 0, 100, rb);
  EXPECT_EQ(res[0][0], " ");
  EXPECT_EQ(res[0][1], " ");
  EXPECT_EQ(res[0][2], "▄");
  EXPECT_EQ(res[0][3], "█");
}

TEST(BuildBarPlot, MultiColumnRightAlignedIfDataGreaterThanWidth)
{
  const auto rb = make_ring_buffer({50., 0., 50., 100.});
  const auto res = build_bar_plot(1, 2, 0, 100, rb);
  EXPECT_EQ(res[0][0], "▄");
  EXPECT_EQ(res[0][1], "█");
}

// TODO
//   - refactor using parametrized google tests (TEST_P)
