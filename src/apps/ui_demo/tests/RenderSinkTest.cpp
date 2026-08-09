#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/RenderSink.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Size;
using antwika::time::fakes::FakeSleeper;
using antwika::ui_demo::DemoOverlay;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::RenderSink;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};

    TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

    TEST(RenderSinkTest, Handle_DrawsAFrameOnATickAndPacesIt)
    {
        DemoOverlay overlay(kCanvas);
        const DemoScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);
        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(
            window, scene, overlay, sleeper,
            std::chrono::milliseconds{5});

        sink.handle(tick());
        EXPECT_EQ(sleeper.requested().size(), 1U);
    }

    TEST(RenderSinkTest, Handle_DrawsNothingForANonTick)
    {
        DemoOverlay overlay(kCanvas);
        const DemoScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(true));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window, scene, overlay, sleeper,
            std::chrono::milliseconds{5});

        sink.handle(other());
        EXPECT_TRUE(sleeper.requested().empty());
    }

    TEST(RenderSinkTest, Handle_DrawsNothingIntoAClosedWindow)
    {
        DemoOverlay overlay(kCanvas);
        const DemoScene scene{kTranslator};
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window, scene, overlay, sleeper,
            std::chrono::milliseconds{5});

        sink.handle(tick());
        EXPECT_TRUE(sleeper.requested().empty());
    }

    TEST(DemoOverlayTest, Set_ReplacesWhateverWasThere)
    {
        DemoOverlay overlay(kCanvas);
        EXPECT_EQ(overlay.canvas(), kCanvas);
        EXPECT_TRUE(overlay.commands().empty());

        const DemoScene scene{kTranslator};
        antwika::ui_demo::DemoState state;
        const auto frame =
            scene.describe(kCanvas, {}, {}, state).commands;
        overlay.set(frame);

        EXPECT_FALSE(frame.empty());
        EXPECT_EQ(overlay.commands(), frame);

        overlay.set(antwika::ui::DrawList{});

        EXPECT_TRUE(overlay.commands().empty());
    }
}
