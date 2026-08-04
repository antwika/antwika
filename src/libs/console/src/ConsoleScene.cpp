#include "antwika/console/ConsoleScene.hpp"

#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

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
        // Darker than the toolbar, so the sheet reads as over it.
        constexpr Color kConsoleBackdrop{.red = 14, .green = 16, .blue = 24};

        // A prompt rather than a caption, so it is a literal.
        // The class comment says why no console word is a MessageId.
        constexpr std::string_view kPrompt = "type a command";
    } // namespace

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
                // Bottom-anchored, so the content rides the lower edge.
                ui.spacer(kGrow);

                const auto &lines = state.history();
                const auto first = lines.size() > kConsoleHistoryShown
                    ? lines.size() - kConsoleHistoryShown
                    : 0;

                for (auto at = first; at < lines.size(); ++at)
                {
                    ui.label(lines[at]);
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

} // namespace antwika::console
