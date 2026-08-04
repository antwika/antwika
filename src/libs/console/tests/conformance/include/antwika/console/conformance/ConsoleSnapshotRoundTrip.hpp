#pragma once

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/testing/ScratchPath.hpp>

#include <antwika/console/conformance/ConsoleContract.hpp>

namespace antwika::console::conformance
{

    /**
     * @brief What dump_state and load_state promise about the console
     * itself, as one test suite.
     *
     * The envelope carries the history so that coming back to an
     * instant means reading what that instant read, and every
     * application with both commands had said so in its own words.
     * What the state member holds is each application's own business
     * and stays in each application's own file; this is the console
     * half alone, which is the same wherever it is mounted.
     *
     * **It is registered apart from ConsoleContract because one
     * application has no load to come back through.**
     * apps/poker dumps a room mid-hand and does not read one back, so
     * it instantiates the contract and not this; weakening the round
     * trip until poker could pass it would have cost the other eight
     * the assertion that matters.
     *
     * A Traits is ConsoleContract's, unchanged.
     */
    template <typename Traits>
    class ConsoleSnapshotRoundTrip : public ConsoleContract<Traits>
    {
    };

    TYPED_TEST_SUITE_P(ConsoleSnapshotRoundTrip);

    TYPED_TEST_P(
        ConsoleSnapshotRoundTrip, DumpThenLoad_CarriesTheHistoryOver)
    {
        // A directory rather than a file.
        // An application may write more beside the document.
        const antwika::testing::ScratchDirectory dir(
            TypeParam::scratchPrefix());
        const auto path = dir.pathIn("dump_state.json");

        // The answer is pushed before the state is taken.
        // So the dump carries the whole exchange that made it.
        const std::vector<std::string> exchange{
            "> dump_state", "dumped state to " + path};

        const auto dumped = this->executed("dump_state", path);

        EXPECT_EQ(TypeParam::console(dumped), exchange);

        // A load replaces the history rather than appending to it.
        // So the "> load_state" that asked for it is gone.
        auto expected = exchange;
        expected.push_back("loaded state from " + path);

        const auto loaded = this->executed("load_state", path);

        EXPECT_EQ(TypeParam::console(loaded), expected);
    }

    REGISTER_TYPED_TEST_SUITE_P(
        ConsoleSnapshotRoundTrip, DumpThenLoad_CarriesTheHistoryOver);

} // namespace antwika::console::conformance
