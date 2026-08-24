#include "antwika/editor/ui/LayerWidgets.hpp"

#include <antwika/map/Layers.hpp>

namespace antwika::editor
{

    widget::WidgetId getLayerWidget(const std::size_t layerIndex)
    {
        return static_cast<widget::WidgetId>(
            static_cast<std::uint64_t>(kFirstLayerWidget)
            + (layerIndex % map::kMaxLayers));
    }

}
