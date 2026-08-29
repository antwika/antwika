#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    [[nodiscard]] widget::WidgetId getLayerWidget(std::size_t layerIndex);

}
