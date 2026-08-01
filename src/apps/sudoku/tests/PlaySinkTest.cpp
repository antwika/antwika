#include <optional>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include <antwika/sudoku/Board.hpp>
#include <antwika/sudoku/BoardOverlay.hpp>
#include <antwika/sudoku/PlaySink.hpp>
#include <antwika/sudoku/PuzzleFile.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/Status.hpp>
#include <antwika/sudoku/SudokuScene.hpp>
#include <antwika/sudoku/Widgets.hpp>

#include "WidgetCentre.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::sudoku::Board;
using antwika::sudoku::BoardOverlay;
using antwika::sudoku::kDemoPuzzle;
using antwika::sudoku::PlaySink;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::Square;
using antwika::sudoku::Status;
using antwika::sudoku::SudokuScene;
using antwika::sudoku::tests::squareCentre;
using antwika::sudoku::tests::widgetCentre;
namespace widgets = antwika::sudoku::widgets;

namespace
{
    constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};

    // The first blank square of the demo puzzle, and a clue.
    constexpr Square kBlank{.row = 0, .col = 2};
    constexpr Square kClue{.row = 0, .col = 0};

    /**
     * @brief Everything one sink needs, in one place.
     *
     * A struct rather than a fixture, because the collaborators have to
     * outlive the sink and be reachable from the assertions.
     */
    struct Session
    {
        PuzzleState state;
        BoardOverlay overlay{kCanvas};
        InputEventCodec codec;
        SudokuScene scene{kTranslator};

        Session()
        {
            state.start(Board::parse(kDemoPuzzle));
        }

        [[nodiscard]] Point centreOf(const Square square) const
        {
            const auto found = squareCentre(
                scene.describe(kCanvas, {}, state), square);
            EXPECT_TRUE(found.has_value());
            return found.value_or(Point{});
        }

        [[nodiscard]] Point solveButton() const
        {
            const auto found = widgetCentre(
                scene.describe(kCanvas, {}, state), widgets::kSolve);
            EXPECT_TRUE(found.has_value());
            return found.value_or(Point{});
        }

        [[nodiscard]] TickEvent pressAt(
            const Point at, const antwika::time::Tick when = 0) const
        {
            return TickEvent{
                .tick = when,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = at.x, .y = at.y}})};
        }

        [[nodiscard]] TickEvent keyPress(
            const Key key,
            const bool repeat = false,
            const antwika::time::Tick when = 0) const
        {
            return TickEvent{
                .tick = when,
                .event = codec.encode(KeyPressed{
                    .key = key, .modifiers = {}, .repeat = repeat})};
        }
    };

    [[nodiscard]] TickEvent tickAt(const antwika::time::Tick when)
    {
        return TickEvent{
            .tick = when,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TEST(PlaySinkTest, Handle_PicksTheSquareUnderAPress)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.pressAt(session.centreOf(kBlank)));

        EXPECT_EQ(session.state.selected(), std::optional{kBlank});
        EXPECT_EQ(session.state.status(), Status::Playing);
    }

    TEST(PlaySinkTest, Handle_SaysSoWhenThePressLandsOnAClue)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.pressAt(session.centreOf(kClue)));

        EXPECT_EQ(session.state.selected(), std::optional{kClue});
        EXPECT_EQ(session.state.status(), Status::GivenLocked);
    }

    TEST(PlaySinkTest, Handle_PicksNothingForAPressBesideTheGrid)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        // Inside the board's own area, but beside the grid itself.
        // Which is the margin left over once it is made square.
        const auto area =
            session.scene.describe(kCanvas, {}, session.state)
                .rects.find(widgets::kBoard)
                .value();

        sink.handle(session.pressAt(Point{
            .x = area.origin.x,
            .y = area.origin.y
                 + static_cast<std::int32_t>(area.size.height) / 2}));

        EXPECT_FALSE(session.state.selected().has_value());
    }

    TEST(PlaySinkTest, Handle_PicksNothingForAButtonThatIsNotTheLeft)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(TickEvent{
            .tick = 0,
            .event = session.codec.encode(PointerButtonPressed{
                .button = MouseButton::Right,
                .position = {
                    .x = session.centreOf(kBlank).x,
                    .y = session.centreOf(kBlank).y}})});

        EXPECT_FALSE(session.state.selected().has_value());
    }

    TEST(PlaySinkTest, Handle_WritesATypedDigitIntoThePickedSquare)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.pressAt(session.centreOf(kBlank)));
        sink.handle(session.keyPress(Key::Digit4));

        EXPECT_EQ(session.state.board().at(0, 2), std::optional{4});

        sink.handle(session.keyPress(Key::Backspace));

        EXPECT_FALSE(session.state.board().at(0, 2).has_value());
    }

    TEST(PlaySinkTest, Handle_StepsOverAHeldKeyRepeatingItself)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.pressAt(session.centreOf(kBlank)));
        sink.handle(session.keyPress(Key::Digit4));
        sink.handle(session.keyPress(Key::Backspace, true));

        EXPECT_EQ(session.state.board().at(0, 2), std::optional{4});
    }

    TEST(PlaySinkTest, Handle_IgnoresAKeyWithNoMeaningHere)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.keyPress(Key::Q));

        EXPECT_EQ(session.state.board().format(), kDemoPuzzle);
    }

    TEST(PlaySinkTest, Handle_FinishesTheGridOnAPressOfSolve)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(session.pressAt(session.solveButton()));

        EXPECT_EQ(session.state.status(), Status::Solved);
        EXPECT_EQ(session.state.filled(), 81U);
    }

    TEST(PlaySinkTest, Handle_SaysSoWhenASolveCannotSucceed)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        // A second 5 in the top-left box, then ask for the rest.
        sink.handle(session.pressAt(session.centreOf(kBlank)));
        sink.handle(session.keyPress(Key::Digit5));
        sink.handle(session.pressAt(session.solveButton()));

        EXPECT_EQ(session.state.status(), Status::Unsolvable);
        EXPECT_EQ(session.state.filled(), 31U);
    }

    TEST(PlaySinkTest, Handle_DrawsThePictureAfreshOnEveryTick)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        EXPECT_TRUE(session.overlay.commands().empty());

        sink.handle(tickAt(0));

        EXPECT_FALSE(session.overlay.commands().empty());

        const auto quiet = session.overlay.commands();
        sink.handle(session.pressAt(session.centreOf(kBlank), 1));

        EXPECT_NE(session.overlay.commands(), quiet);
    }

    TEST(PlaySinkTest, Handle_IgnoresAnEventItDoesNotUnderstand)
    {
        Session session;
        PlaySink sink(
            session.state,
            session.overlay,
            session.codec,
            session.scene);

        sink.handle(TickEvent{
            .tick = 0, .event = Event{.name = "sudoku.solve"}});

        EXPECT_TRUE(session.overlay.commands().empty());
        EXPECT_EQ(session.state.status(), Status::Playing);
    }
} // namespace
