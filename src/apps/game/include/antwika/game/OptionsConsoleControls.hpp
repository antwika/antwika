#pragma once

#include <antwika/console/IConsoleControls.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    /**
     * @brief The console's controls, answered off the run's options.
     *
     * This application's keys are rebindable and its typing board is
     * an options-screen choice, so the console's controls read the
     * simulation state those live in rather than constants -- which
     * is the whole of why console::IConsoleControls is a seam.
     */
    class OptionsConsoleControls final
        : public antwika::console::IConsoleControls
    {
    public:
        /**
         * @brief Construct the controls over the run's options.
         * @param options The bindings and the board. Must outlive
         * this object.
         */
        explicit OptionsConsoleControls(
            const OptionsState &options) noexcept;

        OptionsConsoleControls(const OptionsConsoleControls &) = delete;
        OptionsConsoleControls(OptionsConsoleControls &&) = delete;

        OptionsConsoleControls &operator=(
            const OptionsConsoleControls &) = delete;
        OptionsConsoleControls &operator=(
            OptionsConsoleControls &&) = delete;

        /**
         * @brief Get the key bound to Action::ConsoleToggle.
         * @return The toggle key.
         */
        [[nodiscard]] antwika::input::Key toggleKey() const override;

        /**
         * @brief Get the key bound to Action::ConsoleExecute.
         * @return The execute key.
         */
        [[nodiscard]] antwika::input::Key executeKey() const override;

        /**
         * @brief Get the board the options screen has picked.
         * @return The layout every typed character goes through.
         */
        [[nodiscard]] antwika::console::KeyboardLayout
        keyboard() const override;

    private:
        const OptionsState &options;
    };

} // namespace antwika::game
