#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::map
{

    inline constexpr std::string_view kBaseLayerName = "Base layer";

    inline constexpr std::size_t kBaseLayer = 0;

    inline constexpr std::size_t kMaxLayers = 16;

    inline constexpr widget::WidgetId kFirstLayerWidget{208};

    inline constexpr widget::WidgetId kAddLayerWidget{201};

    inline constexpr widget::WidgetId kRemoveLayerWidget{202};

    struct Layer final
    {
        std::string name;

        [[nodiscard]] bool operator==(const Layer &other) const
            = default;
    };

    [[nodiscard]] std::vector<Layer> defaultLayers();

    [[nodiscard]] std::vector<Layer> withLayerAdded(
        const std::vector<Layer> &layers);

    [[nodiscard]] std::string layerLabel(std::size_t layerIndex);

    [[nodiscard]] std::vector<Layer> withLayerRemoved(
        const std::vector<Layer> &layers, std::size_t layerIndex);

    [[nodiscard]] widget::WidgetId layerWidget(std::size_t layerIndex);

}
