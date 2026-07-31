#include "antwika/game/RenderSystem.hpp"

#include <antwika/ui/Painter.hpp>

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const GridScene &scene,
        const ITexture &atlas,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent,
        const UiOverlay &overlay,
        const AppModeState &mode,
        const MainMenuScene &menuScene,
        const UiOverlay &menuOverlay)
        : window(window),
          scene(scene),
          atlas(atlas),
          paths(paths),
          camera(camera),
          extent(extent),
          overlay(overlay),
          mode(mode),
          menuScene(menuScene),
          menuOverlay(menuOverlay)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        auto &renderer = window.renderer();

        if (mode.mode() == AppMode::MainMenu)
        {
            // The whole screen, with no grid behind it: the menu is a
            // mode rather than a modal -- see AppMode.hpp.
            menuScene.draw(renderer, menuOverlay.commands());
            renderer.present();
            return;
        }

        scene.draw(
            renderer,
            window.size(),
            snapshotOf(world, paths, camera, extent),
            atlas);

        // Over the grid, and last, so the bar reads as being in front.
        // Laid out against the size the window was asked for.
        // That is the size UiSink described it against.
        // And the size a recorded click was resolved against.
        antwika::ui::paint(renderer, overlay.commands());
        renderer.present();
    }

} // namespace antwika::game
