#pragma once

#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    enum class AppMode : std::uint8_t
    {
        MainMenu = 0,

        WorldMap,

        CityMap,

        SaveLoad,
    };

    [[nodiscard]] constexpr bool simulates(AppMode mode) noexcept
    {
        return mode == AppMode::CityMap || mode == AppMode::WorldMap;
    }

    class AppModeState final : public ITickEventSink
    {
    public:
        explicit AppModeState(AppMode initial = AppMode::MainMenu) noexcept
            : current(initial), staged(initial)
        {
        }

        AppModeState(const AppModeState &) = delete;
        AppModeState(AppModeState &&) = delete;

        AppModeState &operator=(const AppModeState &) = delete;
        AppModeState &operator=(AppModeState &&) = delete;

        [[nodiscard]] AppMode mode() const noexcept
        {
            return current;
        }

        [[nodiscard]] AppMode next() const noexcept
        {
            return staged;
        }

        void request(AppMode mode) noexcept
        {
            staged = mode;
        }

        void handle(const TickEvent &event) override;

    private:
        AppMode current;
        AppMode staged;
    };

}
