#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ColorSpace.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/tilemap/Tilemap.hpp>
#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    using gfx::getColorFromHex;
    using gfx::colorOf;
    using gfx::getColorToHex;
    using gfx::Hsv;
    using gfx::hsvOf;

    inline constexpr float kFieldSide = 66.0F;

    inline constexpr float kHueStripWidth = 10.0F;

    inline constexpr float kPickerGap = 4.0F;

    inline constexpr float kPickerPad = 4.0F;

    inline constexpr float kPickerTop = kTopBarHeight;

    inline constexpr std::size_t kPickerBands = 22;

    [[nodiscard]] gfx::RectF getPickerPlace(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF getFieldPlace(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF getHuePlace(gfx::Size canvasSize);

    [[nodiscard]] bool isOnPicker(gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<Hsv> getColorAtPoint(
        gfx::Size canvasSize, Hsv currentHsv, gfx::PointF point);

    [[nodiscard]] gfx::PointF getFieldCursorPos(
        gfx::Size canvasSize, Hsv colorHsv);

    [[nodiscard]] float getHueCursorPos(gfx::Size canvasSize, Hsv colorHsv);

    [[nodiscard]] gfx::RectF getBandPlace(
        gfx::Size canvasSize, std::size_t column, std::size_t row);

    [[nodiscard]] Hsv getBandHsv(
        Hsv colorHsv, std::size_t column, std::size_t row);

    [[nodiscard]] gfx::RectF getHueBandPlace(
        gfx::Size canvasSize, std::size_t bandIndex);

    [[nodiscard]] float getHueBand(std::size_t bandIndex);

}
