#pragma once

#include <cstddef>
#include <optional>
#include <antwika/character/CharacterMarks.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/render/CharacterSkins.hpp>

namespace antwika::editor
{

    struct SheetMark final
    {
        bool selecting = false;

        bool draggingPatch = false;

        std::optional<character::PixelSelection> selection;

        std::optional<character::PixelSelection> grabbedMarkSelection;

        std::optional<geometry::GridCell> grabbedAtCell;

        std::optional<character::PixelBuffer> floatingPatchBuffer;

        character::PixelBuffer clipboardBuffer;

        std::optional<std::size_t> hoveredWayRow;

        std::optional<std::size_t> selectedFrame = 0;
    };

}
