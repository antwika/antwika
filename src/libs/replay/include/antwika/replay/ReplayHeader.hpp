#pragma once

#include <cstdint>
#include <optional>

#include <antwika/geometry/Size.hpp>
#include <antwika/replay/ReplayVersions.hpp>

namespace antwika::replay
{

    struct ReplayHeader final
    {
        std::uint32_t version = kReplayDocumentVersion;

        std::optional<geometry::Size> canvasSize{};

        [[nodiscard]] bool operator==(
            const ReplayHeader &other) const = default;
    };

}
