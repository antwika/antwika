#pragma once

#include <memory>
#include <string_view>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/SoundCapabilities.hpp"

namespace antwika::sound
{

    class ISoundBackend
    {
    public:
        virtual ~ISoundBackend() = default;

        [[nodiscard]] virtual std::string_view name() const = 0;

        [[nodiscard]] virtual SoundCapabilities capabilities() const = 0;

        [[nodiscard]] virtual std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) = 0;
    };

}
