#include <gtest/gtest.h>
#include <stdexcept>

#include "canvas.hpp"

// Cell

// basic

TEST(Cell, CanBeConstructed) { Cell{"x"}; }

TEST(Cell, CanCompare)
{
  EXPECT_TRUE(Cell{"x"} == Cell{"x"});
  EXPECT_FALSE(Cell{"x"} == Cell{"y"});
}

// input validation

TEST(Cell, CanCreateWithMaxBytes)
{
  const std::string s = std::string(Cell::CELL_CHAR_SIZE, 'x');
  EXPECT_NO_THROW(Cell{s});
}

TEST(Cell, ThrowsOnTooManyBytes)
{
  const std::string s = std::string(Cell::CELL_CHAR_SIZE + 1, 'x');
  EXPECT_THROW(Cell{s}, std::invalid_argument);
}

// functionality

TEST(Cell, WritesZeroToUnusedBytes)
{
  Cell c{"x"};
  EXPECT_EQ(c.bytes[0], 'x');
  for (size_t i = 1; i <= Cell::CELL_CHAR_SIZE; ++i)
  {
    EXPECT_EQ(c.bytes[i], '\0');
  }
}

// TODO: Cell: ThrowsOnMultipleCodePoints

// Canvas

// basic

TEST(Canvas, CanBeConstructedWithDimensions)
{
  Canvas c1(1, 1);
  Canvas c2(3, 5);
}

// input validation

TEST(Canvas, ThrowsOnZeroRowCount) { EXPECT_THROW(Canvas c(0, 5), std::invalid_argument); }

TEST(Canvas, ThrowsOnZeroColCount) { EXPECT_THROW(Canvas c(3, 0), std::invalid_argument); }

// initialization

TEST(Canvas, ReturnsRowCount)
{
  Canvas c(3, 5);
  EXPECT_EQ(c.row_count(), 3);
}

TEST(Canvas, ReturnsColCount)
{
  Canvas c(3, 5);
  EXPECT_EQ(c.col_count(), 5);
}

TEST(Canvas, DefaultInitializedToWhitespace)
{
  Canvas c(2, 2);
  EXPECT_EQ(c(0, 0), Cell{" "});
  EXPECT_EQ(c(0, 1), Cell{" "});
  EXPECT_EQ(c(1, 0), Cell{" "});
  EXPECT_EQ(c(1, 1), Cell{" "});
}

// access operations

TEST(Canvas, ThrowsOnOutOfRangeCellAccess)
{
  Canvas c(2, 2);

  EXPECT_THROW(c(3, 0), std::out_of_range);
  EXPECT_THROW(c(0, 3), std::out_of_range);
}

TEST(Canvas, CanReadWriteCells)
{
  Canvas c(2, 2);

  c(0, 0) = Cell{"A"};
  c(0, 1) = Cell{"B"};
  c(1, 0) = Cell{"C"};
  c(1, 1) = Cell{"D"};

  EXPECT_EQ(c(0, 0), Cell{"A"});
  EXPECT_EQ(c(0, 1), Cell{"B"});
  EXPECT_EQ(c(1, 0), Cell{"C"});
  EXPECT_EQ(c(1, 1), Cell{"D"});
}

TEST(Canvas, ThrowsOnOutOfRangeRowAccess)
{
  Canvas c(2, 2);

  EXPECT_THROW(c.row_begin(3), std::out_of_range);
  EXPECT_THROW(c.row_end(3), std::out_of_range);
}

TEST(Canvas, CanReadWriteRows)
{
  Canvas c(2, 2);

  const auto row0 = std::vector<Cell>{Cell{"A"}, Cell{"B"}};
  std::copy(row0.begin(), row0.end(), c.row_begin(0));

  const auto row1 = std::vector<Cell>{Cell{"C"}, Cell{"D"}};
  std::copy(row1.begin(), row1.end(), c.row_begin(1));

  EXPECT_EQ(c(0, 0), Cell{"A"});
  EXPECT_EQ(c(0, 1), Cell{"B"});
  EXPECT_EQ(c(1, 0), Cell{"C"});
  EXPECT_EQ(c(1, 1), Cell{"D"});
}

// write operations

TEST(Canvas, CopyNWritesString)
{
  Canvas c(3, 3);
  const std::string row1 = "abc";
  const std::string row2 = "yz";

  c.copy_n(0, 0, row1);
  c.copy_n(2, 1, row2);

  EXPECT_EQ(c(0, 0), Cell{"a"});
  EXPECT_EQ(c(0, 1), Cell{"b"});
  EXPECT_EQ(c(0, 2), Cell{"c"});
  EXPECT_EQ(c(1, 0), Cell{" "});
  EXPECT_EQ(c(1, 1), Cell{" "});
  EXPECT_EQ(c(1, 2), Cell{" "});
  EXPECT_EQ(c(2, 0), Cell{" "});
  EXPECT_EQ(c(2, 1), Cell{"y"});
  EXPECT_EQ(c(2, 2), Cell{"z"});
}

TEST(Canvas, CopyNClampsOnCountExceedsSrcSize)
{
  Canvas c(1, 3);
  const std::string row = "abc";

  c.copy_n(0, 0, row, 4);
  EXPECT_EQ(c(0, 0), Cell{"a"});
  EXPECT_EQ(c(0, 1), Cell{"b"});
  EXPECT_EQ(c(0, 2), Cell{"c"});
}

TEST(Canvas, CopyNThrowsOnStringExceedsColCount)
{
  Canvas c(3, 3);
  const std::string row = "abcd";

  EXPECT_THROW(c.copy_n(0, 0, row), std::out_of_range);
}
