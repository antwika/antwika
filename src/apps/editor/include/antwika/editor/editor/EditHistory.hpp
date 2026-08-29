#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/map/MapFile.hpp>

#include "antwika/editor/editor/History.hpp"

namespace antwika::editor
{

    inline constexpr std::size_t kSnapshotAtlasCount = 2;

    struct Snapshot final
    {
        map::Map map{};

        std::array<gfx::Bitmap, kSnapshotAtlasCount> pixelBitmaps{};

        std::vector<gfx::Bitmap> characterBitmaps{};
    };

    using EditHistory = History<Snapshot>;

}
