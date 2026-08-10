#pragma once

#include <memory>
#include <stdexcept>

#include <antwika/log/ILogger.hpp>

namespace antwika::raylib
{

    using antwika::log::ILogger;

    class RaylibError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class RaylibAudioRuntime final
    {
    public:
        [[nodiscard]] static std::shared_ptr<RaylibAudioRuntime> acquire(
            ILogger &logger);

        explicit RaylibAudioRuntime(ILogger &logger);

        RaylibAudioRuntime(const RaylibAudioRuntime &) = delete;
        RaylibAudioRuntime(RaylibAudioRuntime &&) = delete;

        RaylibAudioRuntime &operator=(const RaylibAudioRuntime &) = delete;
        RaylibAudioRuntime &operator=(RaylibAudioRuntime &&) = delete;

        ~RaylibAudioRuntime();

        [[nodiscard]] bool isReady() const noexcept;

    private:
        bool ready = false;
    };

}
