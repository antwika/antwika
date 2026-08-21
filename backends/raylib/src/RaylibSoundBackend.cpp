#include "RaylibSoundBackend.hpp"

#include <string>

#include <antwika/log/Level.hpp>
#include <antwika/sound/SoundError.hpp>

#include "RaylibDevice.hpp"

namespace antwika::sound
{

    using antwika::log::Level;

    RaylibSoundBackend::RaylibSoundBackend(ILogger &logger)
        : logger(logger),
          audio(antwika::raylib::RaylibAudioRuntime::acquire(logger))
    {
    }

    std::string_view RaylibSoundBackend::name() const
    {
        return "raylib";
    }

    SoundCapabilities RaylibSoundBackend::capabilities() const
    {
        return SoundCapabilities{
            .playback = audio->isReady(), .selfDriven = false};
    }

    std::unique_ptr<IDevice> RaylibSoundBackend::openDevice(
        const DeviceSpec &spec)
    {
        if (!spec.format.isValid())
        {
            throw SoundError(
                "sound.raylib: a device cannot be opened at "
                + std::to_string(spec.format.rate) + " Hz with "
                + std::to_string(spec.format.channels) + " channels");
        }

        try
        {
            auto device =
                std::make_unique<antwika::raylib::RaylibDevice>(logger, spec);

            logger.log(Level::Debug, "sound.raylib: device opened");

            return device;
        }
        catch (const antwika::raylib::RaylibError &error)
        {
            throw SoundError(std::string("sound.") + error.what());
        }
    }

}
