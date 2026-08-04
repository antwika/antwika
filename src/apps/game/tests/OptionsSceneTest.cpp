#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Pointer.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"

using antwika::game::tests::kLanguages;
using antwika::game::tests::kTranslator;
using antwika::game::tests::widgetCentre;

using antwika::game::Action;
using antwika::game::kActions;
using antwika::game::kQuitKey;
using antwika::game::OptionsScene;
using antwika::game::OptionsState;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::Key;
using antwika::ui::Pointer;
namespace optionsWidgets = antwika::game::optionsWidgets;

namespace
{
    // The language every layout in this file is described in.
    // Named once so a call to describe() still fits a line.
    // And named rather than defaulted on purpose.
    // Which language a layout was built in is what these rest on.
    constexpr auto kLocale = antwika::i18n::kDefaultLocale;
    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] Pointer pressAt(Point at)
    {
        return Pointer{.position = at, .down = true, .pressed = true};
    }
} // namespace

TEST(OptionsSceneTest, TheScreenDescribesItselfWithNoPointerAtAll)
{
    const OptionsScene scene{kTranslator, kLanguages};
    const OptionsState state;

    const auto frame = scene.describe(kCanvas, Pointer{}, state, kLocale);

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_EQ(frame.interactions.hovered, antwika::ui::kNoWidget);
    EXPECT_EQ(frame.interactions.activated, antwika::ui::kNoWidget);
}

// The same canvas, pointer and state always produce the same picture.
// That is what lets a recorded click resolve to the same row.
TEST(OptionsSceneTest, TheSameArgumentsProduceTheSamePicture)
{
    const OptionsScene scene{kTranslator, kLanguages};
    const OptionsState state;
    const Pointer pointer{.position = Point{.x = 512, .y = 320}};

    EXPECT_EQ(
        scene.describe(kCanvas, pointer, state, kLocale).commands,
        scene.describe(kCanvas, pointer, state, kLocale).commands);
}

TEST(OptionsSceneTest, EveryActionGetsARowOfItsOwn)
{
    const OptionsScene scene{kTranslator, kLanguages};
    const OptionsState state;

    const auto frame = scene.describe(kCanvas, Pointer{}, state, kLocale);

    for (const auto action : kActions)
    {
        const auto centre =
            widgetCentre(frame, optionsWidgets::actionWidget(action));

        ASSERT_TRUE(centre.has_value());

        EXPECT_EQ(
            scene.describe(kCanvas, pressAt(*centre), state, kLocale)
                .interactions.activated,
            optionsWidgets::actionWidget(action));
    }
}

TEST(OptionsSceneTest, TheBackButtonIsReachable)
{
    const OptionsScene scene{kTranslator, kLanguages};
    const OptionsState state;

    const auto centre = widgetCentre(
        scene.describe(kCanvas, Pointer{}, state, kLocale),
        optionsWidgets::kBack);

    ASSERT_TRUE(centre.has_value());

    EXPECT_EQ(
        scene.describe(kCanvas, pressAt(*centre), state, kLocale)
            .interactions.activated,
        optionsWidgets::kBack);
}

// The row a question is being asked about reads differently.
// Otherwise nothing on screen says a key is being waited for.
TEST(OptionsSceneTest, AWaitingRowIsDrawnDifferentlyFromABoundOne)
{
    const OptionsScene scene{kTranslator, kLanguages};
    OptionsState state;

    const auto quiet =
        scene.describe(kCanvas, Pointer{}, state, kLocale).commands;

    state.await(Action::Pause);

    EXPECT_NE(
        scene.describe(kCanvas, Pointer{}, state, kLocale).commands, quiet);
}

// The card is one height whatever the line under the rows says.
// So nothing jumps under a click after a refusal.
TEST(OptionsSceneTest, TheCardIsOneHeightWhateverTheNoticeSays)
{
    const OptionsScene scene{kTranslator, kLanguages};
    OptionsState state;

    const auto placeOfBack = [&scene, &state]
    {
        return widgetCentre(
            scene.describe(kCanvas, Pointer{}, state, kLocale),
            optionsWidgets::kBack);
    };

    const auto quiet = placeOfBack();

    state.await(Action::Pause);
    EXPECT_TRUE(state.press(kQuitKey).has_value());
    const auto refused = placeOfBack();

    state.await(Action::Pause);
    EXPECT_TRUE(
        state.press(antwika::game::kDefaultBindings.keyFor(Action::ZoomIn))
            .has_value());
    const auto taken = placeOfBack();

    state.await(Action::Pause);
    EXPECT_TRUE(state.press(Key::J).has_value());
    const auto bound = placeOfBack();

    ASSERT_TRUE(quiet.has_value());
    EXPECT_EQ(refused, quiet);
    EXPECT_EQ(taken, quiet);
    EXPECT_EQ(bound, quiet);
}

// Each of the four things an attempt can come to says its own thing.
// A refusal nobody is told about looks like a key that failed.
TEST(OptionsSceneTest, EveryOutcomeIsSaidInItsOwnWords)
{
    const OptionsScene scene{kTranslator, kLanguages};
    OptionsState state;

    const auto picture = [&scene, &state]
    { return scene.describe(kCanvas, Pointer{}, state, kLocale).commands; };

    const auto quiet = picture();

    state.await(Action::Pause);
    EXPECT_TRUE(state.press(kQuitKey).has_value());
    const auto reserved = picture();

    state.await(Action::Pause);
    EXPECT_TRUE(
        state.press(antwika::game::kDefaultBindings.keyFor(Action::ZoomIn))
            .has_value());
    const auto taken = picture();

    state.await(Action::Pause);
    EXPECT_TRUE(state.press(Key::J).has_value());
    const auto bound = picture();

    EXPECT_NE(reserved, quiet);
    EXPECT_NE(taken, quiet);
    EXPECT_NE(taken, reserved);
    EXPECT_NE(bound, taken);
}

// The key a row shows is the one the layout actually holds.
TEST(OptionsSceneTest, ARowFollowsTheKeyItsActionIsBoundTo)
{
    const OptionsScene scene{kTranslator, kLanguages};
    OptionsState state;

    const auto before =
        scene.describe(kCanvas, Pointer{}, state, kLocale).commands;

    EXPECT_TRUE(state.apply(Action::ResetView, Key::J)
                == antwika::game::BindOutcome::Bound);

    EXPECT_NE(
        scene.describe(kCanvas, Pointer{}, state, kLocale).commands, before);
}

// The picker names every language, and each has its own widget.
TEST(OptionsSceneTest, EveryLanguageIsReachableAndDistinct)
{
    const OptionsScene scene{kTranslator, kLanguages};
    const OptionsState state;

    const auto frame = scene.describe(
        kCanvas, Pointer{}, state, kLocale);

    for (const auto locale : antwika::i18n::kAllLocales)
    {
        const auto centre = widgetCentre(
            frame, optionsWidgets::languageWidget(locale));

        ASSERT_TRUE(centre.has_value());

        EXPECT_EQ(
            scene
                .describe(
                    kCanvas,
                    pressAt(*centre),
                    state,
                    kLocale)
                .interactions.activated,
            optionsWidgets::languageWidget(locale));
    }
}

// A language's button may never be a key binding's.
// The two ranges are set far enough apart that neither can reach.
TEST(OptionsSceneTest, ALanguageWidgetIsNeverAnActionWidget)
{
    for (const auto locale : antwika::i18n::kAllLocales)
    {
        for (const auto action : kActions)
        {
            EXPECT_NE(
                optionsWidgets::languageWidget(locale),
                optionsWidgets::actionWidget(action));
        }
    }
}

// Read through the *active* translator.
// So the list is in the language now on.
TEST(OptionsSceneTest, TheLanguageNamesAreWordedInTheLanguageNowOn)
{
    const OptionsScene english{kTranslator, kLanguages};

    const auto shown = [&](antwika::i18n::Locale active)
    {
        std::vector<std::string> text;

        for (const auto &command :
             english.describe(kCanvas, Pointer{}, OptionsState{}, active)
                 .commands)
        {
            if (const auto *drawn =
                    std::get_if<antwika::ui::DrawText>(&command))
            {
                text.push_back(drawn->text);
            }
        }

        return text;
    };

    const auto captions = shown(kLocale);

    EXPECT_NE(
        std::find(captions.begin(), captions.end(), "Swedish"),
        captions.end());
}

// The one on says so, in the text rather than by a colour.
// So the mark survives a theme, and a test can read it.
TEST(OptionsSceneTest, TheActiveLanguageIsMarkedInItsCaption)
{
    const OptionsScene scene{kTranslator, kLanguages};

    std::vector<std::string> text;

    for (const auto &command :
         scene
             .describe(
                 kCanvas,
                 Pointer{},
                 OptionsState{},
                 antwika::i18n::Locale::English)
             .commands)
    {
        if (const auto *drawn =
                std::get_if<antwika::ui::DrawText>(&command))
        {
            text.push_back(drawn->text);
        }
    }

    EXPECT_NE(
        std::find(text.begin(), text.end(), "English (on)"), text.end());
    EXPECT_NE(
        std::find(text.begin(), text.end(), "Swedish"), text.end());
}
