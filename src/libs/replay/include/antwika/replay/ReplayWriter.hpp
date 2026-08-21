#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    class ReplayWriter final
    {
    public:
        explicit ReplayWriter(
            std::optional<geometry::Size> canvasSize = std::nullopt) noexcept;

        void writeHeader(std::ostream &outputStream) const;

        void writeRecord(
            const TickEvent &event, std::ostream &outputStream) const;

        void write(
            const std::vector<TickEvent> &events,
            std::ostream &outputStream) const;

    private:
        std::optional<geometry::Size> canvas;
    };

}
