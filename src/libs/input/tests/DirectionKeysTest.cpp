#include <gtest/gtest.h>

#include <antwika/input/Key.hpp>

#include "antwika/input/DirectionKeys.hpp"

using antwika::input::DirectionKeys;
using antwika::input::applyArrowKey;
using antwika::input::applyWasdKey;
using antwika::input::Key;

TEST(DirectionKeysTest, DirectionKeys_AsksForNothingWithNoKeyDown)
{
    const DirectionKeys keys;

    EXPECT_FLOAT_EQ(keys.getAxisX(), 0.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), 0.0F);
}

TEST(DirectionKeysTest, ApplyArrowKey_TakesEachArrowDownAndBackUp)
{
    DirectionKeys keys;

    applyArrowKey(keys, Key::ArrowRight, true);
    applyArrowKey(keys, Key::ArrowDown, true);

    EXPECT_FLOAT_EQ(keys.getAxisX(), 1.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), 1.0F);

    applyArrowKey(keys, Key::ArrowRight, false);
    applyArrowKey(keys, Key::ArrowDown, false);
    applyArrowKey(keys, Key::ArrowLeft, true);
    applyArrowKey(keys, Key::ArrowUp, true);

    EXPECT_FLOAT_EQ(keys.getAxisX(), -1.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), -1.0F);
}

TEST(DirectionKeysTest, DirectionKeys_StandsStillWithBothWaysOfAnAxisDown)
{
    DirectionKeys keys;

    applyArrowKey(keys, Key::ArrowLeft, true);
    applyArrowKey(keys, Key::ArrowRight, true);
    applyArrowKey(keys, Key::ArrowUp, true);
    applyArrowKey(keys, Key::ArrowDown, true);

    EXPECT_FLOAT_EQ(keys.getAxisX(), 0.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), 0.0F);
}

TEST(DirectionKeysTest, ApplyArrowKey_LeavesTheArrowsAloneForAnyOtherKey)
{
    DirectionKeys keys;

    applyArrowKey(keys, Key::Escape, true);
    applyArrowKey(keys, Key::F10, true);

    EXPECT_EQ(keys, DirectionKeys{});
}

TEST(DirectionKeysTest, ApplyWasdKey_TakesEachOfTheFourDownAndBackUp)
{
    DirectionKeys keys;

    applyWasdKey(keys, Key::D, true);
    applyWasdKey(keys, Key::S, true);

    EXPECT_FLOAT_EQ(keys.getAxisX(), 1.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), 1.0F);

    applyWasdKey(keys, Key::D, false);
    applyWasdKey(keys, Key::S, false);
    applyWasdKey(keys, Key::A, true);
    applyWasdKey(keys, Key::W, true);

    EXPECT_FLOAT_EQ(keys.getAxisX(), -1.0F);
    EXPECT_FLOAT_EQ(keys.getAxisZ(), -1.0F);
}

TEST(DirectionKeysTest, ApplyWasdKey_LeavesTheFourAloneForAnyOtherKey)
{
    DirectionKeys keys;

    applyWasdKey(keys, Key::ArrowUp, true);
    applyWasdKey(keys, Key::Escape, true);

    EXPECT_EQ(keys, DirectionKeys{});
}

TEST(DirectionKeysTest, ApplyArrowKey_AndApplyWasdKey_KeepToTheirOwnKeys)
{
    DirectionKeys keys;

    applyArrowKey(keys, Key::W, true);
    applyWasdKey(keys, Key::ArrowUp, true);

    EXPECT_EQ(keys, DirectionKeys{});
}
