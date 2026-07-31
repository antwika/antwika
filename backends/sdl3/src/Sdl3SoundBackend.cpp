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
        // Not selfDriven, which is the whole finding this rests on.
        // SDL3's stream API with a null callback is a push model.
        // A device here is pumped by whatever pumps everything else.
        return SoundCapabilities{.playback = true, .selfDriven = false};
    }

    // SDL raises this directory's own error type, which stops here.
    // Above this seam, a sound failure is only ever a SoundError.
    std::unique_ptr<IDevice> Sdl3SoundBackend::openDevice(
        const DeviceDesc &desc)
    {
        // Refused before SDL is asked.
        // Every backend then refuses the same description alike.
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

} // namespace antwika::sound
