#pragma once

#include <memory>
#include <string_view>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/SoundCapabilities.hpp"

namespace antwika::sound
{

    /**
     * @brief Opens devices, and reports what it can do.
     *
     * The one seam between Antwika and a concrete audio framework.
     * Exactly one implementation is compiled into a given build, chosen
     * by ANTWIKA_SOUND_BACKEND, so no code above this interface names
     * SDL, miniaudio or anything like them.
     */
    class ISoundBackend
    {
    public:
        virtual ~ISoundBackend() = default;

        /**
         * @brief Get what this backend is called.
         * @return Its name, which does not change over its lifetime.
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief Get what this backend can do.
         * @return Its capabilities, which do not change either.
         */
        [[nodiscard]] virtual SoundCapabilities capabilities() const = 0;

        /**
         * @brief Open a device.
         * @param desc What to open it as.
         * @return The device, never null.
         * @throws SoundError If the format is not one audio could be
         * described by, or the framework refused.
         */
        [[nodiscard]] virtual std::unique_ptr<IDevice> openDevice(
            const DeviceDesc &desc) = 0;
    };

} // namespace antwika::sound
