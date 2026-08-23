#pragma once

#include <cstdint>
#include <optional>

#include <antwika/input/Position.hpp>
#include <antwika/ui/ClickTrack.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/editor/state/CanvasRest.hpp"

namespace antwika::editor
{

    struct PointerTrack final
    {
        std::optional<gfx::Point> pointerInWindow;

        gfx::PointF pointerOnCanvas{};

        input::Position lastPointerPosition{};

        bool pointerHeld = false;

        widget::WidgetId hoveredWidget = widget::kNoWidget;

        std::optional<voxel::VoxelPosition> hoveredPosition;

        ui::HoverTrack hoverTracker;

        ui::ClickTrack clickTracker;

        widget::WidgetId lastPickedWidget = widget::kNoWidget;

        std::uint32_t lastPickedAt = 0;

        CanvasRest canvasRest;
    };

}
