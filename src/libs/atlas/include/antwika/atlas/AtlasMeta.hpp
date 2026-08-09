#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::atlas
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    enum class AtlasKind : std::uint8_t
    {
        Isometric = 0,

        Flat,
    };

    [[nodiscard]] constexpr AtlasKind enumBound(AtlasKind) noexcept
    {
        return AtlasKind::Flat;
    }

    inline constexpr std::size_t kAtlasKindCount =
        antwika::enums::kCount<AtlasKind>;

    struct AtlasMeta final
    {
        AtlasKind kind = AtlasKind::Isometric;

        std::uint32_t columns = 0;

        std::uint32_t rows = 0;

        Size sprite{};

        Point pivot{};

        Size isometric{};

        [[nodiscard]] bool operator==(const AtlasMeta &other) const =
            default;
    };

    /**
     * @brief Sizes the sheet an atlas asks for.
     *
     * @param meta The atlas to read.
     * @return The sheet its columns and rows of slots fill.
     */
    [[nodiscard]] Size sheetSizeOf(const AtlasMeta &meta) noexcept;

    /**
     * @brief Counts the slots a sheet holds.
     *
     * @param meta The atlas whose slot size does the counting.
     * @param sheet The sheet to count over.
     * @return The atlas, with its columns and rows taken from the
     *         sheet rather than from what it carried.
     */
    [[nodiscard]] AtlasMeta counted(AtlasMeta meta, Size sheet) noexcept;

}
