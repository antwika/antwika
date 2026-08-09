#include "Sdl3SoundBackend.hpp"

#include <string>

#include <antwika/log/Level.hpp>
#include <antwika/sound/SoundError.hpp>

#include "Sdl3Device.hpp"
#include "Sdl3Runtime.hpp"

namespace antwika::sound
{

    using antwika::log::Level;

    Sdl3SoundBackend::Sdl3SoundBackend(ILogger &logger) : logger(logger)
    {
    }

    std::string_view Sdl3SoundBackend::name() const
    {
        return "sdl3";
    }

    SoundCapabilities Sdl3SoundBackend::capabilities() const
    {
        return SoundCapabilities{.playback = true, .selfDriven = false};
    }

    std::unique_ptr<IDevice> Sdl3SoundBackend::openDevice(
        const DeviceDesc &desc)
    {
        if (!desc.format.isValid())
        {
            throw SoundError(
                "sound.sdl3: a device cannot be opened at "
                + std::to_string(desc.format.rate) + " Hz with "
                + std::to_string(desc.format.channels) + " channels");
        }

        try
        {
            auto device =
                std::make_unique<antwika::sdl3::Sdl3Device>(logger, desc);

            logger.log(Level::Debug, "sound.sdl3: device opened");

            return device;
        }
        catch (const antwika::sdl3::Sdl3Error &error)
        {
            throw SoundError(std::string("sound.") + error.what());
        }
    }

}
