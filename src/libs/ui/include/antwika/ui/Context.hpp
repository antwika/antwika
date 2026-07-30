#pragma once

#include <memory>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonSpec.hpp"
#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Scope.hpp"
#include "antwika/ui/Sizing.hpp"
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
         */
        Context(Size canvas, Theme theme, Pointer pointer = {});

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
        std::unique_ptr<detail::LayoutTree> tree;
    };

} // namespace antwika::ui
