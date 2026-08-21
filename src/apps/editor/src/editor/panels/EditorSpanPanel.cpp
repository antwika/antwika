#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::layoutSpanRows(
        ui::Context &context, const decor::DecorTile &decor)
    {
        context.label(
            "Span " + std::to_string(decor.width) + " x "
                + std::to_string(decor.height),
            kTextColor);

        {
            const auto ways = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            context.button(
                "-",
                antwika::ui::ButtonSpec{
                    .widgetId = decor::
                        kSpanAcrossLessWidget});
            context.button(
                "+",
                antwika::ui::ButtonSpec{
                    .widgetId = decor::
                        kSpanAcrossMoreWidget});
            context.label("across", kTextColor);
        }

        {
            const auto ways = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            context.button(
                "-",
                antwika::ui::ButtonSpec{
                    .widgetId = decor::kSpanDownLessWidget});
            context.button(
                "+",
                antwika::ui::ButtonSpec{
                    .widgetId = decor::kSpanDownMoreWidget});
            context.label("down", kTextColor);
        }

        if (!decor::decorSpanned(decor))
        {
            return;
        }

        for (std::uint8_t row = 0; row < decor.height; ++row)
        {
            const auto places = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            for (std::uint8_t column = 0;
                 column < decor.width;
                 ++column)
            {
                const auto place =
                    (static_cast<std::size_t>(row)
                     * decor.width)
                    + column;

                context.button(
                    std::to_string(place + 1),
                    antwika::ui::ButtonSpec{
                        .widgetId = decor::memberWidget(
                            place),
                        .fillColor =
                            assignMode.memberAssigning
                                    && place
                                           == assignMode
                                                  .memberPicked
                                ? kSelectionAccentColor
                                : kGridLineColor});
            }
        }
    }

}
