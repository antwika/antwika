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

    class NullSoundBackend final : public ISoundBackend
    {
    public:
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

}
