#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/log/Level.hpp>

using antwika::app::ConsoleLogging;
using antwika::log::Level;

TEST(ConsoleLoggingTest, WritesALoggedLineToItsStream)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Info);

    logging.logger().log(Level::Info, "running");

    EXPECT_NE(out.str().find("running"), std::string::npos);
}

TEST(ConsoleLoggingTest, DropsWhatIsBelowTheMinimumLevel)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Warning);

    logging.logger().log(Level::Info, "running");

    EXPECT_TRUE(out.str().empty());
}

TEST(ConsoleLoggingTest, IsOneLoggerOverOneAppender)
{
    std::ostringstream out;
    ConsoleLogging logging(out, Level::Info);

    EXPECT_EQ(&logging.logger(), &logging.logger());
}
