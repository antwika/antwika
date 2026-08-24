#include "antwika/map/Layers.hpp"

#include <algorithm>
#include <iterator>

namespace antwika::map
{

    std::vector<Layer> getDefaultLayers()
    {
        return std::vector<Layer>{
            Layer{.name = std::string(kBaseLayerName)}};
    }

    std::vector<Layer> getWithLayerAdded(
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

    std::vector<Layer> getWithLayerRemoved(
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

    std::string getLayerLabel(const std::size_t layerIndex)
    {
        return layerIndex == kBaseLayer
                           ? std::string(kBaseLayerName)
                           : "Decor " + std::to_string(layerIndex);
    } // GCOVR_EXCL_LINE

}
