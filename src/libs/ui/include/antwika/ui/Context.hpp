#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonSpec.hpp"
#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Scope.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/TextInput.hpp"
#include "antwika/ui/Theme.hpp"

namespace antwika::ui
{

    namespace detail
    {
        class LayoutTree;
    } // namespace detail

    /**
     * @brief One frame's UI, declared as it is written.
     *
     * The caller writes widgets in the order they appear, and what that
     * builds is a tree, laid out only once finish() is called.
     * Deferring it is what lets a container size itself from children it
     * has not seen yet, which is what makes nesting work at all.
     *
     * Holds nothing between frames and reads nothing outside its
     * arguments, so the same declarations, canvas and pointer always
     * produce the same picture and the same interactions.
     */
    class Context final
    {
    public:
        /**
         * @brief Begin one frame's UI.
         * @param canvas The area the UI is laid out into.
         * @param theme The colours and metrics widgets draw from.
         * @param pointer Where the pointer is and what it is doing, in
         * the same pixels the canvas is measured in. Left out, this
         * frame has no pointer and nothing can be hovered or activated.
         * @param keys What was typed this frame. Left out, nothing was,
         * and no field reports an edit.
         */
        Context(
            Size canvas,
            Theme theme,
            Pointer pointer = {},
            TextInput keys = {});

        /**
         * @brief Discard the frame.
         */
        ~Context();

        Context(const Context &) = delete;
        Context(Context &&) = delete;

        Context &operator=(const Context &) = delete;
        Context &operator=(Context &&) = delete;

        /**
         * @brief Get the theme this frame draws from.
         * @return The theme, for a caller wanting one of its colours.
         */
        [[nodiscard]] const Theme &theme() const noexcept;

        /**
         * @brief Open a container that lays its children out across.
         * @param spec What the container is being asked for.
         * @return The scope holding it open; keep it in a named variable.
         */
        [[nodiscard]] Scope row(ContainerSpec spec = {});

        /**
         * @brief Open a container that lays its children out downwards.
         * @param spec What the container is being asked for.
         * @return The scope holding it open; keep it in a named variable.
         */
        [[nodiscard]] Scope column(ContainerSpec spec = {});

        /**
         * @brief Open a column carrying the theme's fill and inset.
         *
         * Differs from column() only in what an unset field falls back
         * to, so a caller that wants a bare column asks for one.
         *
         * @param spec What the container is being asked for.
         * @return The scope holding it open; keep it in a named variable.
         */
        [[nodiscard]] Scope panel(ContainerSpec spec = {});

        /**
         * @brief Add a line of text in the theme's text colour.
         * @param text The characters to draw.
         */
        void label(std::string_view text);

        /**
         * @brief Add a line of text in a colour of your own.
         * @param text The characters to draw.
         * @param color The colour to draw them in.
         */
        void label(std::string_view text, Color color);

        /**
         * @brief Add a button: a filled box around a centred label.
         *
         * Named in the spec, it works out its own appearance from the
         * pointer and reports being pressed through finish().
         *
         * @param text The button's label.
         * @param spec What the button is being asked for.
         */
        void button(std::string_view text, ButtonSpec spec = {});

        /**
         * @brief Add a box holding characters somebody typed.
         *
         * **The characters are the caller's, not this library's.** They
         * arrive in the spec and any edit comes back through
         * Interactions::edit, because nothing here is retained between
         * frames: a field that owned what was typed would be state a
         * replay could not regenerate. See TextFieldSpec.
         *
         * Only a focused field draws a caret and reports an edit, so a
         * frame's typing lands in exactly one field however many are
         * declared.
         *
         * @param spec What the field is being asked for.
         */
        void textField(const TextFieldSpec &spec);

        /**
         * @brief Add a box naming one of a list of options.
         *
         * **Whether the list is open is the caller's, not this
         * library's**, for the same reason a field's characters are:
         * see DropdownSpec.
         *
         * An open list is drawn over whatever sits below the box rather
         * than pushing it aside, and is hit before it. Both fall out of
         * the list being an overlay: painted after every other command
         * and hit-tested before them, since antwika::gfx offers no depth
         * of its own.
         *
         * @param spec What the dropdown is being asked for.
         */
        void dropdown(const DropdownSpec &spec);

        /**
         * @brief Add an empty child that takes up room.
         *
         * How leading, trailing and centred content are expressed, rather
         * than by a second alignment rule that could disagree with the
         * space distribution the layout already does.
         *
         * @param along How much room to take along the open container's
         * axis.
         */
        void spacer(Sizing along);

        /**
         * @brief Lay the frame out, resolve the pointer against it, and
         * produce the picture it describes.
         *
         * The pointer is resolved against this frame's layout, so what a
         * press hit is what the same call is about to draw.
         *
         * Asking twice gives the same answer: nothing here is consumed.
         *
         * That ordering is what a caller has to plan around. Activation
         * is decided during the layout, so a frame reports the press
         * alongside a picture drawn from the state the press has not
         * changed yet. The remedy is describe, act, describe again:
         *
         * @code
         * auto frame = describe(canvas, pointer, state);
         * if (frame.interactions.activated == widgets::kZoomIn)
         * {
         *     state.zoomIn();
         *     // The picture above predates that, so it is described
         *     // once more and the second frame is the one drawn.
         *     frame = describe(canvas, pointer, state);
         * }
         * @endcode
         *
         * Describing twice costs one more layout and no retained state,
         * which is the price of activating on the press rather than on
         * a press-then-release match a replay would have to regenerate.
         *
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame finish();

    private:
        friend class Scope;

        void closeContainer() noexcept;

        Scope openContainer(Axis axis, const ContainerSpec &spec);

        Size canvasSize;
        Theme themeValue;
        Pointer pointerValue;
        TextInput keysValue;

        /**
         * @brief What the focused field's typing came to, if anything.
         *
         * Worked out where the field is declared, since that is where
         * both the characters and the keys are known, and handed over by
         * finish(). It needs no layout, unlike everything the pointer
         * decides.
         */
        std::optional<TextEdit> pendingEdit{};

        std::unique_ptr<detail::LayoutTree> tree;
    };

} // namespace antwika::ui
