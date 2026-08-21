#pragma once

#include <optional>
#include <string>

#include <antwika/map/MapFile.hpp>

namespace antwika::map
{

    struct Progress final
    {
        std::string map{};

        Placement stancePlacement{};

        [[nodiscard]] bool operator==(const Progress &other) const
            = default;
    };

    void saveProgress(
        const Progress &progress, const std::string &path);

    [[nodiscard]] std::optional<Progress> loadProgress(
        const std::string &path);

}
