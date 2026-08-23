#include "antwika/editor/ui/ColorPicker.hpp"

#include <algorithm>
#include <cmath>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    namespace
    {
        [[nodiscard]] float getWrapped(const float turns)
        {
            return turns - std::floor(turns);
        }

        [[nodiscard]] float fractionAlong(
            const float value, const float start, const float span)
        {
            return span <= 0.0F
                         ? 0.0F
                         : std::clamp((value - start) / span, 0.0F, 1.0F);
        }
    }

    gfx::RectF getPickerPlace(const gfx::Size canvasSize)
    {
        const auto width = (2.0F * kPickerPad) + kFieldSide
                          + kPickerGap + kHueStripWidth;

        return gfx::RectF(
            gfx::PointF{
                static_cast<float>(canvasSize.width) - kPaneMargin - width
                    - kRightPanelWidth,
                kPickerTop},
            gfx::SizeF{
                width, (2.0F * kPickerPad) + kFieldSide});
    }

    gfx::RectF getFieldPlace(const gfx::Size canvasSize)
    {
        const auto panel = getPickerPlace(canvasSize);

        return gfx::RectF(
            gfx::PointF{
                panel.originPoint.x + kPickerPad,
                panel.originPoint.y + kPickerPad},
            gfx::SizeF{kFieldSide, kFieldSide});
    }

    gfx::RectF getHuePlace(const gfx::Size canvasSize)
    {
        const auto field = getFieldPlace(canvasSize);

        return gfx::RectF(
            gfx::PointF{
                field.originPoint.x + kFieldSide + kPickerGap,
                field.originPoint.y},
            gfx::SizeF{kHueStripWidth, kFieldSide});
    }

    bool isOnPicker(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        return holds(getPickerPlace(canvasSize), point);
    }

    std::optional<Hsv> getColorAtPoint(
        const gfx::Size canvasSize,
        const Hsv currentHsv,
        const gfx::PointF point)
    {
        const auto field = getFieldPlace(canvasSize);

        if (holds(field, point))
        {
            return Hsv{
                .hue = currentHsv.hue,
                .saturation = fractionAlong(
                    point.x, field.originPoint.x, field.size.width),
                .value =
                    1.0F
                    - fractionAlong(
                        point.y, field.originPoint.y, field.size.height)};
        }

        const auto strip = getHuePlace(canvasSize);

        if (holds(strip, point))
        {
            return Hsv{
                .hue = fractionAlong(
                    point.y, strip.originPoint.y, strip.size.height),
                .saturation = currentHsv.saturation,
                .value = currentHsv.value};
        }

        return std::nullopt;
    }

    gfx::PointF getFieldCursorPos(
        const gfx::Size canvasSize, const Hsv colorHsv)
    {
        const auto field = getFieldPlace(canvasSize);

        return gfx::PointF{
            field.originPoint.x
                + (std::clamp(colorHsv.saturation, 0.0F, 1.0F)
                   * field.size.width),
            field.originPoint.y
                + ((1.0F - std::clamp(colorHsv.value, 0.0F, 1.0F))
                   * field.size.height)};
    }

    float getHueCursorPos(const gfx::Size canvasSize, const Hsv colorHsv)
    {
        const auto strip = getHuePlace(canvasSize);

        return strip.originPoint.y
               + (getWrapped(colorHsv.hue) * strip.size.height);
    }

    gfx::RectF getBandPlace(
        const gfx::Size canvasSize,
        const std::size_t column,
        const std::size_t row)
    {
        const auto field = getFieldPlace(canvasSize);
        const auto side =
            field.size.width / static_cast<float>(kPickerBands);

        return gfx::RectF(
            gfx::PointF{
                field.originPoint.x + (static_cast<float>(column) * side),
                field.originPoint.y + (static_cast<float>(row) * side)},
            gfx::SizeF{side, side});
    }

    Hsv getBandHsv(
        const Hsv colorHsv,
        const std::size_t column,
        const std::size_t row)
    {
        const auto steps = static_cast<float>(kPickerBands - 1);

        return Hsv{
            .hue = colorHsv.hue,
            .saturation = static_cast<float>(column) / steps,
            .value = 1.0F - (static_cast<float>(row) / steps)};
    }

    gfx::RectF getHueBandPlace(
        const gfx::Size canvasSize, const std::size_t bandIndex)
    {
        const auto strip = getHuePlace(canvasSize);
        const auto height =
            strip.size.height / static_cast<float>(kPickerBands);

        return gfx::RectF(
            gfx::PointF{
                strip.originPoint.x,
                strip.originPoint.y + (static_cast<float>(bandIndex) * height)},
            gfx::SizeF{strip.size.width, height});
    }

    float getHueBand(const std::size_t bandIndex)
    {
        return static_cast<float>(bandIndex)
               / static_cast<float>(kPickerBands);
    }

}
