#include "antwika/companion/RenderSink.hpp"

#include <antwika/engine/Events.hpp>

#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    RenderSink::RenderSink(
        IWindow &window,
        const PetScene &scene,
        const Pet &pet,
        const Lineage &lineage,
        const Size canvas)
        : window(window),
          scene(scene),
          pet(pet),
          lineage(lineage),
          canvas(canvas)
    {
    }

    void RenderSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        if (!window.isOpen())
        {
            return;
        }

        auto &renderer = window.renderer();
        scene.draw(renderer, canvas, snapshotOf(pet, lineage));
        renderer.present();
    }

} // namespace antwika::companion
