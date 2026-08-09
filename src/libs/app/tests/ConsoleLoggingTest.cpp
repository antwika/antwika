#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/log/Level.hpp>

using antwika::app::ConsoleLogging;
using antwika::log::Level;

TEST(ConsoleLoggingTest, Log_WritesALineToItsStream)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Info);

    logging.logger().log(Level::Info, "running");

    EXPECT_NE(out.str().find("running"), std::string::npos);
}

TEST(ConsoleLoggingTest, Log_DropsWhatIsBelowTheMinimumLevel)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Warning);

    logging.logger().log(Level::Info, "running");

    EXPECT_TRUE(out.str().empty());
}

TEST(ConsoleLoggingTest, Logger_AccumulatesEveryLineOnTheOneStream)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Info);

    logging.logger().log(Level::Info, "first");
    logging.logger().log(Level::Info, "second");

    const auto written = out.str();
    const auto first = written.find("first");
    const auto second = written.find("second");

    ASSERT_NE(first, std::string::npos);
    ASSERT_NE(second, std::string::npos);
    EXPECT_LT(first, second);
}
