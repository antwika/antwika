#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IClipboard.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    /**
     * @brief Create the clipboard the build's input backend reaches.
     *
     * Declared here but deliberately not defined here, exactly as
     * makeSelectedInputBackend() is: the definition comes from
     * whichever backend under backends/ was selected via
     * ANTWIKA_INPUT_BACKEND, so no code under src/ names the window
     * system that really holds the characters.
     *
     * The null backend answers with a MemoryClipboard, so a headless
     * run still pastes what it copied within itself.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected clipboard, never null.
     * @throws InputError If the underlying framework failed to
     * initialise.
     */
    [[nodiscard]] std::unique_ptr<IClipboard> makeSelectedClipboard(
        ILogger &logger);

} // namespace antwika::input
