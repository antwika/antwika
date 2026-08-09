#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/event/Event.hpp>

#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Events.hpp"
#include "antwika/input/InputError.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

using antwika::event::Event;
using antwika::input::InputError;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;

namespace events = antwika::input::events;

namespace
{
    constexpr KeyModifiers kHeld{
        .shift = true, .control = false, .alt = true, .super = false};

    constexpr Position kSomewhere{.x = 412, .y = -118};

    [[nodiscard]] std::vector<InputEvent> everyKindOfEvent()
    {
        return {
            KeyPressed{
                .key = Key::Escape, .modifiers = kHeld, .repeat = true},
            KeyPressed{.key = Key::A, .modifiers = {}, .repeat = false},
            KeyReleased{.key = Key::LeftSuper, .modifiers = kHeld},
            PointerMoved{.position = kSomewhere},
            PointerButtonPressed{
                .button = MouseButton::Left,
                .position = kSomewhere,
                .modifiers = kHeld},
            PointerButtonReleased{
                .button = MouseButton::X2,
                .position = kSomewhere,
                .modifiers = {}},
            PointerScrolled{.horizontal = -3, .vertical = 7},
        };
    }

    bool decodes(const InputEventCodec &codec, const Event &event)
    {
        return codec.decode(event).has_value();
    }
}

TEST(InputEventCodecTest, Decode_ReturnsWhateverEncodeWasGiven)
{
    const InputEventCodec codec;

    for (const auto &event : everyKindOfEvent())
    {
        const auto encoded = codec.encode(event);

        EXPECT_EQ(codec.decode(encoded), event);
    }
}

TEST(InputEventCodecTest, Encode_NamesEachKindOfEvent)
{
    const InputEventCodec codec;

    EXPECT_EQ(codec.encode(KeyPressed{}).name, events::kKeyDown);
    EXPECT_EQ(codec.encode(KeyReleased{}).name, events::kKeyUp);
    EXPECT_EQ(codec.encode(PointerMoved{}).name, events::kPointerMove);
    EXPECT_EQ(
        codec.encode(PointerButtonPressed{}).name, events::kPointerDown);
    EXPECT_EQ(
        codec.encode(PointerButtonReleased{}).name, events::kPointerUp);
    EXPECT_EQ(
        codec.encode(PointerScrolled{}).name, events::kPointerScroll);
}

TEST(InputEventCodecTest, Encode_WritesAKeyByItsSymbolicName)
{
    const InputEventCodec codec;

    const auto encoded = codec.encode(KeyPressed{
        .key = Key::Escape,
        .modifiers = {.shift = true},
        .repeat = false});

    EXPECT_EQ(
        encoded.payload,
        R"({"alt":false,"control":false,"key":"Escape","repeat":false,)"
        R"("shift":true,"super":false})");
}

TEST(InputEventCodecTest, Encode_WritesAButtonByItsSymbolicName)
{
    const InputEventCodec codec;

    const auto encoded = codec.encode(PointerButtonPressed{
        .button = MouseButton::Right,
        .position = {.x = 4, .y = -5},
        .modifiers = {}});

    EXPECT_EQ(
        encoded.payload,
        R"({"alt":false,"button":"Right","control":false,"shift":false,)"
        R"("super":false,"x":4,"y":-5})");
}

TEST(InputEventCodecTest, Decode_ReturnsNulloptForAnEventFromSomewhereElse)
{
    const InputEventCodec codec;

    EXPECT_FALSE(decodes(codec, Event{.name = "engine.tick"}));
    EXPECT_FALSE(decodes(codec, Event{.name = "life.toggle_cell"}));
    EXPECT_FALSE(decodes(codec, Event{.name = ""}));
}

TEST(InputEventCodecTest, Decode_ThrowsOnAPayloadThatIsNotJson)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(codec, Event{.name = events::kKeyDown, .payload = "{"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsWhenAFieldIsMissing)
{
    const InputEventCodec codec;

    const std::vector<Event> incomplete{
        Event{
            .name = events::kKeyDown,
            .payload = R"({"key":"A","shift":false,"control":false,)"
                       R"("alt":false,"super":false})"},
        Event{
            .name = events::kKeyUp,
            .payload = R"({"shift":false,"control":false,"alt":false,)"
                       R"("super":false})"},
        Event{.name = events::kPointerMove, .payload = R"({"x":1})"},
        Event{
            .name = events::kPointerDown,
            .payload = R"({"button":"Left","x":1,"y":2})"},
        Event{
            .name = events::kPointerUp,
            .payload = R"({"button":"Left","x":1,"y":2})"},
        Event{
            .name = events::kPointerScroll,
            .payload = R"({"vertical":1})"},
    };

    for (const auto &event : incomplete)
    {
        EXPECT_THROW(decodes(codec, event), InputError) << event.name;
    }
}

TEST(InputEventCodecTest, Decode_ThrowsOnAFieldOfTheWrongType)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kPointerMove,
                .payload = R"({"x":"far","y":2})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnAnUnexpectedField)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kPointerMove,
                .payload = R"({"x":1,"y":2,"z":3})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnACoordinateTooBigForAPosition)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kPointerMove,
                .payload = R"({"x":2147483648,"y":0})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnAKeyNoKeyGoesBy)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kKeyDown,
                .payload =
                    R"({"key":"Any","shift":false,"control":false,)"
                    R"("alt":false,"super":false,"repeat":false})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnAKeyNoKeyGoesByOnARelease)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kKeyUp,
                .payload =
                    R"({"key":"Any","shift":false,"control":false,)"
                    R"("alt":false,"super":false})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnAButtonNoButtonGoesBy)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kPointerUp,
                .payload =
                    R"({"button":"Thumb","x":1,"y":2,"shift":false,)"
                    R"("control":false,"alt":false,"super":false})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_ThrowsOnAButtonNoButtonGoesByOnAPress)
{
    const InputEventCodec codec;

    EXPECT_THROW(
        decodes(
            codec,
            Event{
                .name = events::kPointerDown,
                .payload =
                    R"({"button":"Thumb","x":1,"y":2,"shift":false,)"
                    R"("control":false,"alt":false,"super":false})"}),
        InputError);
}

TEST(InputEventCodecTest, Decode_NamesTheEventItCouldNotMakeSenseOf)
{
    const InputEventCodec codec;

    try
    {
        decodes(
            codec,
            Event{.name = events::kPointerScroll, .payload = "nope"});
        FAIL() << "decoding a broken payload did not throw";
    }
    catch (const InputError &error)
    {
        EXPECT_THAT(
            std::string(error.what()),
            ::testing::HasSubstr("input.pointer_scroll"));
    }
}
