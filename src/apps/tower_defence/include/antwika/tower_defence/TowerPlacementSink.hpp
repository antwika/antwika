#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    class TowerPlacementSink final : public ITickEventSink
    {
    public:
        TowerPlacementSink(
            Campaign &campaign,
            const IInputEventCodec &codec,
            Size canvas);

        TowerPlacementSink(const TowerPlacementSink &) = delete;
        TowerPlacementSink(TowerPlacementSink &&) = delete;

        TowerPlacementSink &operator=(const TowerPlacementSink &) = delete;
        TowerPlacementSink &operator=(TowerPlacementSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Campaign &campaign;
        const IInputEventCodec &codec;
        Size canvas;
    };

}
