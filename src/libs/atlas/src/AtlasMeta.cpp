#include "antwika/atlas/AtlasMeta.hpp"

#include <cstdint>

namespace antwika::atlas
{

    namespace
    {
        [[nodiscard]] std::uint32_t slotsAcross(
            const std::uint32_t span, const std::uint32_t slot) noexcept
        {
            return slot > 0 ? span / slot : 0;
        }
    }

    Size sheetSizeOf(const AtlasMeta &meta) noexcept
    {
        return Size{
            .width = meta.columns * meta.sprite.width,
            .height = meta.rows * meta.sprite.height};
    }

    AtlasMeta counted(AtlasMeta meta, const Size sheet) noexcept
    {
        meta.columns = slotsAcross(sheet.width, meta.sprite.width);
        meta.rows = slotsAcross(sheet.height, meta.sprite.height);

        return meta;
    }

}
