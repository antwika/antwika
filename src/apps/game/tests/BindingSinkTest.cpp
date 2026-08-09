#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/BindingSink.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsFormatError.hpp"
#include "antwika/game/OptionsState.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Action;
using antwika::game::BindingSink;
using antwika::game::bindKeyPayload;
using antwika::game::kDefaultBindings;
using antwika::game::KeyBinding;
using antwika::game::OptionsFormatError;
using antwika::game::OptionsState;
using antwika::input::Key;

namespace
{
    [[nodiscard]] TickEvent bindEvent(KeyBinding binding)
    {
        return TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kBindKey,
                .payload = bindKeyPayload(binding)}};
    }
}

TEST(BindingSinkTest, Handle_FoldsAnAnnouncedBindingIn)
{
    OptionsState options;
    BindingSink sink{options};

    sink.handle(
        bindEvent(KeyBinding{.action = Action::Pause, .key = Key::J}));

    EXPECT_EQ(options.bindings().keyFor(Action::Pause), Key::J);
}

TEST(BindingSinkTest, Handle_AnythingElseIsIgnored)
{
    OptionsState options;
    BindingSink sink{options};

    sink.handle(
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(options.bindings(), kDefaultBindings);
}

TEST(BindingSinkTest, Handle_TakesAnAnnouncementAsNoGesture)
{
    OptionsState options;
    BindingSink sink{options};

    options.await(Action::ZoomIn);

    sink.handle(
        bindEvent(KeyBinding{.action = Action::Pause, .key = Key::J}));

    EXPECT_EQ(options.awaiting(), Action::ZoomIn);
    EXPECT_FALSE(options.notice().has_value());
}

TEST(BindingSinkTest, Handle_RefusesAnUnreadablePayload)
{
    OptionsState options;
    BindingSink sink{options};

    EXPECT_THROW(
        sink.handle(
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::game::events::kBindKey,
                    .payload = "not json"}}),
        OptionsFormatError);
}

TEST(BindingSinkTest, Handle_IgnoresAnAlreadyHeldKey)
{
    OptionsState options;
    BindingSink sink{options};

    sink.handle(
        bindEvent(
            KeyBinding{
                .action = Action::Pause,
                .key = kDefaultBindings.keyFor(Action::ZoomIn)}));

    EXPECT_EQ(options.bindings(), kDefaultBindings);
}
