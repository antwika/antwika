#pragma once

#include <optional>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    struct ReplayDocument final
    {
        std::vector<TickEvent> events{};

        std::optional<geometry::Size> canvas{};

        [[nodiscard]] bool operator==(
            const ReplayDocument &other) const = default;
    };

}
