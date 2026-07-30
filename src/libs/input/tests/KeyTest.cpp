#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"
#include "antwika/input/Key.hpp"

using antwika::input::InputError;
using antwika::input::Key;
using antwika::input::keyFromString;
using antwika::input::keyIndex;
using antwika::input::kKeyCount;
using antwika::input::toString;

TEST(KeyTest, KeyIndex_NumbersTheFirstKeyFromZero)
{
    EXPECT_EQ(keyIndex(Key::A), 0u);
}

TEST(KeyTest, KeyIndex_StaysBelowTheKeyCountForEveryNamedKey)
{
    // The folded device state sizes its bitset by kKeyCount.
    // It indexes that bitset by keyIndex, so overrunning it corrupts memory.
    for (std::size_t index = 0; index < kKeyCount; ++index)
    {
        const auto key = static_cast<Key>(index);

        EXPECT_LT(keyIndex(key), kKeyCount) << toString(key);
    }
}

TEST(KeyTest, KKeyCount_CountsEveryKeyThatHasAName)
{
    // kKeyCount comes from the last enumerator, so this catches a later one.
    for (std::size_t index = 0; index < kKeyCount; ++index)
    {
        const auto key = static_cast<Key>(index);

        EXPECT_NO_THROW(static_cast<void>(toString(key))) << index;
    }
}

TEST(KeyTest, ToString_NamesAKey)
{
    EXPECT_EQ(toString(Key::Escape), "Escape");
    EXPECT_EQ(toString(Key::A), "A");
    EXPECT_EQ(toString(Key::Digit0), "Digit0");
    EXPECT_EQ(toString(Key::F12), "F12");
    EXPECT_EQ(toString(Key::ArrowLeft), "ArrowLeft");
    EXPECT_EQ(toString(Key::RightSuper), "RightSuper");
}

TEST(KeyTest, ToString_ThrowsOnAValueOutsideTheEnumeration)
{
    // Reachable only by a cast, which a backend mapping key codes may do.
    // It fails here, where the offending key is in hand.
    // Naming it "Unknown" would fail in whatever replays the recording.
    const auto beyond = static_cast<Key>(kKeyCount);

    EXPECT_THROW(static_cast<void>(toString(beyond)), InputError);
}

TEST(KeyTest, ToString_AgreesWithKeyFromStringOnWhatHasNoName)
{
    // The asymmetry this replaced let a session record and not replay.
    const auto beyond = static_cast<Key>(kKeyCount);

    try
    {
        static_cast<void>(toString(beyond));
        FAIL() << "expected an InputError";
    }
    catch (const InputError &error)
    {
        EXPECT_NE(
            std::string(error.what()).find(std::to_string(kKeyCount)),
            std::string::npos);
    }
}

TEST(KeyTest, ToString_GivesEveryKeyItsOwnName)
{
    // A duplicate name would make keyFromString ambiguous.
    // It would silently turn one key into another across a replay.
    std::set<std::string_view> names;

    for (std::size_t index = 0; index < kKeyCount; ++index)
    {
        const auto key = static_cast<Key>(index);

        EXPECT_TRUE(names.insert(toString(key)).second) << toString(key);
    }

    EXPECT_EQ(names.size(), kKeyCount);
}

TEST(KeyTest, KeyFromString_FindsTheKeyANameRefersTo)
{
    EXPECT_EQ(keyFromString("Escape"), Key::Escape);
}

TEST(KeyTest, KeyFromString_RoundTripsEveryKey)
{
    // This is the property the replay format rests on.
    // A key written out and read back must be the key that was pressed.
    for (std::size_t index = 0; index < kKeyCount; ++index)
    {
        const auto key = static_cast<Key>(index);

        EXPECT_EQ(keyFromString(toString(key)), key) << toString(key);
    }
}

TEST(KeyTest, KeyFromString_ThrowsOnANameNoKeyGoesBy)
{
    EXPECT_THROW(static_cast<void>(keyFromString("Nope")), InputError);
}

TEST(KeyTest, KeyFromString_SaysWhichNameItRejected)
{
    try
    {
        const auto key = keyFromString("Nope");
        FAIL() << "expected an InputError, got " << toString(key);
    }
    catch (const InputError &error)
    {
        EXPECT_NE(std::string(error.what()).find("Nope"), std::string::npos);
    }
}

TEST(KeyTest, KeyFromString_IsCaseSensitive)
{
    // The names are a wire format, not user input.
    EXPECT_THROW(static_cast<void>(keyFromString("escape")), InputError);
}
