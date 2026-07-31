#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/sound/ISoundBackend.hpp"

namespace antwika::sound
{

    using antwika::log::ILogger;

    /**
     * @brief Create the sound backend chosen at build time.
     *
     * Declared here but deliberately not defined here: the definition
     * comes from whichever backend under backends/ was selected via
     * ANTWIKA_SOUND_BACKEND. That is what keeps every concrete audio
     * framework out of src/ entirely. Linking a program that calls this
     * without linking a backend is a link error, by design.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected backend, never null.
     * @throws SoundError If the underlying framework failed to
     * initialise.
     */
    [[nodiscard]] std::unique_ptr<ISoundBackend> makeSelectedSoundBackend(
        ILogger &logger);

} // namespace antwika::sound
