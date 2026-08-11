#include <gtest/gtest.h>

#include <cstddef>
#include <map>
#include <string>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/map_editor/Hotkeys.hpp"

using antwika::input::Key;
using antwika::map_editor::actionOfKey;
using antwika::map_editor::bindableHotkey;
using antwika::map_editor::defaultHotkeyBindings;
using antwika::map_editor::HotkeyAction;
using antwika::map_editor::HotkeyBindings;
using antwika::map_editor::hotkeyLabel;
using antwika::map_editor::hotkeysFromConfig;
using antwika::map_editor::hotkeysToConfig;
using antwika::map_editor::kHotkeyActionCount;
using antwika::map_editor::keyCaption;
using antwika::map_editor::toString;

namespace
{
    [[nodiscard]] Key boundTo(
        const HotkeyBindings &bindings, const HotkeyAction action)
    {
        return bindings[antwika::enums::index(action)];
    }
}

TEST(HotkeysTest, HotkeyAction_CountsEveryDeclaredAction)
{
    EXPECT_EQ(kHotkeyActionCount, 21U);
}

TEST(HotkeysTest, ToString_NamesEveryActionDistinctly)
{
    std::map<std::string, std::size_t> seen;

    for (const auto action : antwika::enums::kAll<HotkeyAction>)
    {
        ++seen[std::string(toString(action))];
    }

    EXPECT_EQ(seen.size(), kHotkeyActionCount);
    EXPECT_EQ(toString(HotkeyAction::RaiseHeight), "raiseHeight");
    EXPECT_EQ(toString(HotkeyAction::Picker), "picker");
}

TEST(HotkeysTest, HotkeyLabel_LabelsEveryAction)
{
    for (const auto action : antwika::enums::kAll<HotkeyAction>)
    {
        EXPECT_FALSE(hotkeyLabel(action).empty());
    }

    EXPECT_EQ(hotkeyLabel(HotkeyAction::RaiseHeight), "level up");
    EXPECT_EQ(hotkeyLabel(HotkeyAction::Picker), "sprite picker");
}

TEST(HotkeysTest, DefaultHotkeyBindings_BindEveryActionApart)
{
    const auto bindings = defaultHotkeyBindings();

    for (std::size_t at = 0; at < kHotkeyActionCount; ++at)
    {
        for (std::size_t other = at + 1; other < kHotkeyActionCount;
             ++other)
        {
            EXPECT_NE(bindings[at], bindings[other]);
        }
    }
}

TEST(HotkeysTest, BindableHotkey_TakesLettersAndFunctionKeys)
{
    EXPECT_TRUE(bindableHotkey(Key::A));
    EXPECT_TRUE(bindableHotkey(Key::Z));
    EXPECT_TRUE(bindableHotkey(Key::F1));
    EXPECT_TRUE(bindableHotkey(Key::F12));
}

TEST(HotkeysTest, BindableHotkey_TakesThePunctuationKeys)
{
    for (const auto key : {
             Key::Minus,
             Key::Equal,
             Key::LeftBracket,
             Key::RightBracket,
             Key::Backslash,
             Key::Semicolon,
             Key::Apostrophe,
             Key::Comma,
             Key::Period,
             Key::Slash})
    {
        EXPECT_TRUE(bindableHotkey(key));
    }
}

TEST(HotkeysTest, BindableHotkey_ReservesDigitsAndControlKeys)
{
    for (const auto key : {
             Key::Digit0,
             Key::Digit9,
             Key::Tab,
             Key::Escape,
             Key::Grave,
             Key::ArrowLeft,
             Key::ArrowRight,
             Key::ArrowUp,
             Key::ArrowDown})
    {
        EXPECT_FALSE(bindableHotkey(key));
    }
}

TEST(HotkeysTest, ActionOfKey_FindsTheActionABindingHolds)
{
    const auto bindings = defaultHotkeyBindings();

    EXPECT_EQ(actionOfKey(bindings, Key::U), HotkeyAction::Undo);
    EXPECT_EQ(actionOfKey(bindings, Key::I), HotkeyAction::Picker);
}

TEST(HotkeysTest, ActionOfKey_YieldsNothingForAnUnboundKey)
{
    EXPECT_FALSE(
        actionOfKey(defaultHotkeyBindings(), Key::Tab).has_value());
}

TEST(HotkeysTest, ActionOfKey_TakesTheFirstActionHoldingAKey)
{
    auto bindings = defaultHotkeyBindings();
    bindings[antwika::enums::index(HotkeyAction::Picker)] = Key::U;

    EXPECT_EQ(actionOfKey(bindings, Key::U), HotkeyAction::Undo);
}

TEST(HotkeysTest, KeyCaption_ShortensThePunctuationKeys)
{
    EXPECT_EQ(keyCaption(Key::Minus), "-");
    EXPECT_EQ(keyCaption(Key::Equal), "=");
    EXPECT_EQ(keyCaption(Key::LeftBracket), "[");
    EXPECT_EQ(keyCaption(Key::RightBracket), "]");
    EXPECT_EQ(keyCaption(Key::Backslash), "\\");
    EXPECT_EQ(keyCaption(Key::Semicolon), ";");
    EXPECT_EQ(keyCaption(Key::Apostrophe), "'");
    EXPECT_EQ(keyCaption(Key::Comma), ",");
    EXPECT_EQ(keyCaption(Key::Period), ".");
    EXPECT_EQ(keyCaption(Key::Slash), "/");
}

TEST(HotkeysTest, KeyCaption_FallsBackToTheKeyName)
{
    EXPECT_EQ(keyCaption(Key::A), antwika::input::toString(Key::A));
}

TEST(HotkeysTest, HotkeysFromConfig_TakesADeclaredBinding)
{
    const auto bindings = hotkeysFromConfig({{"undo", "J"}});

    EXPECT_EQ(boundTo(bindings, HotkeyAction::Undo), Key::J);
}

TEST(HotkeysTest, HotkeysFromConfig_KeepsDefaultsForMissingEntries)
{
    const auto bindings = hotkeysFromConfig({});

    EXPECT_EQ(bindings, defaultHotkeyBindings());
}

TEST(HotkeysTest, HotkeysFromConfig_IgnoresAnUnknownActionName)
{
    const auto bindings = hotkeysFromConfig({{"levitate", "J"}});

    EXPECT_EQ(bindings, defaultHotkeyBindings());
}

TEST(HotkeysTest, HotkeysFromConfig_IgnoresAnUnknownKeyName)
{
    const auto bindings = hotkeysFromConfig({{"undo", "not-a-key"}});

    EXPECT_EQ(boundTo(bindings, HotkeyAction::Undo), Key::U);
}

TEST(HotkeysTest, HotkeysFromConfig_IgnoresAReservedKey)
{
    const auto bindings = hotkeysFromConfig({{"undo", "Tab"}});

    EXPECT_EQ(boundTo(bindings, HotkeyAction::Undo), Key::U);
}

TEST(HotkeysTest, HotkeysFromConfig_KeepsTheFirstActionOfAClaimedKey)
{
    const auto bindings =
        hotkeysFromConfig({{"undo", "J"}, {"redo", "J"}});

    EXPECT_EQ(boundTo(bindings, HotkeyAction::Undo), Key::J);
    EXPECT_EQ(boundTo(bindings, HotkeyAction::Redo), Key::R);
}

TEST(HotkeysTest, HotkeysToConfig_NamesEveryActionAndKey)
{
    const auto entries = hotkeysToConfig(defaultHotkeyBindings());

    EXPECT_EQ(entries.size(), kHotkeyActionCount);
    EXPECT_EQ(
        entries.at("undo"), antwika::input::toString(Key::U));
}

TEST(HotkeysTest, HotkeysToConfig_RoundTripsThroughHotkeysFromConfig)
{
    auto bindings = defaultHotkeyBindings();
    bindings[antwika::enums::index(HotkeyAction::Undo)] = Key::J;

    EXPECT_EQ(hotkeysFromConfig(hotkeysToConfig(bindings)), bindings);
}
