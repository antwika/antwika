#include "antwika/companion/RenderSink.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Painter.hpp>

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

        // The console last of all, so it stands over the companion.
        // Described in the tick path; painted here only.
        antwika::ui::paint(renderer, consolePicture.commands());

        renderer.present();
    }

} // namespace antwika::companion
