#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    bool OptionsState::open() const noexcept
    {
        return showing;
    }

    void OptionsState::setOpen(bool showing) noexcept
    {
        this->showing = showing;
        pending.reset();
        last.reset();
    }

    const KeyBindings &OptionsState::bindings() const noexcept
    {
        return current;
    }

    std::optional<Action> OptionsState::awaiting() const noexcept
    {
        return pending;
    }

    void OptionsState::await(Action action) noexcept
    {
        pending = action;
        last.reset();
    }

    std::optional<BindOutcome> OptionsState::notice() const noexcept
    {
        return last;
    }

    std::optional<BindOutcome> OptionsState::press(Key key) noexcept
    {
        if (!pending.has_value())
        {
            return std::nullopt;
        }

        const auto outcome = current.bind(*pending, key);
        last = outcome;

        // A refusal leaves the question up, so the next key answers it.
        if (outcome == BindOutcome::Bound
            || outcome == BindOutcome::Unchanged)
        {
            pending.reset();
        }

        return outcome;
    }

    BindOutcome OptionsState::apply(Action action, Key key) noexcept
    {
        return current.bind(action, key);
    }

    antwika::i18n::Locale OptionsState::locale() const noexcept
    {
        return language;
    }

    void OptionsState::setLocale(antwika::i18n::Locale locale) noexcept
    {
        language = locale;
    }

    KeyboardLayout OptionsState::keyboard() const noexcept
    {
        return typing;
    }

    void OptionsState::setKeyboard(KeyboardLayout layout) noexcept
    {
        typing = layout;
    }

} // namespace antwika::game
