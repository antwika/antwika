#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/sound/DeviceSpec.hpp>
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

        [[nodiscard]] std::string_view getName() const override;

        [[nodiscard]] SoundCapabilities getCapabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceSpec &spec) override;

    private:
        ILogger &logger;
        std::shared_ptr<antwika::raylib::RaylibAudioRuntime> audio;
    };

}
