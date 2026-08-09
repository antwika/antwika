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
#include "antwika/ui/SliderSpec.hpp"
#include "antwika/ui/SplitSpec.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    namespace detail
    {
        class LayoutTree;
    }

    class Context final
    {
    public:
        Context(
            Size canvas,
            Theme theme,
            Pointer pointer = {},
            Keyboard keyboard = {},
            WidgetId focus = kNoWidget);

        ~Context();

        Context(const Context &) = delete;
        Context(Context &&) = delete;

        Context &operator=(const Context &) = delete;
        Context &operator=(Context &&) = delete;

        [[nodiscard]] const Theme &theme() const noexcept;

        [[nodiscard]] Scope row(ContainerSpec spec = {});

        [[nodiscard]] Scope column(ContainerSpec spec = {});

        [[nodiscard]] Scope panel(ContainerSpec spec = {});

        /**
         * @brief Opens a pair of panes with a draggable divider between.
         *
         * @param spec Which way the pair splits, where the divider
         *             sits, and what each pane keeps.
         * @return The scope the two panes are added inside.
         *
         * Requires: exactly two children are added before the scope
         *           closes, or finish() throws.
         * Ensures:  neither pane is thinner than spec.minimum unless
         *           the pair has no room for two of them.
         */
        [[nodiscard]] Scope split(const SplitSpec &spec);

        void label(std::string_view text);

        void label(std::string_view text, Color color);

        void button(std::string_view text, ButtonSpec spec = {});

        void textField(const TextFieldSpec &spec);

        void textArea(const TextAreaSpec &spec);

        void dropdown(const DropdownSpec &spec);

        void slider(const SliderSpec &spec);

        void spacer(Sizing along);

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

        std::optional<TextEdit> pendingEdit{};

        std::unique_ptr<detail::LayoutTree> tree;
    };

}
