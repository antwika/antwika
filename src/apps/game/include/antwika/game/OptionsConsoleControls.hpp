#pragma once

#include <antwika/console/IConsoleControls.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    class OptionsConsoleControls final
        : public antwika::console::IConsoleControls
    {
    public:
        explicit OptionsConsoleControls(
            const OptionsState &options) noexcept;

        OptionsConsoleControls(const OptionsConsoleControls &) = delete;
        OptionsConsoleControls(OptionsConsoleControls &&) = delete;

        OptionsConsoleControls &operator=(
            const OptionsConsoleControls &) = delete;
        OptionsConsoleControls &operator=(
            OptionsConsoleControls &&) = delete;

        [[nodiscard]] antwika::input::Key toggleKey() const override;

        [[nodiscard]] antwika::input::Key executeKey() const override;

        [[nodiscard]] antwika::console::KeyboardLayout
        keyboard() const override;

    private:
        const OptionsState &options;
    };

}
