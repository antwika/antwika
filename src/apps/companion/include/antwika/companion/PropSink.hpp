#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    class PropSink final : public ITickEventSink
    {
    public:
        PropSink(Pet &pet, const IInputEventCodec &codec, Size canvas);

        PropSink(const PropSink &) = delete;
        PropSink(PropSink &&) = delete;

        PropSink &operator=(const PropSink &) = delete;
        PropSink &operator=(PropSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        const IInputEventCodec &codec;
        Size canvas;
    };

}
