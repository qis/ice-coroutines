#include <ice/error.hpp>
#include <ice/result.hpp>
#include <gtest/gtest.h>

class test {
public:
  test(int) {}
};

TEST(result, types)
{
  ice::result<int> v0{};
  const auto v1 = v0;
  const auto v2 = std::move(v0);

  //EXPECT_TRUE(ice::result<int>{});
  //EXPECT_TRUE(ice::result<int>{ 0 });
  //EXPECT_FALSE(ice::result<int>{ std::error_code{} });
  //EXPECT_FALSE(ice::result<int>{ ice::make_error_code(33) });
}

TEST(result, value) {}

TEST(result, error) {}
