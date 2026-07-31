#include <gtest/gtest.h>

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "antwika/app/RunGuarded.hpp"

using antwika::app::runGuarded;

TEST(RunGuardedTest, RunsTheBodyAndSucceeds)
{
    std::ostringstream errors;
    bool ran = false;

    const int exitCode =
        runGuarded("antwika_test", [&ran] { ran = true; }, errors);

    EXPECT_TRUE(ran);
    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_TRUE(errors.str().empty());
}

TEST(RunGuardedTest, ReportsAFailureUnderTheProgramsName)
{
    std::ostringstream errors;

    const int exitCode = runGuarded(
        "antwika_test",
        [] { throw std::runtime_error("it went wrong"); },
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(errors.str(), "antwika_test: it went wrong\n");
}

TEST(RunGuardedTest, LetsWhatIsNotAnExceptionThrough)
{
    std::ostringstream errors;

    // A throw that is not a std::exception is not a failed run.
    // It is a bug in the body, and hiding it would report success.
    EXPECT_THROW(
        static_cast<void>(
            runGuarded("antwika_test", [] { throw 42; }, errors)),
        int);

    EXPECT_TRUE(errors.str().empty());
}
