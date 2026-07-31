#include "antwika/game/SaveLoadScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::DropdownSpec;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::TextFieldSpec;
    using antwika::ui::Theme;

    namespace
    {
        // The same backdrop the main menu uses.
        // The two are the same kind of screen, so they read as siblings.
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        // Wide enough for a file name and three buttons in a row.
        constexpr std::uint32_t kCardWidth = 420;
    } // namespace

    Frame SaveLoadScene::describe(
        Size canvas,
        Pointer pointer,
        const Keyboard &keyboard,
        const SaveLoadState &state) const
    {
        Context ui{
            canvas,
            scaledTheme(Theme{}, scaleForCanvas(canvas)),
            pointer,
            keyboard,
            state.focus()};

        {
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label("SAVE / LOAD");

                ui.dropdown(DropdownSpec{
                    .id = saveWidgets::kPicker,
                    .optionIdBase = saveWidgets::kFirstOption,
                    .width = kGrow,
                    .options = state.options(),
                    .selected = state.selected(),
                    .placeholder = "no saved games",
                    .open = state.listOpen()});

                ui.textField(TextFieldSpec{
                    .id = saveWidgets::kName,
                    .width = kGrow,
                    .text = state.name(),
                    .placeholder = "name a new save",
                    .cursor = state.caret()});

                {
                    const auto row = ui.row({.width = kGrow});

                    ui.button(
                        "Save", {.id = saveWidgets::kSave, .width = kGrow});
                    ui.button(
                        "Load", {.id = saveWidgets::kLoad, .width = kGrow});
                    ui.button(
                        "Back", {.id = saveWidgets::kBack, .width = kGrow});
                }

                // Always declared, so the card is always one height.
                // Nothing then jumps under a click after a message.
                ui.label(state.message(), ui.theme().muted);
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

    void SaveLoadScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

} // namespace antwika::game
