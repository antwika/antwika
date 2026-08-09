#pragma once

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetScene.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    class RenderSink final : public ITickEventSink
    {
    public:
        RenderSink(
            IWindow &window,
            const PetScene &scene,
            const Pet &pet,
            const Lineage &lineage,
            Size canvas,
            const antwika::console::ConsolePicture &consolePicture);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const PetScene &scene;
        const Pet &pet;
        const Lineage &lineage;
        Size canvas;
        const antwika::console::ConsolePicture &consolePicture;
    };

}
