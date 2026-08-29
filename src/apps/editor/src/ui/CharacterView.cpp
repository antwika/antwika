#include "antwika/editor/ui/CharacterView.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/character/Character.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr float kSheetLeft = 80.0F;

        constexpr float kCellGap = 2.0F;

        constexpr float kSheetMargin = 8.0F;

        constexpr float kSheetScale = 1.0F;

        constexpr float kDrawnScale = 6.0F;

                [[nodiscard]] float getCellWide()
        {
            return static_cast<float>(character::kCharacterCellSize.width)
                   * kSheetScale;
        }

        [[nodiscard]] float getCellTall()
        {
            return static_cast<float>(character::kCharacterCellSize.height)
                   * kSheetScale;
        }

        constexpr std::size_t kBankWays = 4;

        constexpr float kBankGap = 8.0F;

        [[nodiscard]] float getBankWide()
        {
            return (static_cast<float>(character::kCharacterFrames)
                    * (getCellWide() + kCellGap))
                   - kCellGap;
        }

        [[nodiscard]] std::size_t getBankCount()
        {
            return character::kCharacterWays / kBankWays;
        }

        [[nodiscard]] float getGridWide()
        {
            return (static_cast<float>(getBankCount()) * getBankWide())
                   + (static_cast<float>(getBankCount() - 1) * kBankGap);
        }

        [[nodiscard]] float getGridTall()
        {
            return (static_cast<float>(kBankWays)
                    * (getCellTall() + kCellGap))
                   - kCellGap;
        }

        [[nodiscard]] float getSheetScale(const gfx::RectF sheetRect)
        {
            return std::min(
                sheetRect.size.width / getGridWide(),
                sheetRect.size.height / getGridTall());
        }

        [[nodiscard]] gfx::RectF drawnAt(
            const gfx::Size canvasSize, const float railWidth)
        {
            const auto width = static_cast<float>(
                                  character::kCharacterCellSize.width)
                              * kDrawnScale;
            const auto height = static_cast<float>(
                                  character::kCharacterCellSize.height)
                              * kDrawnScale;

            return gfx::RectF(
                gfx::PointF{
                    static_cast<float>(canvasSize.width) - width
                        - kSheetMargin - railWidth,
                    kSheetMargin},
                gfx::SizeF{width, height});
        }
    }

    gfx::RectF getCharacterSheetBounds(const gfx::Size canvasSize)
    {
        return gfx::RectF(
            gfx::PointF{kSheetLeft, 0.0F},
            gfx::SizeF{
                getGridWide(), static_cast<float>(canvasSize.height)});
    }

    gfx::RectF getCharacterDrawBounds(
        const gfx::Size canvasSize, const float railWidth)
    {
        return drawnAt(canvasSize, railWidth);
    }

    gfx::RectF getCharacterPlace(
        const gfx::RectF sheetRect,
        const std::size_t direction,
        const std::size_t frame)
    {
        const auto scale = getSheetScale(sheetRect);
        const auto cellWide = getCellWide() * scale;
        const auto cellTall = getCellTall() * scale;
        const auto cellGap = kCellGap * scale;
        const auto bankGap = kBankGap * scale;
        const auto bankWide =
            (static_cast<float>(character::kCharacterFrames)
             * (cellWide + cellGap))
            - cellGap;
        const auto gridWide =
            (static_cast<float>(getBankCount()) * bankWide)
            + (static_cast<float>(getBankCount() - 1) * bankGap);
        const auto gridTall =
            (static_cast<float>(kBankWays) * (cellTall + cellGap))
            - cellGap;
        const auto left =
            sheetRect.originPoint.x
            + ((sheetRect.size.width - gridWide) / 2.0F);
        const auto top =
            sheetRect.originPoint.y
            + ((sheetRect.size.height - gridTall) / 2.0F);
        const auto bank = direction / kBankWays;

        return gfx::RectF(
            gfx::PointF{
                left + (static_cast<float>(bank) * (bankWide + bankGap))
                    + (static_cast<float>(frame) * (cellWide + cellGap)),
                top
                    + (static_cast<float>(direction % kBankWays)
                       * (cellTall + cellGap))},
            gfx::SizeF{cellWide, cellTall});
    }

    std::optional<std::size_t> characterAt(
        const gfx::RectF sheetRect, const gfx::PointF point)
    {
        for (std::size_t direction = 0;
             direction < character::kCharacterWays;
             ++direction)
        {
            for (std::size_t frame = 0; frame < character::kCharacterFrames;
                 ++frame)
            {
                const auto where =
                    getCharacterPlace(sheetRect, direction, frame);

                if (point.x >= where.originPoint.x
                    && point.y >= where.originPoint.y
                    && point.x
                           < where.originPoint.x + where.size.width
                    && point.y
                           < where.originPoint.y + where.size.height)
                {
                    return (direction * character::kCharacterFrames) + frame;
                }
            }
        }

        return std::nullopt;
    }

    gfx::RectF getCharacterCanvasRect(const gfx::RectF drawRect)
    {
        const auto cellWide =
            static_cast<float>(character::kCharacterCellSize.width);
        const auto cellTall =
            static_cast<float>(character::kCharacterCellSize.height);
        const auto roomTall = std::max(
            drawRect.size.height - kPaneMargin, 0.0F);
        const auto scale = std::min(
            drawRect.size.width / cellWide, roomTall / cellTall);
        const auto width = cellWide * scale;
        const auto height = cellTall * scale;

        return gfx::RectF(
            gfx::PointF{
                drawRect.originPoint.x + drawRect.size.width - width,
                drawRect.originPoint.y + kPaneMargin},
            gfx::SizeF{width, height});
    }

}
