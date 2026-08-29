#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Bitmap.hpp>

namespace antwika::editor
{

    enum class GizmoKind : std::uint8_t
    {
        Spawn,
        Exit,
        Food,
        Water,
        Lamp,
        Checkpoint,
    };

    [[nodiscard]] constexpr GizmoKind getLastEnumerator(GizmoKind) noexcept
    {
        return GizmoKind::Checkpoint;
    }

    inline constexpr std::string_view kGizmoSheet = "gizmos-16.png";

    inline constexpr std::array<std::string_view, enums::kCount<GizmoKind>>
        kGizmoNames{
            "spawn", "exit", "food", "water", "lamp", "checkpoint"};

    [[nodiscard]] gfx::Bitmap getBlankGizmoSheet();

    [[nodiscard]] gfx::Bitmap getLoadGizmoSheet(
        const std::string &mapPath, std::string_view app);

    [[nodiscard]] bool isGizmoDrawn(
        const gfx::Bitmap &sheetBitmap, std::size_t gizmoIndex);

    void wipeGizmo(gfx::Bitmap &sheetBitmap, std::size_t gizmoIndex);

}
