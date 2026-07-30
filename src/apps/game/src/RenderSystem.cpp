#include "antwika/game/RenderSystem.hpp"

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    RenderSystem::RenderSystem(
        IWindow &window,
        const GridScene &scene,
        const ITexture &atlas,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent)
        : window(window),
          scene(scene),
          atlas(atlas),
          paths(paths),
          camera(camera),
          extent(extent)
    {
    }

    void RenderSystem::update(World &world, antwika::time::Tick)
    {
        auto &renderer = window.renderer();

        scene.draw(
            renderer,
            window.size(),
            snapshotOf(world, paths, camera, extent),
            atlas);
        renderer.present();
    }

} // namespace antwika::game
