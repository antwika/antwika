#pragma once

#include <cstdint>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

namespace antwika::replay::fakes
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class FakeFoldingStateReducer final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            fold(event.tick);
            fold(event.event.name);
            fold(event.event.payload);
        }

        [[nodiscard]] std::uint64_t hash() const noexcept
        {
            return state;
        }

    private:
        void fold(std::uint64_t value)
        {
            for (int i = 0; i < 8; ++i)
            {
                fold(static_cast<unsigned char>((value >> (i * 8)) & 0xFF));
            }
        }

        void fold(const std::string &value)
        {
            for (unsigned char byte : value)
            {
                fold(byte);
            }
        }

        void fold(unsigned char byte)
        {
            state ^= byte;
            state *= 1099511628211ULL;
        }

        std::uint64_t state{14695981039346656037ULL};
    };

}
