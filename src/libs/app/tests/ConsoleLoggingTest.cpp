#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/log/Level.hpp>

using antwika::app::ConsoleLogging;
using antwika::log::Level;

TEST(ConsoleLoggingTest, Log_WritesALineToItsStream)
{
    std::ostringstream outputStream;
    ConsoleLogging logging(outputStream, Level::Info);

    logging.logger().log(Level::Info, "running");

    EXPECT_NE(outputStream.str().find("running"), std::string::npos);
}

TEST(ConsoleLoggingTest, Log_DropsWhatIsBelowTheMinimumLevel)
{
    std::ostringstream outputStream;
    ConsoleLogging logging(outputStream, Level::Warning);

    logging.logger().log(Level::Info, "running");

    EXPECT_TRUE(outputStream.str().empty());
}

TEST(ConsoleLoggingTest, Logger_AccumulatesEveryLineOnTheOneStream)
{
    std::ostringstream outputStream;
    ConsoleLogging logging(outputStream, Level::Info);

    logging.logger().log(Level::Info, "first");
    logging.logger().log(Level::Info, "second");

    const auto logText = outputStream.str();
    const auto first = logText.find("first");
    const auto second = logText.find("second");

    ASSERT_NE(first, std::string::npos);
    ASSERT_NE(second, std::string::npos);
    EXPECT_LT(first, second);
}
