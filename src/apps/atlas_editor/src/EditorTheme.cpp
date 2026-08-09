#include "antwika/atlas_editor/EditorTheme.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/atlas_editor/Modal.hpp"

namespace antwika::atlas_editor
{

    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        [[nodiscard]] std::uint32_t lineHeight(
            const Theme &theme) noexcept
        {
            return antwika::gfx::kGlyphLineHeight * theme.textScale;
        }

        [[nodiscard]] std::uint32_t rowHeight(const Theme &theme) noexcept
        {
            return lineHeight(theme) + theme.buttonPadding * 2;
        }

        [[nodiscard]] std::uint32_t rowStep(const Theme &theme) noexcept
        {
            return rowHeight(theme) + theme.gap;
        }

        [[nodiscard]] std::uint32_t oneRowHeight(
            const Theme &theme, const std::size_t labels) noexcept
        {
            const auto lines =
                static_cast<std::uint32_t>(labels) * lineHeight(theme);

            return theme.padding * 4
                   + theme.gap * (3 + static_cast<std::uint32_t>(labels))
                   + lines + rowHeight(theme) * 3;
        }
    }

    std::size_t labelsAbove(const Modal modal) noexcept
    {
        return modal == Modal::Save ? kCardLabels + kMetaLines
                                    : kCardLabels;
    }

    Theme editorTheme(const Size canvas) noexcept
    {
        Theme theme = scaledTheme(Theme{}, scaleForCanvas(canvas));
        theme.textScale = std::max(theme.textScale * 3 / 4, 1U);

        return theme;
    }

    std::size_t filesShownIn(
        const Size canvas, const std::size_t labels) noexcept
    {
        const Theme theme = editorTheme(canvas);
        const auto least = oneRowHeight(theme, labels);

        if (canvas.height <= least)
        {
            return 1;
        }

        return 1U + (canvas.height - least) / rowStep(theme);
    }

}
