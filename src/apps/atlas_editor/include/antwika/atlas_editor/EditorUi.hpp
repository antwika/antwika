#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the toolbar's widgets are called.
     *
     * Symbolic names rather than where a widget ended up in the layout,
     * because this is what crosses back into the session's state.
     * None of these ever reaches a replay: what is recorded is the
     * click, and which widget it hit is worked out again from it -- see
     * EditorSink.
     */
    namespace widgets
    {
        /** @brief Draw one image pixel smaller. */
        inline constexpr WidgetId kZoomOut{1};

        /** @brief Draw one image pixel larger. */
        inline constexpr WidgetId kZoomIn{2};

        /** @brief Put the whole sheet back in the middle. */
        inline constexpr WidgetId kResetView{3};

        /** @brief Show or hide the slot grid. */
        inline constexpr WidgetId kGrid{4};

        /** @brief Write the sheet out. */
        inline constexpr WidgetId kSave{5};

        /** @brief Read the sheet back in, losing every unsaved change. */
        inline constexpr WidgetId kLoad{6};

        /**
         * @brief The first tool button, one per Tool.
         *
         * The tools run from here in their declaration order, so a tool
         * and its button cannot drift apart.
         */
        inline constexpr WidgetId kFirstTool{16};

        /**
         * @brief The first palette swatch, one per colour.
         *
         * Far enough above the tools that a palette twice this long
         * still cannot collide with one.
         */
        inline constexpr WidgetId kFirstSwatch{32};

        /**
         * @brief Get which button selects a tool.
         * @param tool The tool to ask about.
         * @return That tool's button.
         */
        [[nodiscard]] constexpr WidgetId toolWidget(const Tool tool) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool)
                + static_cast<std::uint64_t>(tool));
        }

        /**
         * @brief Get which swatch selects a colour.
         * @param index Which colour of the palette.
         * @return That colour's swatch.
         */
        [[nodiscard]] constexpr WidgetId swatchWidget(
            const std::size_t index) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstSwatch) + index);
        }
    } // namespace widgets

    // Two widgets sharing an id would be one widget, silently.
    // The tool and swatch ids are derived rather than written.
    // So this is where the two that could ever meet are checked.
    static_assert(
        antwika::ui::assertDistinct(
            widgets::kZoomOut,
            widgets::kZoomIn,
            widgets::kResetView,
            widgets::kGrid,
            widgets::kSave,
            widgets::kLoad,
            widgets::toolWidget(Tool::Paint),
            widgets::toolWidget(Tool::Erase),
            widgets::toolWidget(Tool::Pick),
            widgets::swatchWidget(0)),
        "every toolbar widget needs its own id");

    /**
     * @brief Write the one line under the toolbar.
     *
     * Split out from the bar so a test can assert what it says without
     * reading it back off a layout.
     *
     * @param state The session to describe.
     * @param translator Words the message the state carries.
     * @return Where the pointer is, what it would do, and how the sheet
     * stands, in one line.
     */
    [[nodiscard]] std::string statusLine(
        const EditorState &state, const Translator &translator);

    /**
     * @brief Describe the toolbar for one tick.
     *
     * A pure function of the state and the pointer, so the same two
     * always produce the same picture and the same answer about what the
     * pointer is on.
     *
     * It is laid out against EditorState::canvas(), which is the size
     * the window was *asked* for and never the size one reports: a
     * hit-test is a function of the layout and the layout is a function
     * of the canvas, so resolving a recorded click against a differently
     * sized window would resolve it to a different button.
     *
     * **The bar is measured from translated text, which is why this
     * application's locale is fixed in main().** A button is as wide as
     * its own label, and a press is resolved against the layout those
     * widths produce, so a session recorded in one language and
     * replayed in another would resolve the same click to a different
     * button. The locale is therefore a constant of the build here,
     * read from no environment variable and no flag, since neither is
     * carried by a recording -- see antwika/i18n/Translator.hpp.
     *
     * @param state The session the bar reports and acts on.
     * @param pointer Where the pointer is and what it is doing.
     * @param translator Words every label on the bar.
     * @return The picture, and what the pointer did to it.
     */
    [[nodiscard]] Frame describeEditor(
        const EditorState &state,
        Pointer pointer,
        const Translator &translator);

} // namespace antwika::atlas_editor
