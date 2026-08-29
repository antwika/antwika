#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/editor/state/CharacterTool.hpp"
#include "antwika/editor/editor/state/GrowSetup.hpp"
#include "antwika/editor/editor/state/OverlayCache.hpp"
#include "antwika/editor/editor/state/StampTool.hpp"
#include "antwika/editor/editor/state/WorldEdit.hpp"
#include "antwika/editor/editor/state/WorldPaint.hpp"
#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class WorldView final : public IEditorView
    {
    public:
        [[nodiscard]] bool claims(
            View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        [[nodiscard]] bool offersPaint(
            Paint paint) const noexcept override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        void trackPointer(const ViewContext &viewContext) override;

        [[nodiscard]] std::vector<voxel::VoxelPosition> getStampGhost(
            const ViewContext &viewContext,
            voxel::VoxelPosition positionCell) const;

        [[nodiscard]] bool isUpperSightOn(
            const ViewContext &viewContext) const;

        bool beginShape(
            const ViewContext &viewContext,
            voxel::VoxelPosition position,
            input::MouseButton button);

        void finishShape(
            const ViewContext &viewContext, input::MouseButton button);

        void pressStamp(
            const ViewContext &viewContext,
            voxel::VoxelPosition position,
            input::MouseButton button);

        void finishStamp(
            const ViewContext &viewContext, input::MouseButton button);

        bool beginLampCarry(
            const ViewContext &viewContext, voxel::VoxelPosition position);

        void beginPaintDrag(
            voxel::VoxelPosition position,
            input::MouseButton button) noexcept;

        void endPaintDrag(input::MouseButton button) noexcept;

        void endDrags() noexcept;

        void markOverlaysStale() noexcept;

        [[nodiscard]] std::uint64_t takeGrowSeed() noexcept;

        void setGrowTrouble(
            std::vector<voxel::VoxelPosition> troublePositions);

        void clearGrowTrouble() noexcept;

        [[nodiscard]] WorldEdit &worldEdit() noexcept;

        [[nodiscard]] CharacterTool &characterTool() noexcept;

        [[nodiscard]] const CharacterTool &getCharacterTool() const noexcept;

    private:
        void carryLamp(const ViewContext &viewContext);

        WorldEdit worldEditState;

        CharacterTool characterToolState;

        OverlayCache overlays;

        GrowSetup grow;

        StampTool stamp;

        WorldPaint worldPaint;

        void drawWorldOverlays(
            const ViewContext &viewContext,
            const ui::Frame &frame,
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix);

        void drawHealthBars(
            const ViewContext &viewContext,
            const gfx::Mat4 &clipMatrix);
    };

}
