#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/testing/ScratchPath.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>

namespace antwika::console::conformance
{

    template <typename Traits>
    class ConsoleSnapshotRoundTripTest : public ConsoleContractTest<Traits>
    {
    };

    TYPED_TEST_SUITE_P(ConsoleSnapshotRoundTripTest);

    TYPED_TEST_P(
        ConsoleSnapshotRoundTripTest, DumpThenLoad_CarriesTheHistoryOver)
    {
        const antwika::testing::ScratchDirectory dir(
            TypeParam::scratchPrefix());
        const auto path = dir.pathIn("dump_state.json");

        const std::vector<std::string> exchange{
            "> dump_state", "dumped state to " + path};

        const auto dumped = this->executed("dump_state", path);

        EXPECT_EQ(TypeParam::console(dumped), exchange);

        auto expected = exchange;
        expected.push_back("loaded state from " + path);

        const auto loaded = this->executed("load_state", path);

        EXPECT_EQ(TypeParam::console(loaded), expected);
    }

    REGISTER_TYPED_TEST_SUITE_P(
        ConsoleSnapshotRoundTripTest, DumpThenLoad_CarriesTheHistoryOver);

}
