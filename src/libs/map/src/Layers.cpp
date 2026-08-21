#include "antwika/map/Layers.hpp"

#include <algorithm>
#include <iterator>

namespace antwika::map
{

    std::vector<Layer> defaultLayers()
    {
        return std::vector<Layer>{
            Layer{.name = std::string(kBaseLayerName)}};
    }

    std::vector<Layer> withLayerAdded(
        const std::vector<Layer> &layers)
    {
        if (layers.size() >= kMaxLayers)
        {
            return layers;
        }

        auto updatedLayers = layers;
        auto layerNumber = std::size_t{1};
        auto label = std::string();

        do
        {
            label = "Decor " + std::to_string(layerNumber);
            ++layerNumber;
        } while (
            std::any_of(
                updatedLayers.begin(),
                updatedLayers.end(),
                [&label](const Layer &layer)
                { return layer.name == label; }));

        updatedLayers.push_back(Layer{.name = label});

        return updatedLayers;
    } // GCOVR_EXCL_LINE

    std::vector<Layer> withLayerRemoved(
        const std::vector<Layer> &layers, const std::size_t layerIndex)
    {
        if (layerIndex == kBaseLayer || layerIndex >= layers.size())
        {
            return layers;
        }

        auto updatedLayers = layers;

        updatedLayers.erase(
            std::next(updatedLayers.begin(),
            static_cast<std::ptrdiff_t>(layerIndex)));

        return updatedLayers;
    } // GCOVR_EXCL_LINE

    std::string layerLabel(const std::size_t layerIndex)
    {
        return layerIndex == kBaseLayer
                           ? std::string(kBaseLayerName)
                           : "Decor " + std::to_string(layerIndex);
    } // GCOVR_EXCL_LINE

    ui::WidgetId layerWidget(const std::size_t layerIndex)
    {
        return static_cast<ui::WidgetId>(
            static_cast<std::uint64_t>(kFirstLayerWidget)
            + (layerIndex % kMaxLayers));
    }

}
