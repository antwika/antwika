#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>
#include <antwika/sudoku/BoardOverlay.hpp>
#include <antwika/sudoku/PuzzleState.hpp>
#include <antwika/sudoku/RenderSink.hpp>
#include <antwika/sudoku/SudokuScene.hpp>

#include "antwika/sudoku/Messages.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Size;
using antwika::sudoku::BoardOverlay;
using antwika::sudoku::PuzzleState;
using antwika::sudoku::RenderSink;
using antwika::sudoku::SudokuScene;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr antwika::sudoku::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 720, .height = 800};

    const antwika::console::ConsolePicture kNoConsole{};

    [[nodiscard]] TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

    TEST(RenderSinkTest, Handle_DrawsAFrameOnATickAndPacesIt)
    {
        BoardOverlay overlay(kCanvas);
        const SudokuScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);
        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(
            window,
            scene,
            overlay,
            kNoConsole,
            sleeper,
            std::chrono::milliseconds{5});

        sink.handle(tick());
        EXPECT_EQ(sleeper.requested().size(), 1U);
    }

    TEST(RenderSinkTest, Handle_DrawsNothingForANonTick)
    {
        BoardOverlay overlay(kCanvas);
        const SudokuScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window,
            scene,
            overlay,
            kNoConsole,
            sleeper,
            std::chrono::milliseconds{5});

        sink.handle(other());
        EXPECT_TRUE(sleeper.requested().empty());
    }

    TEST(RenderSinkTest, Handle_DrawsNothingIntoAClosedWindow)
    {
        BoardOverlay overlay(kCanvas);
        const SudokuScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window,
            scene,
            overlay,
            kNoConsole,
            sleeper,
            std::chrono::milliseconds{5});

        sink.handle(tick());
        EXPECT_TRUE(sleeper.requested().empty());
    }

    TEST(BoardOverlayTest, Set_ReplacesWhateverWasThere)
    {
        BoardOverlay overlay(kCanvas);
        EXPECT_EQ(overlay.canvas(), kCanvas);
        EXPECT_TRUE(overlay.commands().empty());

        const SudokuScene scene{kTranslator};
        const PuzzleState state;
        overlay.set(scene.describe(kCanvas, {}, state).commands);

        EXPECT_FALSE(overlay.commands().empty());

        overlay.set({});

        EXPECT_TRUE(overlay.commands().empty());
    }
}
