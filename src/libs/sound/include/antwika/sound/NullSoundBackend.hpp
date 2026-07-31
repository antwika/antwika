#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/ISoundBackend.hpp"
#include "antwika/sound/SoundCapabilities.hpp"

namespace antwika::sound
{

    using antwika::log::ILogger;

    /**
     * @brief A backend that opens devices which discard what they play.
     *
     * It lives here rather than under backends/null/ for the reason
     * input::NullInputBackend does: `backends/` is exempt from the
     * coverage gate and `src/` is not, so a headless implementation kept
     * here is one the gate can actually hold to 100%.
     * All that lives under backends/null/ is the two-line factory.
     */
    class NullSoundBackend final : public ISoundBackend
    {
    public:
        /**
         * @brief Construct the backend over where it reports to.
         * @param logger Receives its diagnostics; must outlive it.
         */
        explicit NullSoundBackend(ILogger &logger);

        NullSoundBackend(const NullSoundBackend &) = delete;
        NullSoundBackend(NullSoundBackend &&) = delete;

        NullSoundBackend &operator=(const NullSoundBackend &) = delete;
        NullSoundBackend &operator=(NullSoundBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;
        [[nodiscard]] SoundCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) override;

    private:
        ILogger &logger;
    };

} // namespace antwika::sound
