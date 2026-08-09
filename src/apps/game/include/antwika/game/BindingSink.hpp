#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class BindingSink final : public ITickEventSink
    {
    public:
        explicit BindingSink(OptionsState &options) noexcept;

        BindingSink(const BindingSink &) = delete;
        BindingSink(BindingSink &&) = delete;

        BindingSink &operator=(const BindingSink &) = delete;
        BindingSink &operator=(BindingSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        OptionsState &options;
    };

}
