#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IInputBackend.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    /**
     * @brief Create the input backend chosen at build time.
     *
     * Declared here but deliberately not defined here: the definition
     * comes from whichever backend under backends/ was selected via
     * ANTWIKA_INPUT_BACKEND. That is what keeps every concrete input
     * framework out of src/ entirely. Linking a program that calls this
     * without linking a backend is a link error, by design.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected backend, never null.
     * @throws InputError If the underlying framework failed to initialise.
     */
    [[nodiscard]] std::unique_ptr<IInputBackend> makeSelectedInputBackend(
        ILogger &logger);

} // namespace antwika::input
