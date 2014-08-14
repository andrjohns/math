#include <stan/agrad/rev/operators/operator_less_than_or_equal.hpp>
#include <test/unit/agrad/util.hpp>
#include <gtest/gtest.h>

TEST(AgradRev,a_lte_b) {
  AVAR a = 5.0;
  AVAR b = 6.0;
  EXPECT_TRUE(a <= b);
  EXPECT_FALSE(b <= a);
  AVAR c = 6.0;
  EXPECT_TRUE(b <= c);
  EXPECT_TRUE(c <= b);
}

TEST(AgradRev,a_lte_y) {
  AVAR a = 5.0;
  double y = 6.0;
  EXPECT_TRUE(a <= y);
  EXPECT_FALSE(y <= a);
  AVAR c = 5.0;
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(c <= a);
}


TEST(AgradRev,x_lte_b) {
  double x = 5.0;
  AVAR b = 6.0;
  EXPECT_TRUE(x <= b);
  EXPECT_FALSE(b <= x);
  double y = 6.0;
  EXPECT_TRUE(b <= y);
  EXPECT_TRUE(y <= b);
}

TEST(AgradRev, logical_lte_nan) {
  stan::agrad::var nan = std::numeric_limits<double>::quiet_NaN();
  stan::agrad::var a = 1.0;
  stan::agrad::var b = 2.0;
  double nan_dbl = std::numeric_limits<double>::quiet_NaN();

  EXPECT_FALSE(1.0 <= nan);
  EXPECT_FALSE(nan <= 2.0);
  EXPECT_FALSE(nan <= nan);
  EXPECT_FALSE(a <= nan);
  EXPECT_FALSE(nan <= b);
  EXPECT_FALSE(a <= nan_dbl);
  EXPECT_FALSE(nan_dbl <= b);
}
