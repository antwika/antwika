#include "antwika/character/Character.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <numbers>
#include <set>
#include <tuple>
#include <utility>

#include <antwika/geometry/GridStep.hpp>
#include <antwika/animation/Playback.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/tile/TilePaint.hpp>

namespace antwika::character
{

    namespace
    {
        constexpr float kPixelClear = 1.0F;

        constexpr std::array<std::string_view, kCharacterWays>
            kWayNames{
                "east",
                "south east",
                "south",
                "south west",
                "west",
                "north west",
                "north",
                "north east"};

    }

    gfx::Size getCharacterSheetSize()
    {
        return gfx::Size{
            .width =
                kCharacterCellSize.width
                * static_cast<std::uint32_t>(kCharacterFrames),
            .height =
                kCharacterCellSize.height
                * static_cast<std::uint32_t>(kCharacterWays)};
    }

    gfx::Bitmap getBlankCharacter()
    {
        const auto size = getCharacterSheetSize();
        const auto blank = tile::kPaletteColors.front();

        gfx::Bitmap bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * static_cast<std::size_t>(size.height)
                    * gfx::kBytesPerPixel,
                0)};

        for (std::size_t index = 0; index < bitmap.pixels.size();
             index += gfx::kBytesPerPixel)
        {
            bitmap.pixels[index] = blank.red;
            bitmap.pixels[index + 1] = blank.green;
            bitmap.pixels[index + 2] = blank.blue;
        }

        return bitmap;
    } // GCOVR_EXCL_LINE

    std::string_view getDirectionName(const std::size_t direction)
    {
        return kWayNames.at(direction % kCharacterWays);
    }

    gfx::Rect getCharacterCell(
        const std::size_t direction, const std::size_t frame)
    {
        return gfx::Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(
                     frame * kCharacterCellSize.width),
                 .y = static_cast<std::int32_t>(
                     direction * kCharacterCellSize.height)},
            .size = kCharacterCellSize};
    }

    gfx::Color getCharacterPaletteColor(
        const std::span<const gfx::Color> paletteColors,
        const std::size_t which)
    {
        if (which == 0)
        {
            return gfx::Color{
                .red = 0, .green = 0, .blue = 0, .alpha = 0};
        }

        return paletteColors[which];
    }

    gfx::Vec3 headTopOf(const component::Position stoodPosition)
    {
        return gfx::Vec3{
            stoodPosition.x,
            stoodPosition.y - collision::kFootprintPivotY + kCharacterTall
                + kSpriteLift,
            stoodPosition.z};
    } // GCOVR_EXCL_LINE

    gfx::MeshData getCharacterMesh()
    {
        const auto arm = kCharacterWide * 0.5F;
        gfx::MeshData mesh;

        const auto foot = -collision::kFootprintPivotY;
        const auto head = foot + kCharacterTall;
        const auto lying = -foot * kSpriteGroundSkew;
        const auto uprightSkew = head * kSpriteUprightSkew;
        const auto seam = head / kCharacterTall;

        for (const auto &[byX, byY, byZ, atU, atV] :
             {std::tuple{-arm, foot, lying, 0.0F, 1.0F},
              std::tuple{arm, foot, lying, 1.0F, 1.0F},
              std::tuple{arm, 0.0F, 0.0F, 1.0F, seam},
              std::tuple{-arm, 0.0F, 0.0F, 0.0F, seam},
              std::tuple{arm, head, uprightSkew, 1.0F, 0.0F},
              std::tuple{-arm, head, uprightSkew, 0.0F, 0.0F}})
        {
            mesh.vertices.push_back(
                gfx::Vertex3D{ // GCOVR_EXCL_LINE
                    .position = gfx::Vec3{byX, byY, byZ},
                    .normal = gfx::Vec3{0.0F, 1.0F, 0.0F},
                    .texCoordinate = gfx::Vec2{atU, atV}});
        }

        mesh.indices = {0, 1, 2, 0, 2, 3, 3, 2, 4, 3, 4, 5};

        return mesh;
    } // GCOVR_EXCL_LINE

    gfx::Mat4 getSpriteBillboardMatrix(
        const gfx::Vec3 position, const gfx::Mat4 &viewMatrix)
    {
        const auto rotationMatrix = gfx::Mat4(glm::mat3(viewMatrix));
        const auto toWorld = glm::transpose(glm::mat3(viewMatrix));
        const auto sinkHeight =
            collision::kFootprintPivotY
            * std::max(
                toWorld[1].y - (kSpriteGroundSkew * toWorld[2].y),
                0.0F);
        const gfx::Vec3 overPosition{
            position.x,
            position.y + kSpriteDepthBias + sinkHeight + kSpriteLift,
            position.z};

        return glm::translate(gfx::getIdentityMatrix(), overPosition)
               * glm::transpose(rotationMatrix);
    }

    gfx::Vec3 getFrameUvOffset(
        const std::size_t direction, const std::size_t frame)
    {
        const auto sheet = getCharacterSheetSize();
        const auto cell = getCharacterCell(direction, frame);

        return gfx::Vec3{
            static_cast<float>(cell.originPoint.x)
                / static_cast<float>(sheet.width),
            static_cast<float>(cell.originPoint.y)
                / static_cast<float>(sheet.height),
            0.0F};
    }

    gfx::Vec3 getFrameUvSize()
    {
        const auto sheet = getCharacterSheetSize();

        return gfx::Vec3{
            static_cast<float>(kCharacterCellSize.width)
                / static_cast<float>(sheet.width),
            static_cast<float>(kCharacterCellSize.height)
                / static_cast<float>(sheet.height),
            0.0F};
    }

    gfx::RectF getCharacterSource(
        const std::size_t direction, const std::size_t frame)
    {
        const auto cell = getCharacterCell(direction, frame);

        return gfx::RectF(
            gfx::PointF{
                static_cast<float>(cell.originPoint.x),
                static_cast<float>(cell.originPoint.y)},
            gfx::SizeF{
                static_cast<float>(cell.size.width),
                static_cast<float>(cell.size.height)});
    }

    std::optional<std::size_t> getFacingFromVelocity(
        const component::Velocity velocity)
    {
        if (velocity.velocityX == 0.0F && velocity.velocityZ == 0.0F)
        {
            return std::nullopt;
        }

        const auto turn =
            std::atan2(velocity.velocityZ, velocity.velocityX);
        const auto eighth = std::numbers::pi_v<float> / 4.0F;
        const auto frameCount =
            static_cast<std::int32_t>(std::lround(turn / eighth));
        const auto ways = static_cast<std::int32_t>(kCharacterWays);

        return static_cast<std::size_t>(((frameCount % ways) + ways)
                                        % ways);
    }

    animation::Clip getWalkingClip(const std::size_t direction)
    {
        return animation::getUniformClip(
            (direction % kCharacterWays) * kCharacterFrames,
            kCharacterFrames,
            kCharacterPaceTick);
    } // GCOVR_EXCL_LINE

    animation::Clip getStandingClip(const std::size_t direction)
    {
        return animation::getUniformClip(
            (direction % kCharacterWays) * kCharacterFrames,
            1,
            kCharacterPaceTick);
    } // GCOVR_EXCL_LINE

    std::size_t getCurrentFrame(
        const component::AnimationState posedState,
        const time::Tick tick)
    {
        const auto clip =
            posedState.walking ? getWalkingClip(posedState.direction)
                          : getStandingClip(posedState.direction);
        const auto elapsedTicks =
            tick >= posedState.startedAtTick
                  ? tick - posedState.startedAtTick
                  : 0;

        return animation::getFrameAt(clip, elapsedTicks).index;
    }

    geometry::GridCell getCharacterPixel(
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell pixelCell)
    {
        const auto cell = getCharacterCell(direction, frame);

        return geometry::GridCell{
            .column = static_cast<std::uint32_t>(cell.originPoint.x)
                      + pixelCell.column,
            .row = static_cast<std::uint32_t>(cell.originPoint.y)
                   + pixelCell.row};
    }

    void paintCharacter(
        gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell pixelCell,
        const gfx::Color color)
    {
        const auto sheetCell = getCharacterPixel(direction, frame, pixelCell);

        gfx::setColorAt(
            sheetBitmap,
            static_cast<std::int32_t>(sheetCell.column),
            static_cast<std::int32_t>(sheetCell.row),
            color);
    }

    void paintCharacterLine(
        gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell fromCell,
        const geometry::GridCell toCell,
        const gfx::Color color)
    {
        for (const auto pixel : tile::getLinePixels(fromCell, toCell))
        {
            paintCharacter(sheetBitmap, direction, frame, pixel, color);
        }
    }

    void paintCharacterFill(
        gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell pixelCell,
        const gfx::Color color)
    {
        const auto was =
            getCharacterPixelColor(sheetBitmap, direction, frame, pixelCell);

        std::set<std::pair<std::uint32_t, std::uint32_t>> seenCells;
        std::deque<geometry::GridCell> goingCells{pixelCell};

        seenCells.insert({pixelCell.column, pixelCell.row});

        while (!goingCells.empty())
        {
            const auto hereCell = goingCells.front();

            goingCells.pop_front();
            paintCharacter(sheetBitmap, direction, frame, hereCell, color);

            for (const auto step : geometry::kFourNeighbourSteps)
            {
                const auto steppedCell = geometry::steppedFrom(
                    kCharacterCellSize, hereCell, step);

                if (!steppedCell.has_value())
                {
                    continue;
                }

                const auto nextCell = *steppedCell;

                if (seenCells.contains({nextCell.column, nextCell.row})
                    || getCharacterPixelColor(
                           sheetBitmap, direction, frame, nextCell)
                           != was)
                {
                    continue;
                }

                seenCells.insert({nextCell.column, nextCell.row});
                goingCells.push_back(nextCell);
            }
        }
    }

        std::optional<geometry::GridCell> characterPixelAt(
        const gfx::RectF whereRect, const gfx::PointF point)
    {
        if (whereRect.size.width <= 0.0F || whereRect.size.height <= 0.0F)
        {
            return std::nullopt;
        }

        const auto acrossFraction = (point.x - whereRect.originPoint.x)
                            / whereRect.size.width
                            * static_cast<float>(
                                kCharacterCellSize.width);
        const auto downFraction = (point.y - whereRect.originPoint.y)
                          / whereRect.size.height
                          * static_cast<float>(
                              kCharacterCellSize.height);

        if (acrossFraction < 0.0F || downFraction < 0.0F
            || acrossFraction >= static_cast<float>(kCharacterCellSize.width)
            || downFraction >= static_cast<float>(kCharacterCellSize.height))
        {
            return std::nullopt;
        }

        return geometry::GridCell{
            .column = static_cast<std::uint32_t>(acrossFraction),
            .row = static_cast<std::uint32_t>(downFraction)};
    }

    gfx::RectF getCharacterPixelPlace(
        const gfx::RectF whereRect, const geometry::GridCell pixelCell)
    {
        const auto width =
            whereRect.size.width
            / static_cast<float>(kCharacterCellSize.width);
        const auto height =
            whereRect.size.height
            / static_cast<float>(kCharacterCellSize.height);
        const auto clear = std::min(
            kPixelClear, std::min(width, height) / 2.0F);

        return gfx::RectF(
            gfx::PointF{
                whereRect.originPoint.x
                    + (static_cast<float>(pixelCell.column) * width)
                    + (clear / 2.0F),
                whereRect.originPoint.y
                    + (static_cast<float>(pixelCell.row) * height)
                    + (clear / 2.0F)},
            gfx::SizeF{width - clear, height - clear});
    }

    gfx::Color getCharacterPixelColor(
        const gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell pixelCell)
    {
        const auto sheetCell = getCharacterPixel(direction, frame, pixelCell);

        return gfx::colorAt(
                   sheetBitmap,
                   static_cast<std::int32_t>(sheetCell.column),
                   static_cast<std::int32_t>(sheetCell.row))
            .value_or(gfx::Color{.alpha = 0});
    }

}
