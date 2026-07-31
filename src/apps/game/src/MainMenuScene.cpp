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
        // Darker than the grid's sky, so the two modes do not look like
        // the same screen with something laid over it.
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        // Wide enough that every item is the same width: the items grow
        // into a card that does not, so the row a pointer is over is
        // decided by height alone.
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

                // TODO(game-save-load): give this an id and a mode of
                // its own once there is a saved game to load.
                // Unnamed on purpose: a button with no WidgetId cannot
                // be hovered or activated, so the placeholder is inert
                // rather than merely painted to look that way.
                ui.button(
                    "Load Game",
                    {.width = kGrow, .state = ButtonState::Pressed});

                // TODO(game-world-map): likewise, once the world map and
                // its cities exist to be selected from here.
                ui.button(
                    "World Map",
                    {.width = kGrow, .state = ButtonState::Pressed});

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
