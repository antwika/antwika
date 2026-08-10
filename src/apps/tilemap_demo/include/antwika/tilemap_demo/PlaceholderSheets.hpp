#pragma once

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::tilemap_demo
{

    [[nodiscard]] gfx::Bitmap placeholderSheet(
        tilemap::TerrainClass terrain, gfx::Color ink);

}
