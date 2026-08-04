#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    /**
     * @brief Every promise the console makes whatever application
     * mounts it, as one test suite.
     *
     * Nine applications mount the same console, and each had written
     * out the same four cases against its own bootstrap: an unknown
     * line is echoed and refused, a key pressed while the sheet is
     * still sliding types nowhere, a load is refused under --record
     * and --replay, and a load answers a file that is not there.
     * Nothing in any of the four is about a city, a board or a hand;
     * they are the library's own behaviour, seen through whichever
     * application happens to be underneath.
     * Copied per application, the wording of a refusal was nine
     * places that could drift apart -- and a tenth application would
     * have inherited nothing.
     *
     * A Traits supplies the application's half, and only that half:
     * its Summary type, a run() that takes a script and hands back
     * one, the console history out of a Summary, an expectUntouched()
     * saying what a refused load must have left alone, and a
     * scratchPrefix() naming its temporary files.
     * run() appends the application's own stop or tick limit, which
     * is what keeps nine differently-shaped bootstraps behind one
     * call here.
     *
     * The round trip through dump_state and load_state is registered
     * separately, as ConsoleSnapshotRoundTrip, because one
     * application has no load to round trip through.
     */
    template <typename Traits>
    class ConsoleContract : public ::testing::Test
    {
    protected:
        using Summary = typename Traits::Summary;

        /**
         * @brief Script the toggle, then one line typed and entered.
         * @param line What to type once the sheet stands fully open.
         * @return The script, short of whatever ends the run.
         *
         * Every character of the line lands on the tick its Enter
         * does, which is what ConsoleScript.hpp's typeText() is for.
         */
        [[nodiscard]] std::vector<TickEvent> lineTyped(
            const std::string_view line) const
        {
            std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
            typeText(events, codec, kOpenTick, line);
            events.push_back(keyAt(codec, kOpenTick, Key::Enter));

            return events;
        }

        /**
         * @brief Run one typed line through the application.
         * @param line What to type once the sheet stands fully open.
         * @param dumpPath Where its snapshot commands read and write.
         * @param loadEnabled Whether a load is permitted at all.
         * @return Whatever that application's bootstrap answered.
         */
        [[nodiscard]] Summary executed(
            const std::string_view line,
            const std::string &dumpPath,
            const bool loadEnabled = true) const
        {
            return Traits::run(lineTyped(line), dumpPath, loadEnabled);
        }

        /**
         * @brief Stateless, so a script may be encoded by any copy.
         */
        InputEventCodec codec;
    };

    TYPED_TEST_SUITE_P(ConsoleContract);

    TYPED_TEST_P(ConsoleContract, Execute_EchoesAndRefusesAnUnknownLine)
    {
        const auto summary = this->executed("hello", "unused.json");

        EXPECT_EQ(
            TypeParam::console(summary),
            (std::vector<std::string>{
                "> hello", "unknown command: hello"}));
    }

    TYPED_TEST_P(
        ConsoleContract, Typing_ReachesNoFieldUntilTheSheetIsOpen)
    {
        std::vector<TickEvent> events{keyAt(this->codec, 1, Key::Grave)};

        // Half way along the slide, none of this may land.
        typeText(events, this->codec, 3, "hello");
        events.push_back(keyAt(this->codec, 3, Key::Enter));

        // Fully open, the field is still empty, so Enter says nothing.
        events.push_back(keyAt(this->codec, kOpenTick, Key::Enter));

        const auto summary =
            TypeParam::run(std::move(events), "unused.json", true);

        EXPECT_TRUE(TypeParam::console(summary).empty());
    }

    // A load reads a file no recording carries.
    // So a recorded run answers with a history line instead.
    TYPED_TEST_P(
        ConsoleContract, LoadState_IsRefusedWhileRecordingOrReplaying)
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

    TYPED_TEST_P(ConsoleContract, LoadState_AnswersAFileThatIsNotThere)
    {
        // The directory is made and the file inside it never is.
        // So the path is one nothing else could have written first.
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
        ConsoleContract,
        Execute_EchoesAndRefusesAnUnknownLine,
        Typing_ReachesNoFieldUntilTheSheetIsOpen,
        LoadState_IsRefusedWhileRecordingOrReplaying,
        LoadState_AnswersAFileThatIsNotThere);

} // namespace antwika::console::conformance
