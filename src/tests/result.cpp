#include <ice/error.hpp>
#include <ice/result.hpp>
#include <gtest/gtest.h>

TEST(result, types)
{
  EXPECT_FALSE(ice::result<int>{});
  EXPECT_FALSE(ice::result<int>{ std::error_code{} });
  EXPECT_FALSE(ice::result<int>{ ice::make_error_code(33) });
  EXPECT_TRUE(ice::result<int>{ 0 });
}

TEST(result, value) {}

TEST(result, error) {}
