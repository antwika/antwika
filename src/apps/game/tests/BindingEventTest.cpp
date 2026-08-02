#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/BindingEvent.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::game::Action;
using antwika::game::bindKeyFromPayload;
using antwika::game::bindKeyPayload;
using antwika::game::KeyBinding;
using antwika::game::OptionsFormatError;
using antwika::input::Key;

namespace
{
    constexpr KeyBinding kBinding{
        .action = Action::ZoomOut, .key = Key::J};
} // namespace

// Names rather than numbers, for InputEventCodec's reason.
// A recording is read by builds this one has never met.
// And an enumerator's position is not a promise either made.
TEST(BindingEventTest, APayloadHoldsBothHalvesAsTheirNames)
{
    const auto parsed = nlohmann::json::parse(bindKeyPayload(kBinding));

    EXPECT_EQ(parsed.at("action").get<std::string>(), "zoom_out");
    EXPECT_EQ(parsed.at("key").get<std::string>(), "J");
}

TEST(BindingEventTest, ABindingRoundTripsThroughItsPayload)
{
    EXPECT_EQ(bindKeyFromPayload(bindKeyPayload(kBinding)), kBinding);
}

// Both halves are compared.
// So a payload that decoded one of them wrongly cannot pass.
TEST(BindingEventTest, TwoBindingsCompareByBothHalves)
{
    EXPECT_EQ(kBinding, kBinding);

    EXPECT_NE(
        kBinding, (KeyBinding{.action = Action::Pause, .key = Key::J}));
    EXPECT_NE(
        kBinding, (KeyBinding{.action = Action::ZoomOut, .key = Key::K}));
}

TEST(BindingEventTest, APayloadThatIsNotJsonIsRefused)
{
    EXPECT_THROW((void)bindKeyFromPayload("not json"), OptionsFormatError);
}

TEST(BindingEventTest, APayloadOfTheWrongShapeIsRefused)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"pause"})"), OptionsFormatError);
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":1,"key":"J"})"),
        OptionsFormatError);
}

TEST(BindingEventTest, APayloadNamingAnActionThisBuildLacksIsRefused)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"fly","key":"J"})"),
        OptionsFormatError);
}

TEST(BindingEventTest, APayloadNamingAKeyThisBuildLacksIsRefused)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"pause","key":"Joystick"})"),
        OptionsFormatError);
}
