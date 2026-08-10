#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/SoundCapabilities.hpp>

#include "RaylibAudioRuntime.hpp"

namespace antwika::sound
{

    using antwika::log::ILogger;

    class RaylibSoundBackend final : public ISoundBackend
    {
    public:
        explicit RaylibSoundBackend(ILogger &logger);

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] SoundCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) override;

    private:
        ILogger &logger;
        std::shared_ptr<antwika::raylib::RaylibAudioRuntime> audio;
    };

}
