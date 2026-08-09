#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    class ReviveSink final : public ITickEventSink
    {
    public:
        ReviveSink(
            Pet &pet,
            Lineage &lineage,
            const IInputEventCodec &codec,
            Size canvas);

        ReviveSink(const ReviveSink &) = delete;
        ReviveSink(ReviveSink &&) = delete;

        ReviveSink &operator=(const ReviveSink &) = delete;
        ReviveSink &operator=(ReviveSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        Lineage &lineage;
        const IInputEventCodec &codec;
        Size canvas;
    };

}
