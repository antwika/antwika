#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/Events.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleSource.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::ReplaySource;
using antwika::sudoku::Board;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::newPuzzleEvent;
using antwika::sudoku::PuzzleSource;

namespace
{
    [[nodiscard]] std::vector<TickEvent> scripted()
    {
        return {
            TickEvent{
                .tick = 0,
                .event = Event{.name = "sudoku.solve", .payload = ""}},
            TickEvent{
                .tick = 1,
                .event = Event{.name = "sudoku.solve", .payload = ""}}};
    }

    TEST(PuzzleSourceTest, NewPuzzleEvent_CarriesTheGridAsItStands)
    {
        const auto event = newPuzzleEvent(Board::parse(kDemoPuzzle));

        EXPECT_EQ(event.name, antwika::sudoku::events::kNewPuzzle);
        EXPECT_EQ(
            nlohmann::json::parse(event.payload)
                .at("cells")
                .get<std::string>(),
            kDemoPuzzle);
    }

    TEST(PuzzleSourceTest, EventsFor_AnnouncesThePuzzleBeforeAnythingElse)
    {
        ReplaySource inner(scripted());
        PuzzleSource source(inner, Board::parse(kDemoPuzzle));

        const auto first = source.eventsFor(0);

        ASSERT_EQ(first.size(), 2U);
        EXPECT_EQ(first[0].name, antwika::sudoku::events::kNewPuzzle);
        EXPECT_EQ(first[1].name, "sudoku.solve");

        const auto second = source.eventsFor(1);

        ASSERT_EQ(second.size(), 1U);
        EXPECT_EQ(second[0].name, "sudoku.solve");
    }

    TEST(PuzzleSourceTest, EventsFor_AnnouncesNothingWithNoPuzzle)
    {
        ReplaySource inner(scripted());
        PuzzleSource source(inner, std::nullopt);

        const auto first = source.eventsFor(0);

        ASSERT_EQ(first.size(), 1U);
        EXPECT_EQ(first[0].name, "sudoku.solve");
    }
}
