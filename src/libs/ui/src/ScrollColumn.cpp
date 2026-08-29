#include <algorithm>
#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/ScrollSpec.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "Saturate.hpp"
#include "ScrollPane.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::getClampToU32;
        using detail::Node;
        using detail::ScrollPane;

        constexpr std::uint32_t kWheelLines = 3;

        [[nodiscard]] std::uint32_t wheelStepOf(const Theme &theme) noexcept
        {
            return std::max(
                1U,
                getClampToU32(
                    std::uint64_t{antwika::gfx::glyphLineHeightOf(theme.face)}
                    * theme.textScale * kWheelLines));
        }
    }

    ContainerScope Context::scrollColumn(const ScrollSpec &spec)
    {
        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = spec.heightSizing,
            .gap = 0,
            .widgetId = spec.widgetId});

        const auto viewport = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .widthSizing = kGrowSizing,
            .heightSizing = kGrowSizing,
            .gap = themeValue.gap,
            .clips = true,
            .scrollOffset = getClampToU32(spec.offset)});

        openScrolls.push_back(
            OpenScroll{.viewport = viewport, .spec = spec});

        return ContainerScope{*this};
    }

    void Context::finishScrollColumn()
    {
        const auto pending = openScrolls.back();

        openScrolls.pop_back();

        tree->close();

        const auto track = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .widthSizing = getFixedSize(themeValue.scrollbarWidth),
            .heightSizing = kGrowSizing,
            .gap = 0,
            .backgroundColor = themeValue.scrollTrackColor});

        const auto thumb = tree->add(Node{ // GCOVR_EXCL_LINE
            .widthSizing = kGrowSizing,
            .heightSizing = getFixedSize(0),
            .backgroundColor = themeValue.scrollThumbColor});

        tree->close();
        tree->close();

        tree->addPane(ScrollPane{
            .widgetId = pending.spec.widgetId,
            .viewport = pending.viewport,
            .track = track,
            .thumb = thumb,
            .requestedOffset = pending.spec.offset,
            .step = wheelStepOf(themeValue),
            .dragging = pending.spec.dragging});
    }

}
