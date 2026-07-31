#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/SoundCapabilities.hpp>

namespace antwika::sound
{

    using antwika::log::ILogger;

    /**
     * @brief Plays sound through SDL3.
     *
     * It reports selfDriven false, because SDL3's stream API with a null
     * callback is a push model: nothing is rendered until a caller pumps
     * the device, on the caller's own thread.  So this backend adds no
     * thread, no lock and no queue of ours to a project that has none.
     *
     * It claims SDL's audio subsystem and nothing else, so a build
     * selecting sdl3 for sound alone never asks for a display.
     */
    class Sdl3SoundBackend final : public ISoundBackend
    {
    public:
        /**
         * @brief Create the backend.
         * @param logger Receives its diagnostics, and every device's.
         */
        explicit Sdl3SoundBackend(ILogger &logger);

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] SoundCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) override;

    private:
        ILogger &logger;
    };

} // namespace antwika::sound
