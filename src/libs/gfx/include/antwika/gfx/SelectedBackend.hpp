#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IGfxBackend.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    /**
     * @brief Create the graphics backend chosen at build time.
     *
     * Declared here but deliberately not defined here: the definition
     * comes from whichever backend under backends/ was selected via
     * ANTWIKA_GFX_BACKEND. That is what keeps every concrete graphics
     * framework out of src/ entirely. Linking a program that calls this
     * without linking a backend is a link error, by design.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected backend, never null.
     * @throws GfxError If the underlying framework failed to initialise.
     */
    [[nodiscard]] std::unique_ptr<IGfxBackend> makeSelectedBackend(
        ILogger &logger);

} // namespace antwika::gfx
