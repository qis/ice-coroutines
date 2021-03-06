#include <ice/async.hpp>
#include <ice/net/service.hpp>
#include <gtest/gtest.h>
#include <thread>

// Verifies that the schedule operation works.
TEST(service, schedule)
{
  static ice::net::service c0;
  static ice::net::service c1;
  EXPECT_FALSE(c0.create());
  EXPECT_FALSE(c1.create());

  auto t0 = std::thread([&]() { c0.run(); });
  auto t1 = std::thread([&]() { c1.run(); });

  [&]() -> ice::task {
    EXPECT_NE(std::this_thread::get_id(), t0.get_id());
    EXPECT_NE(std::this_thread::get_id(), t1.get_id());
    EXPECT_FALSE(c0.is_current());
    EXPECT_FALSE(c1.is_current());

    co_await c0.schedule(true);
    EXPECT_EQ(std::this_thread::get_id(), t0.get_id());
    EXPECT_TRUE(c0.is_current());
    EXPECT_FALSE(c1.is_current());

    co_await c1.schedule(true);
    EXPECT_EQ(std::this_thread::get_id(), t1.get_id());
    EXPECT_FALSE(c0.is_current());
    EXPECT_TRUE(c1.is_current());

    co_await c1.schedule();
    EXPECT_EQ(std::this_thread::get_id(), t1.get_id());
    EXPECT_FALSE(c0.is_current());
    EXPECT_TRUE(c1.is_current());

    co_await c0.schedule();
    EXPECT_EQ(std::this_thread::get_id(), t0.get_id());
    EXPECT_TRUE(c0.is_current());
    EXPECT_FALSE(c1.is_current());

    c0.stop();
    c1.stop();
  }();

  t0.join();
  t1.join();
}
