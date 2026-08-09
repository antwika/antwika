#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class KeyboardSource final : public ITickEventSource
    {
    public:
        KeyboardSource(
            ITickEventSource &inner,
            std::optional<KeyboardLayout> announced) noexcept;

        KeyboardSource(const KeyboardSource &) = delete;
        KeyboardSource(KeyboardSource &&) = delete;

        KeyboardSource &operator=(const KeyboardSource &) = delete;
        KeyboardSource &operator=(KeyboardSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<KeyboardLayout> announced;
    };

}
