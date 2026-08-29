#pragma once

#include <cstdint>
#include <optional>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/TextEdit.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    void resolveAreas(
        LayoutTree &tree,
        const Pointer &pointer,
        bool underOverlay,
        Interactions &interactions,
        std::optional<TextEdit> &edit);

    void resolveRails(
        LayoutTree &tree,
        const Pointer &pointer,
        bool underOverlay,
        std::uint32_t thumbWidth,
        Interactions &interactions);

    void resolveBars(
        const LayoutTree &tree,
        const Pointer &pointer,
        bool underOverlay,
        Interactions &interactions);

    void resolveEdges(
        const LayoutTree &tree,
        const Pointer &pointer,
        bool underOverlay,
        Interactions &interactions);

    void resolvePanes(
        LayoutTree &tree,
        const Pointer &pointer,
        bool underOverlay,
        Interactions &interactions);

    void applyVisualState(
        LayoutTree &tree,
        const Interactions &interactions,
        bool down);

}
