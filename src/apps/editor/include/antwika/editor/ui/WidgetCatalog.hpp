#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    class Editor;

}

namespace antwika::editor::widget_catalog
{

    struct Catalog final
    {
        enum class Delegate : std::uint8_t
        {
            Activation,
            Slider,
            InkPanel,
        };

        struct SoloRow final
        {
            widget::WidgetId widget = widget::kNoWidget;

            std::string_view hint = {};

            bool toolPanelMembership = false;

            Delegate delegate = Delegate::Activation;

            bool (*activation)(Editor &) = nullptr;
        };

        struct FamilyRow final
        {
            widget::WidgetId (*widgetAt)(std::size_t) = nullptr;

            std::size_t firstPlace = 0;

            std::size_t placeCount = 0;

            std::size_t (*placeCountOf)(const Editor &) = nullptr;

            std::string_view hint = {};

            std::string_view (*hintAt)(std::size_t) = nullptr;

            bool toolPanelMembership = false;

            Delegate delegate = Delegate::Activation;

            bool (*activation)(Editor &, std::size_t) = nullptr;
        };

        struct SliderRow final
        {
            widget::WidgetId widget = widget::kNoWidget;

            bool undoNeed = true;

            bool decorNeed = false;

            bool (*slideGate)(const Editor &) = nullptr;

            std::uint32_t (*valueOf)(const Editor &) = nullptr;

            void (*slideEffect)(Editor &, std::uint32_t) = nullptr;

            void (*settleEffect)(Editor &) = nullptr;
        };

        struct FieldRow final
        {
            widget::WidgetId widget = widget::kNoWidget;

            void (*editEffect)(Editor &, const std::string &) = nullptr;
        };

        struct FieldFamilyRow final
        {
            widget::WidgetId (*widgetAt)(std::size_t) = nullptr;

            std::size_t placeCount = 0;

            void (*editEffect)(
                Editor &, std::size_t, const std::string &) = nullptr;
        };

        std::span<const SoloRow> soloRows;

        std::span<const FamilyRow> familyRows;

        std::span<const SliderRow> sliderRows;

        std::span<const FieldRow> fieldRows;

        std::span<const FieldFamilyRow> fieldFamilies;
    };

    /**
     * @brief The place just past the family's last, counted from
     * firstPlace. placeCountOf, when a family carries one, wins over
     * placeCount.
     */
    [[nodiscard]] inline std::size_t placeEndIn(
        const Catalog::FamilyRow &family, const Editor &editor)
    {
        return family.firstPlace
               + (family.placeCountOf != nullptr
                      ? family.placeCountOf(editor)
                      : family.placeCount);
    }

    /**
     * @brief hintAt, when a family carries one, wins over the shared hint.
     */
    [[nodiscard]] inline std::string_view hintIn(
        const Catalog::FamilyRow &family, const std::size_t place)
    {
        return family.hintAt != nullptr ? family.hintAt(place)
                                        : family.hint;
    }

    /**
     * @brief Every field family names both its widgets and its edit
     * effect.
     */
    template <std::size_t FamilyCount>
    [[nodiscard]] constexpr bool isEveryFieldFamilyClaimed(
        const std::array<Catalog::FieldFamilyRow, FamilyCount>
            &fieldFamilies) noexcept
    {
        for (const auto &family : fieldFamilies)
        {
            if (family.widgetAt == nullptr || family.editEffect == nullptr)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief No field family widget, over the family's whole domain,
     * stands in another family's domain or on a solo field row.
     */
    template <std::size_t FamilyCount, std::size_t FieldCount>
    [[nodiscard]] constexpr bool isEveryFieldFamilyApart(
        const std::array<Catalog::FieldFamilyRow, FamilyCount>
            &fieldFamilies,
        const std::array<Catalog::FieldRow, FieldCount>
            &fieldRows) noexcept
    {
        for (std::size_t familyAt = 0; familyAt < FamilyCount; ++familyAt)
        {
            const auto &ownFamily = fieldFamilies[familyAt];

            for (std::size_t ownPlace = 0; ownPlace < ownFamily.placeCount;
                 ++ownPlace)
            {
                const auto ownWidget = ownFamily.widgetAt(ownPlace);

                for (const auto &fieldRow : fieldRows)
                {
                    if (ownWidget == fieldRow.widget)
                    {
                        return false;
                    }
                }

                for (std::size_t otherAt = familyAt; otherAt < FamilyCount;
                     ++otherAt)
                {
                    const auto &otherFamily = fieldFamilies[otherAt];

                    for (auto otherPlace =
                             otherAt == familyAt ? ownPlace + 1
                                                 : std::size_t{0};
                         otherPlace < otherFamily.placeCount;
                         ++otherPlace)
                    {
                        if (ownWidget == otherFamily.widgetAt(otherPlace))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    /**
     * @brief Hands a field edit to the family place whose widget it
     * names. Only the first matching place is told, since one frame
     * carries at most one edit.
     */
    inline void carryFamilyEdit(
        const Catalog &catalog,
        Editor &editor,
        const widget::WidgetId fieldWidget,
        const std::string &text)
    {
        for (const auto &family : catalog.fieldFamilies)
        {
            for (std::size_t place = 0; place < family.placeCount; ++place)
            {
                if (fieldWidget == family.widgetAt(place))
                {
                    family.editEffect(editor, place, text);

                    return;
                }
            }
        }
    }

    [[nodiscard]] inline bool isOnToolPanel(
        const Catalog &catalog,
        const Editor &editor,
        const widget::WidgetId whichWidget)
    {
        for (const auto &row : catalog.soloRows)
        {
            if (row.toolPanelMembership && whichWidget == row.widget)
            {
                return true;
            }
        }

        for (const auto &family : catalog.familyRows)
        {
            if (!family.toolPanelMembership)
            {
                continue;
            }

            const auto placeEnd = placeEndIn(family, editor);

            for (auto place = family.firstPlace; place < placeEnd;
                 ++place)
            {
                if (whichWidget == family.widgetAt(place))
                {
                    return true;
                }
            }
        }

        return false;
    }

}
