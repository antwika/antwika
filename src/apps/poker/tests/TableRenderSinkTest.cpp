#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/Events.hpp"
#include "antwika/poker/TableRenderSink.hpp"
#include "antwika/poker/TableScene.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::holdem::Blinds;
using antwika::holdem::Table;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::TableRenderSink;
using antwika::poker::TableScene;
using antwika::time::fakes::FakeSleeper;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using namespace std::chrono_literals;

namespace
{
    constexpr auto kFramePeriod = 80ms;

    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] TickEvent tickAt(antwika::time::Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick},
        };
    }
} // namespace

class TableRenderSinkTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
        ON_CALL(window, size())
            .WillByDefault(
                Return(Size{.width = 1024, .height = 640}));

        EXPECT_CALL(renderer, clear(_)).Times(AnyNumber());
        EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
        EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(AnyNumber());
    }

    NiceMock<MockWindow> window;
    NiceMock<MockRenderer> renderer;
    Table table{3, Blinds{.small = 5, .big = 10}};
    BankrollLedger ledger;
    CashGame game{table, ledger, 100};
    TableScene scene;
    FakeSleeper sleeper;

    TableRenderSink sink{
        window,
        kCanvas,
        scene,
        table,
        game,
        sleeper,
        kFramePeriod,
        "Antwika"};
};

TEST_F(TableRenderSinkTest, Handle_DrawsAndPresentsOneFramePerTick)
{
    EXPECT_CALL(renderer, present()).Times(2);

    sink.handle(tickAt(0));
    sink.handle(tickAt(1));
}

TEST_F(TableRenderSinkTest, Handle_HoldsEachFrameForTheFramePeriod)
{
    sink.handle(tickAt(0));
    sink.handle(tickAt(1));

    EXPECT_EQ(
        sleeper.requested(),
        (std::vector<std::chrono::milliseconds>{kFramePeriod, kFramePeriod}));
}

TEST_F(TableRenderSinkTest, Handle_DrawsNothingForAnEventThatIsNotATick)
{
    EXPECT_CALL(renderer, present()).Times(0);

    sink.handle(
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::poker::events::kBuyIn,
                .payload = R"({"player":"alice","amount":200})"}});

    EXPECT_TRUE(sleeper.requested().empty());
}

TEST_F(TableRenderSinkTest, Handle_DrawsNothingOnceTheWindowIsClosed)
{
    ON_CALL(window, isOpen()).WillByDefault(Return(false));

    EXPECT_CALL(renderer, present()).Times(0);

    sink.handle(tickAt(0));

    // A closed window is not paced either.
    // Which is why nothing is left looping on one.
    EXPECT_TRUE(sleeper.requested().empty());
}

TEST_F(TableRenderSinkTest, Render_DrawsWithoutBeingHandedAnEvent)
{
    EXPECT_CALL(renderer, present()).Times(1);

    sink.render();
}

TEST_F(TableRenderSinkTest, Render_ShowsTheTableItWasGiven)
{
    ledger.deposit("alice", 500);
    static_cast<void>(game.buyIn("alice", 300));

    // The frame comes from the live table, not from a stale copy.
    EXPECT_CALL(renderer, drawText(_, "alice", _, _));
    EXPECT_CALL(renderer, drawText(_, "300", _, _));
    EXPECT_CALL(renderer, present());

    sink.render();
}

// The console's sheet goes over the table, when one is mounted.
// Described in the tick path; this sink only paints what it is handed.
TEST_F(TableRenderSinkTest, Render_PaintsTheConsolePictureLast)
{
    const antwika::gfx::Rect sheet{
        .origin = {.x = 1, .y = 2},
        .size = {.width = 3, .height = 4}};
    const antwika::gfx::Color ink{.red = 5, .green = 6, .blue = 7};

    antwika::console::ConsolePicture picture(kCanvas);
    picture.set({antwika::ui::FillRect{.rect = sheet, .color = ink}});

    TableRenderSink overlaid{
        window,
        kCanvas,
        scene,
        table,
        game,
        sleeper,
        kFramePeriod,
        "Antwika",
        std::nullopt,
        picture};

    EXPECT_CALL(renderer, drawRect(sheet, ink)).Times(1);
    EXPECT_CALL(renderer, present()).Times(1);

    overlaid.render();
}

// The size the window was asked for, never the size it reports.
// A layout is a function of its canvas.
// The art is a function of that layout.
// A frame described against a resized window would misplace both.
TEST_F(TableRenderSinkTest, Render_LaysOutAgainstTheConfiguredSize)
{
    NiceMock<MockWindow> small;
    ON_CALL(small, isOpen()).WillByDefault(Return(true));
    ON_CALL(small, renderer()).WillByDefault(ReturnRef(renderer));

    // Wildly bigger than the canvas, and a different glyph scale.
    ON_CALL(small, size())
        .WillByDefault(Return(Size{.width = 1920, .height = 1200}));

    TableRenderSink sink{
        small,
        Size{.width = 320, .height = 200},
        scene,
        table,
        game,
        sleeper,
        kFramePeriod,
        "Antwika"};

    // One pixel per glyph pixel is what 200 rows asks for.
    EXPECT_CALL(renderer, drawText(_, _, 1, _)).Times(AtLeast(1));
    EXPECT_CALL(renderer, drawText(_, _, 3, _)).Times(0);

    sink.render();
}
