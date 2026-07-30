#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::input::InputError;
using antwika::input::kMouseButtonCount;
using antwika::input::MouseButton;
using antwika::input::mouseButtonFromString;
using antwika::input::mouseButtonIndex;
using antwika::input::toString;

TEST(MouseButtonTest, MouseButtonIndex_NumbersTheFirstButtonFromZero)
{
    EXPECT_EQ(mouseButtonIndex(MouseButton::Left), 0u);
}

TEST(MouseButtonTest, MouseButtonIndex_StaysBelowTheButtonCount)
{
    for (std::size_t index = 0; index < kMouseButtonCount; ++index)
    {
        const auto button = static_cast<MouseButton>(index);

        EXPECT_LT(mouseButtonIndex(button), kMouseButtonCount)
            << toString(button);
    }
}

TEST(MouseButtonTest, KMouseButtonCount_CountsEveryButtonThatHasAName)
{
    for (std::size_t index = 0; index < kMouseButtonCount; ++index)
    {
        const auto button = static_cast<MouseButton>(index);

        EXPECT_NE(toString(button), "Unknown") << index;
    }
}

TEST(MouseButtonTest, ToString_NamesAButton)
{
    EXPECT_EQ(toString(MouseButton::Left), "Left");
    EXPECT_EQ(toString(MouseButton::Middle), "Middle");
    EXPECT_EQ(toString(MouseButton::Right), "Right");
    EXPECT_EQ(toString(MouseButton::X1), "X1");
    EXPECT_EQ(toString(MouseButton::X2), "X2");
}

TEST(MouseButtonTest, ToString_ReportsUnknownForAValueOutsideTheEnumeration)
{
    const auto beyond = static_cast<MouseButton>(kMouseButtonCount);

    EXPECT_EQ(toString(beyond), "Unknown");
}

TEST(MouseButtonTest, ToString_GivesEveryButtonItsOwnName)
{
    std::set<std::string_view> names;

    for (std::size_t index = 0; index < kMouseButtonCount; ++index)
    {
        const auto button = static_cast<MouseButton>(index);

        EXPECT_TRUE(names.insert(toString(button)).second)
            << toString(button);
    }

    EXPECT_EQ(names.size(), kMouseButtonCount);
}

TEST(MouseButtonTest, MouseButtonFromString_FindsTheButtonANameRefersTo)
{
    EXPECT_EQ(mouseButtonFromString("Right"), MouseButton::Right);
}

TEST(MouseButtonTest, MouseButtonFromString_RoundTripsEveryButton)
{
    for (std::size_t index = 0; index < kMouseButtonCount; ++index)
    {
        const auto button = static_cast<MouseButton>(index);

        EXPECT_EQ(mouseButtonFromString(toString(button)), button)
            << toString(button);
    }
}

TEST(MouseButtonTest, MouseButtonFromString_ThrowsOnANameNoButtonGoesBy)
{
    EXPECT_THROW(
        static_cast<void>(mouseButtonFromString("Fourth")), InputError);
}

TEST(MouseButtonTest, MouseButtonFromString_SaysWhichNameItRejected)
{
    try
    {
        const auto button = mouseButtonFromString("Fourth");
        FAIL() << "expected an InputError, got " << toString(button);
    }
    catch (const InputError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find("Fourth"), std::string::npos);
    }
}
