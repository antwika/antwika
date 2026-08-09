#pragma once

#include <optional>

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    class OptionsState final
    {
    public:
        OptionsState() noexcept = default;

        OptionsState(const OptionsState &) = delete;
        OptionsState(OptionsState &&) = delete;

        OptionsState &operator=(const OptionsState &) = delete;
        OptionsState &operator=(OptionsState &&) = delete;

        [[nodiscard]] bool open() const noexcept;

        void setOpen(bool showing) noexcept;

        [[nodiscard]] const KeyBindings &bindings() const noexcept;

        [[nodiscard]] std::optional<Action> awaiting() const noexcept;

        void await(Action action) noexcept;

        [[nodiscard]] std::optional<BindOutcome> notice() const noexcept;

        std::optional<BindOutcome> press(Key key) noexcept;

        BindOutcome apply(Action action, Key key) noexcept;

        [[nodiscard]] antwika::i18n::Locale locale() const noexcept;

        void setLocale(antwika::i18n::Locale locale) noexcept;

        [[nodiscard]] KeyboardLayout keyboard() const noexcept;

        void setKeyboard(KeyboardLayout layout) noexcept;

    private:
        KeyBindings current{};
        std::optional<Action> pending{};
        std::optional<BindOutcome> last{};
        bool showing = false;
        antwika::i18n::Locale language{antwika::i18n::kDefaultLocale};
        KeyboardLayout typing{kDefaultKeyboardLayout};
    };

}
