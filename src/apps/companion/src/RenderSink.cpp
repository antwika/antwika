#include "antwika/companion/RenderSink.hpp"

#include <antwika/app/FramePresentation.hpp>

#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    RenderSink::RenderSink(
        IWindow &window,
        const PetScene &scene,
        const Pet &pet,
        const Lineage &lineage,
        const Size canvas,
        const antwika::console::ConsolePicture &consolePicture)
        : window(window),
          scene(scene),
          pet(pet),
          lineage(lineage),
          canvas(canvas),
          consolePicture(consolePicture)
    {
    }

    void RenderSink::handle(const TickEvent &event)
    {
        if (!antwika::app::drawsOn(event, window))
        {
            return;
        }

        // The console last of all, so it stands over the companion.
        // Described in the tick path; painted here only.
        antwika::app::presentFrame(
            window,
            consolePicture,
            [this](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(renderer, canvas, snapshotOf(pet, lineage));
            });
    }

} // namespace antwika::companion
