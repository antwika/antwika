#include <gtest/gtest.h>

#include "antwika/game/KeyboardEvent.hpp"
#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/OptionsFormatError.hpp"

using antwika::game::keyboardFromPayload;
using antwika::game::KeyboardLayout;
using antwika::game::kKeyboardLayouts;
using antwika::game::OptionsFormatError;
using antwika::game::setKeyboardPayload;

TEST(KeyboardEventTest, EveryLayoutRoundTrips)
{
    for (const auto layout : kKeyboardLayouts)
    {
        EXPECT_EQ(
            keyboardFromPayload(setKeyboardPayload(layout)), layout);
    }
}

TEST(KeyboardEventTest, APayloadOfTheWrongShapeIsRefused)
{
    EXPECT_THROW(
        (void)keyboardFromPayload("{}"), OptionsFormatError);
    EXPECT_THROW(
        (void)keyboardFromPayload("not json"), OptionsFormatError);
}

TEST(KeyboardEventTest, ALayoutThisBuildDoesNotKnowIsRefused)
{
    EXPECT_THROW(
        (void)keyboardFromPayload(R"({"keyboard":"dvorak"})"),
        OptionsFormatError);
}
