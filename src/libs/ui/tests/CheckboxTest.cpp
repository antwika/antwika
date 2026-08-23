#include <gtest/gtest.h>

#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/CheckboxSpec.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::CheckboxSpec;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{

    constexpr Size kCanvasSize{.width = 120, .height = 60};

    constexpr WidgetId kBoxWidget{7};

    [[nodiscard]] Theme getPlainTheme()
    {
        return Theme{};
    }

    [[nodiscard]] int boxesOf(const DrawList &drawList)
    {
        auto foundCount = 0;

        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr
                && fill->rect.size.width
                       == getPlainTheme().checkboxSize
                && fill->rect.size.height
                       == getPlainTheme().checkboxSize)
            {
                ++foundCount;
            }
        }

        return foundCount;
    }

    [[nodiscard]] int inkedOf(const DrawList &drawList)
    {
        auto foundCount = 0;

        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr
                && fill->color == getPlainTheme().textColor
                && fill->rect.size.width < getPlainTheme().checkboxSize)
            {
                ++foundCount;
            }
        }

        return foundCount;
    }

    TEST(CheckboxTest, Checkbox_DrawsABoxOfTheSizeTheThemeAsks)
    {
        Context uiContext{kCanvasSize, getPlainTheme()};

        uiContext.checkbox(false);

        EXPECT_EQ(1, boxesOf(uiContext.build().drawList));
    }

    TEST(CheckboxTest, Checkbox_FillsTheBoxOnlyWhenItIsOn)
    {
        Context offContext{kCanvasSize, getPlainTheme()};

        offContext.checkbox(false);

        EXPECT_EQ(0, inkedOf(offContext.build().drawList));

        Context context{kCanvasSize, getPlainTheme()};

        context.checkbox(true);

        EXPECT_EQ(1, inkedOf(context.build().drawList));
    }

    TEST(CheckboxTest, Checkbox_KeepsTheSameSizeWhetherOnOrOff)
    {
        Context offContext{kCanvasSize, getPlainTheme()};
        Context context{kCanvasSize, getPlainTheme()};

        offContext.checkbox(false);
        context.checkbox(true);

        EXPECT_EQ(
            boxesOf(offContext.build().drawList),
            boxesOf(context.build().drawList));
    }

    TEST(CheckboxTest, Checkbox_NamedItCarriesBothABoxAndTheName)
    {
        Context uiContext{kCanvasSize, getPlainTheme()};

        uiContext.checkbox("lighting", CheckboxSpec{.widgetId = kBoxWidget});

        const auto frame = uiContext.build();

        EXPECT_EQ(1, boxesOf(frame.drawList));

        auto toggledFlag = false;

        for (const auto &command : frame.drawList)
        {
            const auto *text =
                std::get_if<antwika::ui::DrawText>(&command);

            toggledFlag = toggledFlag
                    || (text != nullptr && text->text == "lighting");
        }

        EXPECT_TRUE(toggledFlag);
    }

    TEST(CheckboxTest, Checkbox_NamedItAnswersAPressAsOneWidget)
    {
        Context uiContext{kCanvasSize, getPlainTheme()};

        uiContext.checkbox("lighting", CheckboxSpec{.widgetId = kBoxWidget});

        const auto where =
            uiContext.build().rects.getWidgetRect(kBoxWidget).value_or(
                antwika::gfx::Rect{});

        Context againContext{
            kCanvasSize,
            getPlainTheme(),
            Pointer{
                .positionPoint =
                    Point{
                        .x = where.originPoint.x
                             + static_cast<std::int32_t>(
                                 where.size.width / 2),
                        .y = where.originPoint.y
                             + static_cast<std::int32_t>(
                                 where.size.height / 2)},
                .down = true,
                .pressed = true}};

        againContext.checkbox("lighting", CheckboxSpec{.widgetId = kBoxWidget});

        EXPECT_EQ(
            againContext.build().interactions.activatedWidget,
            kBoxWidget);
    }

    TEST(CheckboxTest, Checkbox_NamedItLeavesItsOwnBoxAlone)
    {
        Context uiContext{kCanvasSize, getPlainTheme()};

        uiContext.checkbox("lighting", CheckboxSpec{.widgetId = kBoxWidget});

        EXPECT_EQ(0, inkedOf(uiContext.build().drawList));
    }

}
