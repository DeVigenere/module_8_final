#include <gtest/gtest.h>
#include "Calculator.h"
#include <stdexcept>

TEST(CalculatorTest, AddWorks) {
    Calculator c;
    EXPECT_EQ(c.Add(2, 3), 5);
    EXPECT_EQ(c.Add(-2, -3), -5);
    EXPECT_EQ(c.Add(0, 0), 0);
}

TEST(CalculatorTest, SubtractWorks) {
    Calculator c;
    EXPECT_EQ(c.Subtract(10, 4), 6);
    EXPECT_EQ(c.Subtract(0, 5), -5);
}

TEST(CalculatorTest, MultiplyWorks) {
    Calculator c;
    EXPECT_EQ(c.Multiply(3, 4), 12);
    EXPECT_EQ(c.Multiply(-3, 4), -12);
    EXPECT_EQ(c.Multiply(0, 100), 0);
}

TEST(CalculatorTest, DivideWorks) {
    Calculator c;
    EXPECT_DOUBLE_EQ(c.Divide(10, 2), 5.0);
    EXPECT_DOUBLE_EQ(c.Divide(1, 3), 1.0 / 3.0);
}

TEST(CalculatorTest, DivideByZeroThrows) {
    Calculator c;
    EXPECT_THROW(c.Divide(1, 0), std::invalid_argument);
}

TEST(CalculatorTest, IsEven) {
    Calculator c;
    EXPECT_TRUE(c.IsEven(0));
    EXPECT_TRUE(c.IsEven(2));
    EXPECT_TRUE(c.IsEven(-4));
    EXPECT_FALSE(c.IsEven(3));
    EXPECT_FALSE(c.IsEven(-1));
}