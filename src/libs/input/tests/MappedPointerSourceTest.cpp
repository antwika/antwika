#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/fakes/FakeHalvingPointerMapping.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/MappedPointerSource.hpp"
#include "antwika/input/IPointerMapping.hpp"
#include "antwika/input/InputError.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::IPointerMapping;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MappedPointerSource;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;
using antwika::input::fakes::FakeHalvingPointerMapping;
using antwika::replay::ReplaySource;
using antwika::time::Tick;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] TickEvent at(Tick tick, Event event)
    {
        return TickEvent{.tick = tick, .event = std::move(event)};
    }

    [[nodiscard]] std::vector<InputEvent> decodedFrom(
        const std::vector<Event> &events)
    {
        std::vector<InputEvent> decoded;

        for (const auto &event : events)
        {
            if (const auto edge = kCodec.decode(event))
            {
                decoded.push_back(*edge);
            }
        }

        return decoded;
    }
}

TEST(MappedPointerSourceTest, EventsFor_MapsEveryPositionalEdge)
{
    ReplaySource inner(
        {at(0, kCodec.encode(PointerMoved{.position = {.x = 40, .y = 20}})),
         at(0,
            kCodec.encode(
                PointerButtonPressed{
                    .button = MouseButton::Right,
                    .position = {.x = 10, .y = 6}})),
         at(0,
            kCodec.encode(
                PointerButtonReleased{
                    .button = MouseButton::Right,
                    .position = {.x = 12, .y = 8}}))});

    const FakeHalvingPointerMapping mapping;
    MappedPointerSource source(inner, kCodec, mapping);

    EXPECT_EQ(
        decodedFrom(source.eventsFor(0)),
        (std::vector<InputEvent>{
            PointerMoved{.position = {.x = 20, .y = 10}},
            PointerButtonPressed{
                .button = MouseButton::Right,
                .position = {.x = 5, .y = 3}},
            PointerButtonReleased{
                .button = MouseButton::Right,
                .position = {.x = 6, .y = 4}}}));
}

TEST(MappedPointerSourceTest, EventsFor_LeavesAPositionlessEdgeAlone)
{
    const auto key = kCodec.encode(KeyPressed{.key = Key::F10});
    const auto scroll = kCodec.encode(PointerScrolled{.vertical = 1});

    ReplaySource inner({at(0, key), at(0, scroll)});

    const FakeHalvingPointerMapping mapping;
    MappedPointerSource source(inner, kCodec, mapping);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{key, scroll}));
}

TEST(MappedPointerSourceTest, EventsFor_LeavesAnUnrelatedEventAlone)
{
    const Event other{.name = "game.score_increment", .payload = "{}"};

    ReplaySource inner({at(0, other)});

    const FakeHalvingPointerMapping mapping;
    MappedPointerSource source(inner, kCodec, mapping);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{other}));
}

TEST(MappedPointerSourceTest, EventsFor_KeepsTheStreamsShapeExactly)
{
    ReplaySource inner(
        {at(0, kCodec.encode(PointerMoved{.position = {.x = 2, .y = 2}})),
         at(1, kCodec.encode(KeyPressed{.key = Key::A})),
         at(1, kCodec.encode(PointerMoved{.position = {.x = 4, .y = 4}}))});

    const FakeHalvingPointerMapping mapping;
    MappedPointerSource source(inner, kCodec, mapping);

    EXPECT_EQ(source.eventsFor(0).size(), 1U);
    EXPECT_EQ(source.eventsFor(1).size(), 2U);
    EXPECT_TRUE(source.eventsFor(2).empty());
}

TEST(MappedPointerSourceTest, EventsFor_LetsABadPayloadThrough)
{
    ReplaySource inner(
        {at(0,
            antwika::event::Event{
                .name = "input.pointer_move",
                .payload = R"({"x":"far","y":2})"})});

    const FakeHalvingPointerMapping mapping;
    MappedPointerSource source(inner, kCodec, mapping);

    EXPECT_THROW(
        {
            const auto events = source.eventsFor(0);
        },
        antwika::input::InputError);
}
