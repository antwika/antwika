#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class RestoreSink final : public ITickEventSink
    {
    public:
        RestoreSink(Pet &pet, Lineage &lineage);

        RestoreSink(const RestoreSink &) = delete;
        RestoreSink(RestoreSink &&) = delete;

        RestoreSink &operator=(const RestoreSink &) = delete;
        RestoreSink &operator=(RestoreSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
        Lineage &lineage;
    };

}
