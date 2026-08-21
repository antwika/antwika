#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>

#include <antwika/map/History.hpp>
#include <antwika/map/MapFile.hpp>

namespace antwika::map
{

    inline constexpr std::size_t kSnapshotAtlasCount = 2;

    struct Snapshot final
    {
        Map map{};

        std::array<gfx::Bitmap, kSnapshotAtlasCount> pixelBitmaps{};

        std::vector<gfx::Bitmap> characterBitmaps{};
    };

    using EditHistory = History<Snapshot>;

}
