#pragma once

#include <string>
#include <vector>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/editor/state/FigureTool.hpp"
#include "antwika/editor/editor/state/GrowSetup.hpp"
#include "antwika/editor/editor/state/OverlayCache.hpp"
#include "antwika/editor/editor/state/PlateTool.hpp"
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
            map::View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        [[nodiscard]] bool offersPaint(
            map::Paint paint) const noexcept override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        [[nodiscard]] std::vector<voxel::VoxelPosition> getStampGhost(
            const ViewContext &viewContext,
            voxel::VoxelPosition positionCell) const;

        [[nodiscard]] bool isUpperSightOn(
            const ViewContext &viewContext) const;

        WorldEdit worldEdit;

        OverlayCache overlays;

        StampTool stamp;

        GrowSetup grow;

        FigureTool figureTool;

        PlateTool plateTool;

        WorldPaint worldPaint;

    private:
        void drawWorldOverlays(
            const ViewContext &viewContext,
            const ui::Frame &frame,
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix);

        void drawPointMark(
            const ViewContext &viewContext,
            const gfx::Mat4 &clipMatrix,
            gfx::Vec3 position,
            gfx::Color markColor);

        void drawSightPoints(
            const ViewContext &viewContext,
            const gfx::Mat4 &clipMatrix);

        void drawHealthBars(
            const ViewContext &viewContext,
            const gfx::Mat4 &clipMatrix);
    };

}
