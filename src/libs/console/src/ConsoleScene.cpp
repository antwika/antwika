#include "antwika/console/ConsoleScene.hpp"

#include <cstddef>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/console/HistoryRows.hpp"

namespace antwika::console
{

    using antwika::gfx::Color;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kGrow;
    using antwika::ui::kNoWidget;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::TextFieldSpec;
    using antwika::ui::Theme;

    namespace
    {
        constexpr Color kConsoleBackdrop{.red = 14, .green = 16, .blue = 24};

        constexpr std::string_view kPrompt = "type a command";

        [[nodiscard]] std::size_t columnsAcross(
            Size canvas, const Theme &theme) noexcept
        {
            return canvas.width
                / (antwika::gfx::kGlyphAdvance * theme.textScale);
        }
    }

    Frame ConsoleScene::describe(
        Size canvas,
        Pointer pointer,
        const Keyboard &keyboard,
        const ConsoleState &state) const
    {
        Context ui{
            canvas,
            scaledTheme(Theme{}, scaleForCanvas(canvas)),
            pointer,
            keyboard,
            state.acceptsText() ? consoleWidgets::kInput : kNoWidget};

        if (state.visible())
        {
            const auto sheet = ui.column(
                {.width = kGrow,
                 .height = fixedSize(state.height()),
                 .background = kConsoleBackdrop,
                 .id = consoleWidgets::kSheet});

            if (state.acceptsText())
            {
                ui.spacer(kGrow);

                const auto rows = historyRows(
                    state.history(),
                    columnsAcross(canvas, ui.theme()),
                    kConsoleHistoryShown);

                for (const auto &row : rows)
                {
                    ui.label(row);
                }

                ui.textField(TextFieldSpec{
                    .id = consoleWidgets::kInput,
                    .width = kGrow,
                    .text = state.line(),
                    .placeholder = kPrompt,
                    .cursor = state.caret()});
            }
        }

        return ui.finish();
    }

}
