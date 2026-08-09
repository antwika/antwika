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

    class Sdl3SoundBackend final : public ISoundBackend
    {
    public:
        explicit Sdl3SoundBackend(ILogger &logger);

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] SoundCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) override;

    private:
        ILogger &logger;
    };

}
