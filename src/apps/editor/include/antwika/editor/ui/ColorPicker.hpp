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

    using gfx::colorFromHex;
    using gfx::colorOf;
    using gfx::colorToHex;
    using gfx::Hsv;
    using gfx::hsvOf;

    inline constexpr float kFieldSide = 66.0F;

    inline constexpr float kHueStripWidth = 10.0F;

    inline constexpr float kPickerGap = 4.0F;

    inline constexpr float kPickerPad = 4.0F;

    inline constexpr float kPickerTop = kTopBarHeight;

    inline constexpr std::size_t kPickerBands = 22;

    [[nodiscard]] gfx::RectF pickerPlace(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF fieldPlace(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF huePlace(gfx::Size canvasSize);

    [[nodiscard]] bool onPicker(gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<Hsv> colorAtPoint(
        gfx::Size canvasSize, Hsv currentHsv, gfx::PointF point);

    [[nodiscard]] gfx::PointF fieldCursorPos(
        gfx::Size canvasSize, Hsv colorHsv);

    [[nodiscard]] float hueCursorPos(gfx::Size canvasSize, Hsv colorHsv);

    [[nodiscard]] gfx::RectF bandPlace(
        gfx::Size canvasSize, std::size_t column, std::size_t row);

    [[nodiscard]] Hsv bandHsv(
        Hsv colorHsv, std::size_t column, std::size_t row);

    [[nodiscard]] gfx::RectF hueBandPlace(
        gfx::Size canvasSize, std::size_t bandIndex);

    [[nodiscard]] float hueBand(std::size_t bandIndex);

}
