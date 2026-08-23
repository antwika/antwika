#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/sound/DeviceSpec.hpp"
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

        [[nodiscard]] std::string_view getName() const override;
        [[nodiscard]] SoundCapabilities getCapabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceSpec &spec) override;

    private:
        ILogger &logger;
    };

}
