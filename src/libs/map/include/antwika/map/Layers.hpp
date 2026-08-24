#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace antwika::map
{

    inline constexpr std::string_view kBaseLayerName = "Base layer";

    inline constexpr std::size_t kBaseLayer = 0;

    inline constexpr std::size_t kMaxLayers = 16;

    struct Layer final
    {
        std::string name;

        [[nodiscard]] bool operator==(const Layer &other) const
            = default;
    };

    [[nodiscard]] std::vector<Layer> getDefaultLayers();

    [[nodiscard]] std::vector<Layer> getWithLayerAdded(
        const std::vector<Layer> &layers);

    [[nodiscard]] std::string getLayerLabel(std::size_t layerIndex);

    [[nodiscard]] std::vector<Layer> getWithLayerRemoved(
        const std::vector<Layer> &layers, std::size_t layerIndex);

}
