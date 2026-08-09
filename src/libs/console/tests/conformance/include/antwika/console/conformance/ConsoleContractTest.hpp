#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/testing/ScratchPath.hpp>

namespace antwika::console::conformance
{

    using antwika::console::testing::keyAt;
    using antwika::console::testing::kOpenTick;
    using antwika::console::testing::typeText;
    using antwika::event::TickEvent;
    using antwika::input::InputEventCodec;
    using antwika::input::Key;

    template <typename Traits>
    class ConsoleContractTest : public ::testing::Test
    {
    protected:
        using Summary = typename Traits::Summary;

        [[nodiscard]] std::vector<TickEvent> lineTyped(
            const std::string_view line) const
        {
            std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
            typeText(events, codec, kOpenTick, line);
            events.push_back(keyAt(codec, kOpenTick, Key::Enter));

            return events;
        }

        [[nodiscard]] Summary executed(
            const std::string_view line,
            const std::string &dumpPath,
            const bool loadEnabled = true) const
        {
            return Traits::run(lineTyped(line), dumpPath, loadEnabled);
        }

        InputEventCodec codec;
    };

    TYPED_TEST_SUITE_P(ConsoleContractTest);

    TYPED_TEST_P(ConsoleContractTest, Execute_EchoesAndRefusesAnUnknownLine)
    {
        const auto summary = this->executed("hello", "unused.json");

        EXPECT_EQ(
            TypeParam::console(summary),
            (std::vector<std::string>{
                "> hello", "unknown command: hello"}));
    }

    TYPED_TEST_P(ConsoleContractTest, Quit_EchoesAndSaysItIsQuitting)
    {
        const auto summary = this->executed("quit", "unused.json");

        EXPECT_EQ(
            TypeParam::console(summary),
            (std::vector<std::string>{"> quit", "quitting"}));
    }

    TYPED_TEST_P(
        ConsoleContractTest, Typing_ReachesNoFieldUntilTheSheetIsOpen)
    {
        std::vector<TickEvent> events{keyAt(this->codec, 1, Key::Grave)};

        typeText(events, this->codec, 3, "hello");
        events.push_back(keyAt(this->codec, 3, Key::Enter));

        events.push_back(keyAt(this->codec, kOpenTick, Key::Enter));

        const auto summary =
            TypeParam::run(std::move(events), "unused.json", true);

        EXPECT_TRUE(TypeParam::console(summary).empty());
    }

    TYPED_TEST_P(
        ConsoleContractTest, LoadState_IsRefusedWhileRecordingOrReplaying)
    {
        const auto summary =
            this->executed("load_state", "unused.json", false);

        EXPECT_EQ(
            TypeParam::console(summary),
            (std::vector<std::string>{
                "> load_state",
                "load_state: not available while recording or "
                "replaying"}));

        TypeParam::expectUntouched(summary);
    }

    TYPED_TEST_P(ConsoleContractTest, LoadState_AnswersAFileThatIsNotThere)
    {
        const antwika::testing::ScratchDirectory dir(
            TypeParam::scratchPrefix());

        const auto summary =
            this->executed("load_state", dir.pathIn("absent.json"));

        const auto &history = TypeParam::console(summary);

        ASSERT_EQ(history.size(), 2U);
        EXPECT_EQ(history[0], "> load_state");
        EXPECT_THAT(
            history[1], ::testing::StartsWith("could not load: "));
    }

    REGISTER_TYPED_TEST_SUITE_P(
        ConsoleContractTest,
        Execute_EchoesAndRefusesAnUnknownLine,
        Quit_EchoesAndSaysItIsQuitting,
        Typing_ReachesNoFieldUntilTheSheetIsOpen,
        LoadState_IsRefusedWhileRecordingOrReplaying,
        LoadState_AnswersAFileThatIsNotThere);

}
