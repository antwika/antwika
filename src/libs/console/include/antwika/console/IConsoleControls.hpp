#pragma once

#include <antwika/input/Key.hpp>

#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console
{

    /**
     * @brief What the console's keys are, and what they type by.
     *
     * A seam rather than three constants, because an application with
     * rebindable keys answers off its own simulation state -- the
     * game's options screen rebinds the toggle mid-run -- while one
     * without simply fixes them.
     * Everything answered here must be simulation state or a constant,
     * never something read off the machine inside the tick path.
     */
    class IConsoleControls
    {
    public:
        virtual ~IConsoleControls() = default;

        /**
         * @brief Get the key that slides the console in and out.
         * @return The toggle key.
         */
        [[nodiscard]] virtual antwika::input::Key toggleKey() const = 0;

        /**
         * @brief Get the key that executes the field.
         * @return The execute key.
         */
        [[nodiscard]] virtual antwika::input::Key
        executeKey() const = 0;

        /**
         * @brief Get the board typed characters are read off.
         * @return The layout every typed character goes through.
         */
        [[nodiscard]] virtual KeyboardLayout keyboard() const = 0;
    };

    /**
     * @brief The controls an application without options ships:
     * constants, chosen at construction.
     */
    class FixedConsoleControls final : public IConsoleControls
    {
    public:
        /**
         * @brief Construct the controls every plain application uses.
         * @param toggle The key that slides the console; Grave, until
         * somebody says otherwise, because every console ever has.
         * @param execute The key that executes the field.
         * @param layout The board typed characters are read off.
         */
        explicit FixedConsoleControls(
            antwika::input::Key toggle = antwika::input::Key::Grave,
            antwika::input::Key execute = antwika::input::Key::Enter,
            KeyboardLayout layout = kDefaultKeyboardLayout) noexcept
            : toggle(toggle), execute(execute), layout(layout)
        {
        }

        [[nodiscard]] antwika::input::Key toggleKey() const override
        {
            return toggle;
        }

        [[nodiscard]] antwika::input::Key executeKey() const override
        {
            return execute;
        }

        [[nodiscard]] KeyboardLayout keyboard() const override
        {
            return layout;
        }

    private:
        antwika::input::Key toggle;
        antwika::input::Key execute;
        KeyboardLayout layout;
    };

} // namespace antwika::console
