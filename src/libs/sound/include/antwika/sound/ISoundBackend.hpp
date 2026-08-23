#pragma once

#include <memory>
#include <string_view>

#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/SoundCapabilities.hpp"

namespace antwika::sound
{

    class ISoundBackend
    {
    public:
        virtual ~ISoundBackend() = default;

        [[nodiscard]] virtual std::string_view getName() const = 0;

        [[nodiscard]] virtual SoundCapabilities getCapabilities() const = 0;

        [[nodiscard]] virtual std::unique_ptr<IDevice> openDevice(
            const DeviceSpec &spec) = 0;
    };

}
