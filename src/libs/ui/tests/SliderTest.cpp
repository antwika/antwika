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
using antwika::widget::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::SliderChange;
using antwika::ui::SliderSpec;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kTrackColor{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumbColor{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kLevelWidget{7};
    constexpr WidgetId kPickerWidget{5};
    constexpr WidgetId kFirstOptionWidget{100};

    constexpr Size kCanvasSize{.width = 200, .height = 100};

    constexpr std::array<std::string_view, 2> kSaves{"one", "two"};

    constexpr std::uint32_t kThumbWidth = 8;

    [[nodiscard]] Theme getPlainTheme()
    {
        return Theme{
            .scrollTrackColor = kTrackColor,
            .scrollThumbColor = kThumbColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .focusRingThickness = 0,
            .sliderHeight = 10,
            .sliderThumbWidth = kThumbWidth};
    }

    [[nodiscard]] SliderSpec getLevelSpec()
    {
        return SliderSpec{.widgetId = kLevelWidget, .value = 0, .range = 255};
    }

    [[nodiscard]] DropdownSpec getOpenPickerSpec()
    {
        return DropdownSpec{
            .widgetId = kPickerWidget,
            .optionIdBaseWidget = kFirstOptionWidget,
            .options = kSaves,
            .selectedIndex = 0,
            .open = true};
    }

    [[nodiscard]] Pointer pressAt(std::int32_t x, std::int32_t y)
    {
        return Pointer{
            .positionPoint = Point{.x = x, .y = y},
            .down = true,
            .pressed = true};
    }

    [[nodiscard]] std::optional<Rect> thumbOf(
        const antwika::ui::DrawList &drawList)
    {
        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == kThumbColor)
            {
                return fill->rect;
            }
        }

        return std::nullopt;
    }
}

TEST(SliderTest, Slider_DrawsATrackAndAThumb)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.slider(getLevelSpec());

    const auto frame = uiContext.build();

    EXPECT_TRUE(frame.rects.getFind(kLevelWidget).has_value());
    EXPECT_TRUE(thumbOf(frame.drawList).has_value());
}

TEST(SliderTest, Slider_ReportsNothingWithThePointerAway)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.slider(getLevelSpec());

    EXPECT_FALSE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_ReportsTheValueUnderAPressOnTheTrack)
{
    const auto track =
        [] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.slider(getLevelSpec());

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        pressAt(
            track.originPoint.x
                + static_cast<std::int32_t>(track.size.width) - 1,
            track.originPoint.y + 1)};

    uiContext.slider(getLevelSpec());

    const auto slid = uiContext.build().interactions.slidChange;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ(
        (SliderChange{.sliderWidget = kLevelWidget, .value = 255}),
        *slid);
}

TEST(SliderTest, Slider_ReportsNothingAtTheStartOfTheTrack)
{
    const auto track =
        [] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.slider(getLevelSpec());

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        pressAt(track.originPoint.x, track.originPoint.y + 1)};

    uiContext.slider(getLevelSpec());

    const auto slid = uiContext.build().interactions.slidChange;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ(0U, slid->value);
}

TEST(SliderTest, Slider_ReportsAHalfwayPressAsHalfItsRange)
{
    const auto track =
        [] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.slider(getLevelSpec());

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        pressAt(
            track.originPoint.x
                + static_cast<std::int32_t>(track.size.width / 2),
            track.originPoint.y + 1)};

    uiContext.slider(getLevelSpec());

    const auto slid = uiContext.build().interactions.slidChange;

    ASSERT_TRUE(slid.has_value());
    EXPECT_NEAR(127.0, static_cast<double>(slid->value), 3.0);
}

TEST(SliderTest, Slider_ReportsNothingFromAPressBesideTheTrack)
{
    Context uiContext{kCanvasSize, getPlainTheme(), pressAt(10, 90)};

    uiContext.slider(getLevelSpec());

    EXPECT_FALSE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_KeepsFollowingAPointerThatLeftTheTrack)
{
    auto spec = getLevelSpec();
    spec.dragging = true;

    const auto track =
        [&spec] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.slider(spec);

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = Point{
                .x = track.originPoint.x
                     + static_cast<std::int32_t>(track.size.width) - 1,
                .y = track.originPoint.y + 1},
            .down = true,
            .pressed = false}};

    uiContext.slider(spec);

    EXPECT_TRUE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_ReportsNothingWhileNotDraggingAndNotPressed)
{
    const auto track =
        [] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.slider(getLevelSpec());

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = Point{
                .x = track.originPoint.x + 4, .y = track.originPoint.y + 1},
            .down = true,
            .pressed = false}};

    uiContext.slider(getLevelSpec());

    EXPECT_FALSE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_ReportsNothingUnderAnOpenDropdown)
{
    const auto picker = getOpenPickerSpec();

    const auto track =
        [&picker] {
            Context uiContext{kCanvasSize, getPlainTheme()};
            uiContext.dropdown(picker);
            uiContext.slider(getLevelSpec());

            return uiContext.build().rects.getFind(kLevelWidget).value_or(Rect{});
        }();

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        pressAt(track.originPoint.x + 4, track.originPoint.y + 1)};

    uiContext.dropdown(picker);
    uiContext.slider(getLevelSpec());

    EXPECT_FALSE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_ReportsNothingForAnUnnamedSlider)
{
    auto spec = getLevelSpec();
    spec.widgetId = kNoWidget;

    Context uiContext{kCanvasSize, getPlainTheme(), pressAt(4, 1)};

    uiContext.slider(spec);

    EXPECT_FALSE(uiContext.build().interactions.slidChange.has_value());
}

TEST(SliderTest, Slider_ReportsItsOwnValueOverARangeOfNothing)
{
    auto spec = getLevelSpec();
    spec.range = 0;
    spec.value = 9;

    Context uiContext{kCanvasSize, getPlainTheme(), pressAt(40, 1)};

    uiContext.slider(spec);

    const auto slid = uiContext.build().interactions.slidChange;

    ASSERT_TRUE(slid.has_value());
    EXPECT_EQ(9U, slid->value);
}

TEST(SliderTest, Slider_PutsTheThumbAtTheStartForTheLeastValue)
{
    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.slider(getLevelSpec());

    const auto frame = uiContext.build();
    const auto track = frame.rects.getFind(kLevelWidget);
    const auto thumb = thumbOf(frame.drawList);

    ASSERT_TRUE(track.has_value());
    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(track->originPoint.x, thumb->originPoint.x);
    EXPECT_EQ(kThumbWidth, thumb->size.width);
}

TEST(SliderTest, Slider_PutsTheThumbAtTheEndForTheGreatestValue)
{
    auto spec = getLevelSpec();
    spec.value = spec.range;

    Context uiContext{kCanvasSize, getPlainTheme()};

    uiContext.slider(spec);

    const auto frame = uiContext.build();
    const auto track = frame.rects.getFind(kLevelWidget);
    const auto thumb = thumbOf(frame.drawList);

    ASSERT_TRUE(track.has_value());
    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(
        track->originPoint.x
            + static_cast<std::int32_t>(track->size.width - kThumbWidth),
        thumb->originPoint.x);
}

TEST(SliderTest, Slider_ClipsAThumbWiderThanTheTrackItSitsOn)
{
    auto theme = getPlainTheme();
    theme.sliderThumbWidth = 500;

    auto spec = getLevelSpec();
    spec.widthSizing = antwika::ui::getFixedSize(20);

    Context uiContext{kCanvasSize, theme};

    uiContext.slider(spec);

    const auto thumb = thumbOf(uiContext.build().drawList);

    ASSERT_TRUE(thumb.has_value());
    EXPECT_EQ(20U, thumb->size.width);
}

TEST(SliderTest, Slider_TakesTheTabFocusLikeAnyOtherWidget)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    uiContext.slider(getLevelSpec());

    EXPECT_EQ(kLevelWidget, uiContext.build().interactions.focusedWidget);
}

TEST(SliderTest, SliderChange_ComparesEveryFieldItCarries)
{
    const SliderChange namedChange{.sliderWidget = kLevelWidget, .value = 3};

    EXPECT_EQ(
        namedChange,
        (SliderChange{.sliderWidget = kLevelWidget, .value = 3}));
    EXPECT_NE(
        namedChange,
        (SliderChange{.sliderWidget = kLevelWidget, .value = 4}));
    EXPECT_NE(
        namedChange,
        (SliderChange{.sliderWidget = kNoWidget, .value = 3}));
}

TEST(SliderTest, SliderSpec_ComparesEveryFieldItCarries)
{
    const auto sliderSpec = getLevelSpec();

    EXPECT_EQ(sliderSpec, getLevelSpec());

    using Change = void (*)(SliderSpec &);

    const std::array<Change, 5> changes{
        [](SliderSpec &spec) { spec.widgetId = kNoWidget; },
        [](SliderSpec &spec)
        { spec.widthSizing = antwika::ui::getFixedSize(4); },
        [](SliderSpec &spec) { spec.value = 1; },
        [](SliderSpec &spec) { spec.range = 9; },
        [](SliderSpec &spec) { spec.dragging = true; }};

    for (const auto &change : changes)
    {
        auto changedSpec = getLevelSpec();
        change(changedSpec);

        EXPECT_NE(sliderSpec, changedSpec);
    }
}
