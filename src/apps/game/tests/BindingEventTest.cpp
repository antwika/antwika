#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

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
}

TEST(BindingEventTest, BindKeyPayload_HoldsBothHalvesByName)
{
    const auto parsed = nlohmann::json::parse(bindKeyPayload(kBinding));

    EXPECT_EQ(parsed.at("action").get<std::string>(), "zoom_out");
    EXPECT_EQ(parsed.at("key").get<std::string>(), "J");
}

TEST(BindingEventTest, BindKeyFromPayload_RoundTripsAPayload)
{
    EXPECT_EQ(bindKeyFromPayload(bindKeyPayload(kBinding)), kBinding);
}

TEST(BindingEventTest, OperatorEquals_ComparesBothHalves)
{
    const auto twin = kBinding;
    EXPECT_EQ(kBinding, twin);

    EXPECT_NE(
        kBinding, (KeyBinding{.action = Action::Pause, .key = Key::J}));
    EXPECT_NE(
        kBinding, (KeyBinding{.action = Action::ZoomOut, .key = Key::K}));
}

TEST(BindingEventTest, BindKeyFromPayload_APayloadThatIsNotJsonIsRefused)
{
    EXPECT_THROW((void)bindKeyFromPayload("not json"), OptionsFormatError);
}

TEST(BindingEventTest, BindKeyFromPayload_APayloadOfTheWrongShapeIsRefused)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"pause"})"), OptionsFormatError);
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":1,"key":"J"})"),
        OptionsFormatError);
}

TEST(BindingEventTest, BindKeyFromPayload_RefusesAnUnknownAction)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"fly","key":"J"})"),
        OptionsFormatError);
}

TEST(BindingEventTest, BindKeyFromPayload_RefusesAnUnknownKey)
{
    EXPECT_THROW(
        (void)bindKeyFromPayload(R"({"action":"pause","key":"Joystick"})"),
        OptionsFormatError);
}
