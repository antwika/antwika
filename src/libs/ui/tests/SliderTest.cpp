#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SliderSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DropdownSpec;
using antwika::ui::FillRect;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::SliderChange;
using antwika::ui::SliderSpec;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kTrack{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumb{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kLevel{7};
    constexpr WidgetId kPicker{5};
    constexpr WidgetId kFirstOption{100};

    constexpr Size kCanvas{.width = 200, .height = 100};

    constexpr std::array<std::string_view, 2> kSaves{"one", "two"};

    constexpr std::uint32_t kThumbWidth = 8;

    [[nodiscard]] Theme plainTheme()
    {
        return Theme{
            .scrollTrack = kTrack,
            .scrollThumb = kThumb,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .focusRingThickness = 0,
            .sliderHeight = 10,
            .sliderThumbWidth = kThumbWidth};
    }

    [[nodiscard]] SliderSpec levelSpec()
    {
        return SliderSpec{.id = kLevel, .value = 0, .range = 255};
    }

    [[nodiscard]] DropdownSpec openPickerSpec()
    {
        return DropdownSpec{
            .id = kPicker,
            .optionIdBase = kFirstOption,
            .options = kSaves,
            .selected = 0,
            .open = true};
    }

    [[nodiscard]] Pointer pressAt(std::int32_t x, std::int32_t y)
    {
        return Pointer{
            .position = Point{.x = x, .y = y},
            .down = true,
            .pressed = true};
    }

    [[nodiscard]] std::optional<Rect> thumbOf(
        const antwika::ui::DrawList &commands)
    {
        for (const auto &command : commands)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == kThumb)
            {
                return fill->rect;
            }
        }

        return std::nullopt;
    }
}

TEST(SliderTest, Slider_DrawsATrackAndAThumb)
{
    Context ui{kCanvas, plainTheme()};

    ui.slider(levelSpec());

    const auto frame = ui.finish();

    EXPECT_TRUE(frame.rects.find(kLevel).has_value());
    EXPECT_TRUE(thumbOf(frame.commands).has_value());
}

TEST(SliderTest, Slider_ReportsNothingWithThePointerAway)
{
    Context ui{kCanvas, plainTheme()};

    ui.slider(levelSpec());

    EXPECT_FALSE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_ReportsTheValueUnderAPressOnTheTrack)
{
    const auto track =
        [] {
            Context ui{kCanvas, plainTheme()};
            ui.slider(levelSpec());

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        pressAt(
            track.origin.x
                + static_cast<std::int32_t>(track.size.width) - 1,
            track.origin.y + 1)};

    ui.slider(levelSpec());

    const auto slid = ui.finish().interactions.slid;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ((SliderChange{.slider = kLevel, .value = 255}), *slid);
}

TEST(SliderTest, Slider_ReportsNothingAtTheStartOfTheTrack)
{
    const auto track =
        [] {
            Context ui{kCanvas, plainTheme()};
            ui.slider(levelSpec());

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        pressAt(track.origin.x, track.origin.y + 1)};

    ui.slider(levelSpec());

    const auto slid = ui.finish().interactions.slid;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ(0U, slid->value);
}

TEST(SliderTest, Slider_ReportsAHalfwayPressAsHalfItsRange)
{
    const auto track =
        [] {
            Context ui{kCanvas, plainTheme()};
            ui.slider(levelSpec());

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        pressAt(
            track.origin.x
                + static_cast<std::int32_t>(track.size.width / 2),
            track.origin.y + 1)};

    ui.slider(levelSpec());

    const auto slid = ui.finish().interactions.slid;

    ASSERT_TRUE(slid.has_value());
    EXPECT_NEAR(127.0, static_cast<double>(slid->value), 3.0);
}

TEST(SliderTest, Slider_ReportsNothingFromAPressBesideTheTrack)
{
    Context ui{kCanvas, plainTheme(), pressAt(10, 90)};

    ui.slider(levelSpec());

    EXPECT_FALSE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_KeepsFollowingAPointerThatLeftTheTrack)
{
    auto spec = levelSpec();
    spec.dragging = true;

    const auto track =
        [&spec] {
            Context ui{kCanvas, plainTheme()};
            ui.slider(spec);

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = Point{
                .x = track.origin.x
                     + static_cast<std::int32_t>(track.size.width) - 1,
                .y = track.origin.y + 1},
            .down = true,
            .pressed = false}};

    ui.slider(spec);

    EXPECT_TRUE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_ReportsNothingWhileNotDraggingAndNotPressed)
{
    const auto track =
        [] {
            Context ui{kCanvas, plainTheme()};
            ui.slider(levelSpec());

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = Point{
                .x = track.origin.x + 4, .y = track.origin.y + 1},
            .down = true,
            .pressed = false}};

    ui.slider(levelSpec());

    EXPECT_FALSE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_ReportsNothingUnderAnOpenDropdown)
{
    const auto picker = openPickerSpec();

    const auto track =
        [&picker] {
            Context ui{kCanvas, plainTheme()};
            ui.dropdown(picker);
            ui.slider(levelSpec());

            return ui.finish().rects.find(kLevel).value_or(Rect{});
        }();

    Context ui{
        kCanvas,
        plainTheme(),
        pressAt(track.origin.x + 4, track.origin.y + 1)};

    ui.dropdown(picker);
    ui.slider(levelSpec());

    EXPECT_FALSE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_ReportsNothingForAnUnnamedSlider)
{
    auto spec = levelSpec();
    spec.id = kNoWidget;

    Context ui{kCanvas, plainTheme(), pressAt(4, 1)};

    ui.slider(spec);

    EXPECT_FALSE(ui.finish().interactions.slid.has_value());
}

TEST(SliderTest, Slider_ReportsItsOwnValueOverARangeOfNothing)
{
    auto spec = levelSpec();
    spec.range = 0;
    spec.value = 9;

    Context ui{kCanvas, plainTheme(), pressAt(40, 1)};

    ui.slider(spec);

    const auto slid = ui.finish().interactions.slid;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ(9U, slid->value);
}

TEST(SliderTest, Slider_PutsTheThumbAtTheStartForTheLeastValue)
{
    Context ui{kCanvas, plainTheme()};

    ui.slider(levelSpec());

    const auto frame = ui.finish();
    const auto track = frame.rects.find(kLevel);
    const auto thumb = thumbOf(frame.commands);

    ASSERT_TRUE(track.has_value());
    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(track->origin.x, thumb->origin.x);
    EXPECT_EQ(kThumbWidth, thumb->size.width);
}

TEST(SliderTest, Slider_PutsTheThumbAtTheEndForTheGreatestValue)
{
    auto spec = levelSpec();
    spec.value = spec.range;

    Context ui{kCanvas, plainTheme()};

    ui.slider(spec);

    const auto frame = ui.finish();
    const auto track = frame.rects.find(kLevel);
    const auto thumb = thumbOf(frame.commands);

    ASSERT_TRUE(track.has_value());
    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(
        track->origin.x
            + static_cast<std::int32_t>(track->size.width - kThumbWidth),
        thumb->origin.x);
}

TEST(SliderTest, Slider_ClipsAThumbWiderThanTheTrackItSitsOn)
{
    auto theme = plainTheme();
    theme.sliderThumbWidth = 500;

    auto spec = levelSpec();
    spec.width = antwika::ui::fixedSize(20);

    Context ui{kCanvas, theme};

    ui.slider(spec);

    const auto thumb = thumbOf(ui.finish().commands);

    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(20U, thumb->size.width);
}

TEST(SliderTest, Slider_TakesTheTabFocusLikeAnyOtherWidget)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    ui.slider(levelSpec());

    EXPECT_EQ(kLevel, ui.finish().interactions.focused);
}

TEST(SliderTest, SliderChange_ComparesEveryFieldItCarries)
{
    const SliderChange named{.slider = kLevel, .value = 3};

    EXPECT_EQ(named, (SliderChange{.slider = kLevel, .value = 3}));
    EXPECT_NE(named, (SliderChange{.slider = kLevel, .value = 4}));
    EXPECT_NE(named, (SliderChange{.slider = kNoWidget, .value = 3}));
}

TEST(SliderTest, SliderSpec_ComparesEveryFieldItCarries)
{
    const auto named = levelSpec();

    EXPECT_EQ(named, levelSpec());

    using Change = void (*)(SliderSpec &);

    const std::array<Change, 5> changes{
        [](SliderSpec &spec) { spec.id = kNoWidget; },
        [](SliderSpec &spec)
        { spec.width = antwika::ui::fixedSize(4); },
        [](SliderSpec &spec) { spec.value = 1; },
        [](SliderSpec &spec) { spec.range = 9; },
        [](SliderSpec &spec) { spec.dragging = true; }};

    for (const auto &change : changes)
    {
        auto changed = levelSpec();
        change(changed);

        EXPECT_NE(named, changed);
    }
}
