#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class BindingSource final : public ITickEventSource
    {
    public:
        BindingSource(
            ITickEventSource &inner,
            std::optional<KeyBindings> announced) noexcept;

        BindingSource(const BindingSource &) = delete;
        BindingSource(BindingSource &&) = delete;

        BindingSource &operator=(const BindingSource &) = delete;
        BindingSource &operator=(BindingSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<KeyBindings> announced;
    };

}
