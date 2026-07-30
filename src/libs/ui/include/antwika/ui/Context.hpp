#pragma once

#include <memory>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/DrawList.hpp"
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
     * arguments, so the same declarations and the same canvas always
     * produce the same picture.
     */
    class Context final
    {
    public:
        /**
         * @brief Begin one frame's UI.
         * @param canvas The area the UI is laid out into.
         * @param theme The colours and metrics widgets draw from.
         */
        Context(Size canvas, Theme theme);

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
         * @param text The button's label.
         * @param state How it should look; the caller decides.
         * @param width How wide, defaulting to fitting its label.
         */
        void button(
            std::string_view text,
            ButtonState state = ButtonState::Idle,
            Sizing width = kFit);

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
         * @brief Lay the frame out and produce the picture it describes.
         *
         * Asking twice gives the same answer: nothing here is consumed.
         *
         * @return The drawing commands, in the order they are drawn.
         */
        [[nodiscard]] DrawList finish();

    private:
        friend class Scope;

        void closeContainer() noexcept;

        Scope openContainer(Axis axis, const ContainerSpec &spec);

        Size canvasSize;
        Theme themeValue;
        std::unique_ptr<detail::LayoutTree> tree;
    };

} // namespace antwika::ui
