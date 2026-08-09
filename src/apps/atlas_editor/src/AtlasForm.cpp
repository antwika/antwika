#include "antwika/atlas_editor/AtlasForm.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        constexpr std::uint64_t kFieldLimit = 1U << 16U;

        constexpr std::array<MessageId, kAtlasFieldCount> kFieldNames{
            MessageId::SpriteWidth,
            MessageId::SpriteHeight,
            MessageId::Columns,
            MessageId::Rows,
            MessageId::PivotX,
            MessageId::PivotY,
            MessageId::IsometricWidth,
            MessageId::IsometricHeight};

        constexpr std::uint32_t kPresetColumns = 8;

        constexpr std::uint32_t kPresetRows = 8;

        constexpr std::array<MessageId, kAtlasPresetCount> kPresetNames{
            MessageId::PresetOneByOne,
            MessageId::PresetTwoByTwo,
            MessageId::PresetThreeByThree,
            MessageId::PresetFourByFour};

        [[nodiscard]] TileGrid presetTile(
            const std::size_t preset) noexcept
        {
            const auto cells = static_cast<std::uint32_t>(preset) + 1;

            return TileGrid{
                .width = cells * kIsoTileWidth + kSpriteSideMargin * 2,
                .height = cells * kIsoTileHeight + kSpriteHeadroom
                          + kSpriteSkirtBand};
        }

        [[nodiscard]] std::uint32_t wholeIn(
            const std::string &text) noexcept
        {
            std::uint64_t value = 0;

            if (text.empty())
            {
                return 0;
            }

            for (const char digit : text)
            {
                if (digit < '0' || digit > '9')
                {
                    return 0;
                }

                value = value * 10
                        + static_cast<std::uint64_t>(digit - '0');

                if (value > kFieldLimit)
                {
                    return static_cast<std::uint32_t>(kFieldLimit);
                }
            }

            return static_cast<std::uint32_t>(value);
        }

        [[nodiscard]] std::uint32_t valueOf(
            const AtlasForm &form, const AtlasField field) noexcept
        {
            return wholeIn(form.values[static_cast<std::size_t>(field)]);
        }

        void fill(
            AtlasForm &form,
            const AtlasField field,
            const std::uint32_t value)
        {
            form.values[static_cast<std::size_t>(field)] =
                std::to_string(value);
        }
    }

    MessageId fieldNameId(const AtlasField field) noexcept
    {
        return kFieldNames[static_cast<std::size_t>(field)];
    }

    MessageId presetNameId(const std::size_t preset) noexcept
    {
        return kPresetNames[preset % kAtlasPresetCount];
    }

    AtlasForm presetForm(const std::size_t preset)
    {
        const auto tile = presetTile(preset % kAtlasPresetCount);

        return formOf(metaFor(
            tile,
            Size{
                .width = kPresetColumns * tile.width,
                .height = kPresetRows * tile.height}));
    } // GCOVR_EXCL_LINE

    AtlasForm formOf(const AtlasMeta &meta)
    {
        AtlasForm form;
        form.kind = meta.kind;

        fill(form, AtlasField::SpriteWidth, meta.sprite.width);
        fill(form, AtlasField::SpriteHeight, meta.sprite.height);
        fill(form, AtlasField::Columns, meta.columns);
        fill(form, AtlasField::Rows, meta.rows);
        fill(
            form,
            AtlasField::PivotX,
            static_cast<std::uint32_t>(meta.pivot.x));
        fill(
            form,
            AtlasField::PivotY,
            static_cast<std::uint32_t>(meta.pivot.y));
        fill(form, AtlasField::IsometricWidth, meta.isometric.width);
        fill(form, AtlasField::IsometricHeight, meta.isometric.height);

        return form;
    } // GCOVR_EXCL_LINE

    AtlasMeta metaOf(const AtlasForm &form) noexcept
    {
        AtlasMeta meta;
        meta.kind = form.kind;
        meta.columns = valueOf(form, AtlasField::Columns);
        meta.rows = valueOf(form, AtlasField::Rows);
        meta.sprite = Size{
            .width = valueOf(form, AtlasField::SpriteWidth),
            .height = valueOf(form, AtlasField::SpriteHeight)};
        meta.pivot = Point{
            .x = static_cast<std::int32_t>(
                valueOf(form, AtlasField::PivotX)),
            .y = static_cast<std::int32_t>(
                valueOf(form, AtlasField::PivotY))};
        meta.isometric = Size{
            .width = valueOf(form, AtlasField::IsometricWidth),
            .height = valueOf(form, AtlasField::IsometricHeight)};

        return meta;
    }

    bool formIsWhole(const AtlasForm &form) noexcept
    {
        const auto meta = metaOf(form);

        return meta.columns > 0 && meta.rows > 0 && meta.sprite.width > 0
               && meta.sprite.height > 0;
    }

}
