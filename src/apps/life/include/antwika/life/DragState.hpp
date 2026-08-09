#pragma once

namespace antwika::life
{

    class DragState final
    {
    public:
        void begin() noexcept;

        void end() noexcept;

        [[nodiscard]] bool inProgress() const noexcept;

    private:
        bool dragging = false;
    };

}
