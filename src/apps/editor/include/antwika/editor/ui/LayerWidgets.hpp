#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    inline constexpr widget::WidgetId kFirstLayerWidget{208};

    inline constexpr widget::WidgetId kAddLayerWidget{201};

    inline constexpr widget::WidgetId kRemoveLayerWidget{202};

    [[nodiscard]] widget::WidgetId getLayerWidget(std::size_t layerIndex);

}
