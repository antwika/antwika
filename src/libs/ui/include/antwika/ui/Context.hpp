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
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Scope.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

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
     * arguments, so the same declarations, canvas, pointer, keyboard and
     * focus always produce the same picture and the same interactions.
     *
     * Focus is the one thing a keyboard UI needs that outlives a frame,
     * and it is passed through rather than kept: it goes in here and
     * comes back out as Frame::interactions.focused, so what remembers
     * it is application state a replay already regenerates. See
     * Interactions::focused.
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
         * @param keyboard The key edges and characters this frame, in
         * arrival order. Left out, this frame has no keyboard, focus
         * stays where the caller had it, nothing can be activated by a
         * keystroke and no field reports an edit.
         * @param focus The widget focused going in, which is the
         * focused id the previous frame handed back. Left out, this
         * frame starts with nothing focused, which is where Tab starts
         * from.
         */
        Context(
            Size canvas,
            Theme theme,
            Pointer pointer = {},
            Keyboard keyboard = {},
            WidgetId focus = kNoWidget);

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
         * A named button is also what Tab stops at, in the order the
         * buttons were declared, and draws the theme's border while it
         * is the focused one.
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
         * @brief Lay the frame out, resolve the pointer and the keyboard
         * against it, and produce the picture it describes.
         *
         * The pointer is resolved against this frame's layout, so what a
         * press hit is what the same call is about to draw.
         *
         * The returned focus is this frame's answer, and the caller's to
         * keep and hand back next frame.
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
         * Everything decided here is decided from the arguments this
         * Context was built with, which is recorded input, so a replay
         * reproduces all of it. What a free-moving pointer is over is
         * deliberately not among them: that is applyHover()'s business,
         * it runs after this, and it touches nothing but the picture.
         * See Frame::hoverTargets.
         *
         * @return The drawing commands, what the pointer and the
         * keyboard did, where each named widget went, and which of them
         * a hover pass may recolour.
         */
        [[nodiscard]] Frame finish();

    private:
        friend class Scope;

        void closeContainer() noexcept;

        Scope openContainer(Axis axis, const ContainerSpec &spec);

        Size canvasSize;
        Theme themeValue;
        Pointer pointerValue;
        Keyboard keyboardValue;
        WidgetId focusValue;

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
