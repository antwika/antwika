#include "antwika/autotile/SystemSheet.hpp"

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/autotile/Metrics.hpp"

namespace antwika::autotile
{

    namespace
    {
        constexpr std::array<std::int32_t, enums::kCount<DrawKind>>
            kColumns{0, kHalfTile, 0, 2 * kHalfTile, 3 * kHalfTile};
    }

    geometry::Rect systemSource(const DrawKind kind) noexcept
    {
        return {
            .origin = {.x = kColumns[enums::index(kind)], .y = 0},
            .size = {.width = kHalfTile, .height = kHalfTile}};
    }

}
