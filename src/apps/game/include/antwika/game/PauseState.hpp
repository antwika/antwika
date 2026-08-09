#pragma once

namespace antwika::game
{

    class PauseState final
    {
    public:
        void set(bool paused) noexcept;

        [[nodiscard]] bool paused() const noexcept;

    private:
        bool held = false;
    };

}
