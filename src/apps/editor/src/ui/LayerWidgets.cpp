#include "antwika/editor/ui/LayerWidgets.hpp"

#include <antwika/map/Layers.hpp>

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    widget::WidgetId getLayerWidget(const std::size_t layerIndex)
    {
        return getWidgetAfter(
            kFirstLayerWidget, layerIndex % map::kMaxLayers);
    }

}
