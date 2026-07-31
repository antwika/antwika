#include "antwika/game/MainMenuScene.hpp"

#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        // Darker than the grid's sky.
        // The two modes must not read as one screen with a layer over it.
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        // Wide enough that every item comes out the same width.
        // The items grow into a card that does not.
        // So which item a pointer is over is decided by height alone.
        constexpr std::uint32_t kCardWidth = 260;
    } // namespace

    Frame MainMenuScene::describe(Size canvas, Pointer pointer) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label("ANTWIKA");

                ui.button(
                    "New Game",
                    {.id = menuWidgets::kNewGame, .width = kGrow});

                // TODO(game-save-load): give this an id and a mode.
                // That is for whatever adds a saved game to load.
                // It is unnamed on purpose until then.
                // A button with no WidgetId cannot be hovered.
                // So the placeholder is inert, not merely painted so.
                ui.button(
                    "Load Game",
                    {.width = kGrow, .state = ButtonState::Pressed});

                ui.button(
                    "World Map",
                    {.id = menuWidgets::kWorldMap, .width = kGrow});

                ui.button(
                    "Quit", {.id = menuWidgets::kQuit, .width = kGrow});
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

    void MainMenuScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

} // namespace antwika::game
