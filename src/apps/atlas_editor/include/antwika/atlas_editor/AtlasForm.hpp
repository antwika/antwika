#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    enum class AtlasField : std::uint8_t
    {
        SpriteWidth = 0,

        SpriteHeight,

        Columns,

        Rows,

        PivotX,

        PivotY,

        IsometricWidth,

        IsometricHeight,
    };

    [[nodiscard]] constexpr AtlasField enumBound(AtlasField) noexcept
    {
        return AtlasField::IsometricHeight;
    }

    inline constexpr std::size_t kAtlasFieldCount =
        antwika::enums::kCount<AtlasField>;

    struct AtlasForm final
    {
        AtlasKind kind = AtlasKind::Isometric;

        std::array<std::string, kAtlasFieldCount> values{};

        [[nodiscard]] bool operator==(const AtlasForm &other) const =
            default;
    };

    inline constexpr std::size_t kAtlasPresetCount = 4;

    [[nodiscard]] MessageId fieldNameId(AtlasField field) noexcept;

    [[nodiscard]] MessageId presetNameId(std::size_t preset) noexcept;

    /**
     * @brief Fills a form in from one of the shipped atlas shapes.
     *
     * @param preset The shape to take, counted from zero.
     * @return A form describing an isometric sheet whose slots hold a
     *         block that many cells on a side.
     */
    [[nodiscard]] AtlasForm presetForm(std::size_t preset);

    [[nodiscard]] AtlasForm formOf(const AtlasMeta &meta);

    /**
     * @brief Reads back the atlas a filled-in form describes.
     *
     * @param form The form to read.
     * @return The metadata, counting any field that is not a whole
     *         number as zero.
     */
    [[nodiscard]] AtlasMeta metaOf(const AtlasForm &form) noexcept;

    /**
     * @brief Says whether a form describes a sheet worth making.
     *
     * @param form The form to read.
     * @return True where every slot and count it names has extent.
     */
    [[nodiscard]] bool formIsWhole(const AtlasForm &form) noexcept;

}
