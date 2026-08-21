#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include <antwika/animation/Clip.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>
#include <antwika/collision/Collision.hpp>

#include <antwika/character/CharacterMarks.hpp>

namespace antwika::character
{

    inline constexpr std::size_t kCharacterWays = 8;

    inline constexpr std::size_t kCharacterFrames = 4;

    inline constexpr gfx::Size kCharacterCellSize{
        .width = 20, .height = 28};

    inline constexpr time::Tick kCharacterPaceTick = 6;

    inline constexpr float kCharacterTall =
        static_cast<float>(kCharacterCellSize.height) * collision::kWalkerPixel;

    inline constexpr float kCharacterWide =
        static_cast<float>(kCharacterCellSize.width) * collision::kWalkerPixel;

    inline constexpr float kCutawayMargin = 0.5F * voxel::kVoxelSide;

    inline constexpr float kSpriteLift = 0.5F * voxel::kVoxelSide;

    [[nodiscard]] gfx::Vec3 headTopOf(component::Position stoodPosition);

    inline constexpr float kSpriteGroundSkew = 3.0F / 4.0F;

    inline constexpr float kSpriteUprightSkew = 4.0F / 3.0F;

    inline constexpr float kSpriteDepthBias = collision::kWalkerPixel / 4.0F;

    inline constexpr std::string_view kCharacterSheet =
        "character-20x28.png";

    [[nodiscard]] gfx::Size characterSheetSize();

    [[nodiscard]] gfx::Bitmap blankCharacter();

    [[nodiscard]] std::string_view directionName(std::size_t direction);

    [[nodiscard]] gfx::Rect characterCell(
        std::size_t direction, std::size_t frame);

    [[nodiscard]] gfx::RectF characterSource(
        std::size_t direction, std::size_t frame);

    [[nodiscard]] std::optional<std::size_t> facingFromVelocity(
        component::Velocity velocity);

    [[nodiscard]] animation::Clip walkingClip(std::size_t direction);

    [[nodiscard]] animation::Clip standingClip(std::size_t direction);

    [[nodiscard]] std::size_t currentFrame(
        component::AnimationState posedState, time::Tick tick);

    inline constexpr std::size_t kTransparentInk = 0;

    [[nodiscard]] gfx::Color characterPaletteColor(
        std::span<const gfx::Color> paletteColors, std::size_t which);

    [[nodiscard]] gfx::MeshData characterMesh();

    [[nodiscard]] gfx::Mat4 spriteBillboardMatrix(
        gfx::Vec3 position, const gfx::Mat4 &viewMatrix);

    [[nodiscard]] gfx::Vec3 frameUvOffset(
        std::size_t direction, std::size_t frame);

    [[nodiscard]] gfx::Vec3 frameUvSize();

    [[nodiscard]] geometry::GridCell characterPixel(
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell pixelCell);

    void paintCharacter(
        gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell pixelCell,
        gfx::Color color);

    void paintCharacterLine(
        gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell fromCell,
        geometry::GridCell toCell,
        gfx::Color color);

    void paintCharacterFill(
        gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell pixelCell,
        gfx::Color color);

    [[nodiscard]] std::optional<geometry::GridCell> characterPixelAt(
        gfx::RectF whereRect, gfx::PointF point);

    [[nodiscard]] gfx::RectF characterPixelPlace(
        gfx::RectF whereRect, geometry::GridCell pixelCell);

    [[nodiscard]] gfx::Color characterPixelColor(
        const gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell pixelCell);

}
