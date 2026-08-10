#include "RaylibAudioRuntime.hpp"

#include <raylib.h>

#include <antwika/log/Level.hpp>

namespace antwika::raylib
{

    using antwika::log::Level;

    std::shared_ptr<RaylibAudioRuntime> RaylibAudioRuntime::acquire(
        ILogger &logger)
    {
        static std::weak_ptr<RaylibAudioRuntime> shared;

        auto runtime = shared.lock();

        if (!runtime)
        {
            runtime = std::make_shared<RaylibAudioRuntime>(logger);
            shared = runtime;
        }

        return runtime;
    }

    RaylibAudioRuntime::RaylibAudioRuntime(ILogger &logger)
    {
        SetTraceLogLevel(LOG_WARNING);

        InitAudioDevice();

        ready = IsAudioDeviceReady();

        if (ready)
        {
            logger.log(Level::Info, "sound.raylib: audio device open");

            return;
        }

        logger.log(
            Level::Warning,
            "sound.raylib: no playback device opened, so frames will be "
            "rendered but not heard; a sound server has to be reachable "
            "before the process starts");
    }

    RaylibAudioRuntime::~RaylibAudioRuntime()
    {
        if (ready)
        {
            CloseAudioDevice();
        }
    }

    bool RaylibAudioRuntime::isReady() const noexcept
    {
        return ready;
    }

}
