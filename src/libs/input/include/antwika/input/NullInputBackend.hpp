#pragma once

#include <optional>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/InputCapabilities.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    /**
     * @brief Backend that accepts both devices and reports nothing.
     *
     * Not a placeholder for a real backend: it is what lets tests, CI and
     * replay verification run with no display and no input framework
     * installed. A session recorded against a real backend must reproduce
     * the same state under this one, which is the check that input reaches
     * the engine only as recorded events.
     *
     * It reports both devices rather than neither, because it stands in
     * for a full input source during a headless run: an application
     * verifying a replay must bind the same actions it bound live. Having
     * nothing to report is not the same as having no keyboard.
     *
     * It reports no events, so a program driven purely by this backend
     * ends when its own stop condition says so, not when a user presses a
     * key.
     */
    class NullInputBackend final : public IInputBackend
    {
    public:
        /**
         * @brief Construct the backend.
         * @param logger Receives the backend's diagnostics.
         */
        explicit NullInputBackend(ILogger &logger);

        NullInputBackend(const NullInputBackend &) = delete;
        NullInputBackend(NullInputBackend &&) = delete;

        NullInputBackend &operator=(const NullInputBackend &) = delete;
        NullInputBackend &operator=(NullInputBackend &&) = delete;

        /**
         * @brief Get the backend's name.
         * @return Always "null".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Get which devices this backend deals in.
         * @return Both of them, neither of which ever reports anything.
         */
        [[nodiscard]] InputCapabilities capabilities() const override;

        /**
         * @brief Take the next reported event.
         * @return Always nullopt: there is nothing to report one.
         */
        [[nodiscard]] std::optional<InputEvent> pollEvent() override;
    };

} // namespace antwika::input
