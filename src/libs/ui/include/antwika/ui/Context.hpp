#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonSpec.hpp"
#include "antwika/ui/CheckboxSpec.hpp"
#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Icon.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/ContainerScope.hpp"
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
            Size canvasSize,
            Theme theme,
            Pointer pointer = {},
            Keyboard keyboard = {},
            WidgetId focusWidget = kNoWidget);

        ~Context();

        Context(const Context &) = delete;
        Context(Context &&) = delete;

        Context &operator=(const Context &) = delete;
        Context &operator=(Context &&) = delete;

        [[nodiscard]] const Theme &theme() const noexcept;

        void setTheme(Theme theme) noexcept;

        [[nodiscard]] ContainerScope row(ContainerSpec spec = {});

        [[nodiscard]] ContainerScope column(ContainerSpec spec = {});

        [[nodiscard]] ContainerScope panel(ContainerSpec spec = {});

        [[nodiscard]] ContainerScope split(const SplitSpec &spec);

        void label(std::string_view text);

        void label(std::string_view text, Color color);

        void button(std::string_view text, ButtonSpec spec = {});

        void image(Icon shownIcon, Color tintColor);

        void checkbox(bool checked);

        void checkbox(std::string_view text, CheckboxSpec spec = {});

        void iconButton(Icon shownIcon, ButtonSpec spec = {});

        void textField(const TextFieldSpec &spec);

        void textArea(const TextAreaSpec &spec);

        void dropdown(const DropdownSpec &spec);

        void slider(const SliderSpec &spec);

        void spacer(Sizing alongSizing);

        [[nodiscard]] Frame build();

    private:
        friend class ContainerScope;

        void closeContainer() noexcept;

        ContainerScope openContainer(Axis axis, const ContainerSpec &spec);

        Size canvasSize;
        Theme themeValue;
        Pointer pointerValue;
        Keyboard keyboardValue;
        WidgetId focusedWidget;

        std::optional<TextEdit> pendingEdit{};

        std::unique_ptr<detail::LayoutTree> tree;
    };

}
