#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class PetSink final : public ITickEventSink
    {
    public:
        explicit PetSink(Pet &pet);

        PetSink(const PetSink &) = delete;
        PetSink(PetSink &&) = delete;

        PetSink &operator=(const PetSink &) = delete;
        PetSink &operator=(PetSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        Pet &pet;
    };

}
