#include <gtest/gtest.h>

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "antwika/app/RunCatchingErrors.hpp"

using antwika::app::runCatchingErrors;

TEST(RunCatchingErrorsTest, RunCatchingErrors_RunsTheBodyAndSucceeds)
{
    std::ostringstream errors;
    bool ran = false;

    const int exitCode =
        runCatchingErrors("antwika_test", [&ran] { ran = true; }, errors);

    EXPECT_TRUE(ran);
    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_TRUE(errors.str().empty());
}

TEST(RunCatchingErrorsTest, RunCatchingErrors_ReportsAFailureUnderTheName)
{
    std::ostringstream errors;

    const int exitCode = runCatchingErrors(
        "antwika_test",
        [] { throw std::runtime_error("it went wrong"); },
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(errors.str(), "antwika_test: it went wrong\n");
}

TEST(RunCatchingErrorsTest, RunCatchingErrors_LetsANonExceptionThrough)
{
    std::ostringstream errors;

    EXPECT_THROW(
        static_cast<void>(
            runCatchingErrors("antwika_test", [] { throw 42; }, errors)),
        int);

    EXPECT_TRUE(errors.str().empty());
}
