#pragma once

#include <cstdint>
#include <optional>

#include <antwika/geometry/Size.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    struct ReplayHeader final
    {
        std::uint32_t version = kReplayDocumentVersion;

        std::optional<geometry::Size> canvas{};

        [[nodiscard]] bool operator==(
            const ReplayHeader &other) const = default;
    };

}
