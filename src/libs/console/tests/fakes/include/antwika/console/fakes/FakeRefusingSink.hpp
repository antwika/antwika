#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::console::fakes
{

    class FakeRefusingSink final : public antwika::event::ITickEventSink
    {
    public:
        explicit FakeRefusingSink(std::string reason)
            : refusal(std::move(reason))
        {
        }

        void handle(const antwika::event::TickEvent &) override
        {
            throw std::runtime_error(refusal);
        }

    private:
        std::string refusal;
    };

}
