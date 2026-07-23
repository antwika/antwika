#include <gtest/gtest.h>

#include "antwika/time/fakes/FakeClock.hpp"

using antwika::time::FakeClock;

TEST(FakeClockTest, Now) {

    auto now = std::chrono::system_clock::now();
    FakeClock clock{now};
    EXPECT_EQ(clock.now(), now);
}

TEST(FakeClockTest, Advance) {

    auto now = std::chrono::system_clock::now();
    FakeClock clock{now};

    std::chrono::seconds s{5};
    clock.advance(s);
    EXPECT_EQ(clock.now(), now + s);
}

TEST(FakeClockTest, Set) {

    auto now = std::chrono::system_clock::now();
    FakeClock clock{now};

    std::chrono::seconds s{5};
    auto updatedNow = now + std::chrono::seconds{5};
    clock.set(updatedNow);
    EXPECT_EQ(clock.now(), updatedNow);
}
