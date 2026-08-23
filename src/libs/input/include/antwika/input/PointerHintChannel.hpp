#pragma once
#include <optional>

#include <antwika/event/ITickEventSource.hpp>

#include "antwika/input/PointerHint.hpp"

namespace antwika::input
{

    using antwika::event::ITickEventSource;

    class PointerHintChannel final
    {
    public:
        void publish(PointerHint hint) noexcept;

        [[nodiscard]] std::optional<PointerHint>
        getLatest() const noexcept;

    private:
        std::optional<PointerHint> latestHint;
    };

}
