#include "antwika/sound/NullSoundBackend.hpp"

#include <memory>
#include <string>

#include <antwika/log/Level.hpp>

#include "antwika/sound/NullDevice.hpp"
#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    NullSoundBackend::NullSoundBackend(ILogger &logger) : logger(logger)
    {
    }

    std::string_view NullSoundBackend::name() const
    {
        return "null";
    }

    SoundCapabilities NullSoundBackend::capabilities() const
    {
        // Nothing is heard and nothing drives itself.
        // Saying so is what lets the conformance suite skip honestly.
        return SoundCapabilities{.playback = false, .selfDriven = false};
    }

    std::unique_ptr<IDevice> NullSoundBackend::openDevice(
        const DeviceDesc &desc)
    {
        // Refused here rather than accepted and ignored.
        // A backend that plays nothing still answers as a real one does.
        // Otherwise it is no use as a stand-in for one.
        if (!desc.format.isValid())
        {
            throw SoundError(
                "antwika::sound: a device cannot be opened at "
                + std::to_string(desc.format.rate) + " Hz with "
                + std::to_string(desc.format.channels) + " channels");
        }

        logger.log(
            antwika::log::Level::Debug,
            "Opening a null sound device");

        return std::make_unique<NullDevice>(desc);
    }

} // namespace antwika::sound
