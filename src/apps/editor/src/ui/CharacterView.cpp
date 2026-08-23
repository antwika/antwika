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

        [[nodiscard]] gfx::RectF drawnAt(const gfx::Size canvasSize)
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
                        - kSheetMargin - kRightPanelWidth,
                    kSheetMargin},
                gfx::SizeF{width, height});
        }
    }

    gfx::RectF getCharacterPlace(
        const gfx::Size canvasSize,
        const std::size_t direction,
        const std::size_t frame)
    {
        const auto height =
            ((getCellTall() + kCellGap)
             * static_cast<float>(kBankWays))
            - kCellGap;
        const auto top =
            (static_cast<float>(canvasSize.height) - height) / 2.0F;
        const auto bank = direction / kBankWays;

        return gfx::RectF(
            gfx::PointF{
                kSheetLeft
                    + (static_cast<float>(bank)
                       * (getBankWide() + kBankGap))
                    + (static_cast<float>(frame)
                       * (getCellWide() + kCellGap)),
                top
                    + (static_cast<float>(direction % kBankWays)
                       * (getCellTall() + kCellGap))},
            gfx::SizeF{getCellWide(), getCellTall()});
    }

    std::optional<std::size_t> characterAt(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        for (std::size_t direction = 0;
             direction < character::kCharacterWays;
             ++direction)
        {
            for (std::size_t frame = 0; frame < character::kCharacterFrames;
                 ++frame)
            {
                const auto where =
                    getCharacterPlace(canvasSize, direction, frame);

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

    gfx::RectF getCharacterCanvasRect(const gfx::Size canvasSize)
    {
        return drawnAt(canvasSize);
    }

}
