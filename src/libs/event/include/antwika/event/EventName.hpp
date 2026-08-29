#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

#include "antwika/event/EventNameSeeds.hpp"

namespace antwika::event
{

    class EventName final
    {
    public:
        constexpr EventName() = default;

        explicit EventName(std::string_view text);

        explicit EventName(const char *text);

        explicit EventName(const std::string &text);

        [[nodiscard]] std::string_view getText() const;

        [[nodiscard]] static consteval EventName getSeeded(std::string_view text)
        {
            return EventName{getSeededIdOf(text)};
        }

        constexpr bool operator==(const EventName &other) const = default;

    private:
        explicit constexpr EventName(std::uint32_t id) : id(id) {}

        std::uint32_t id{0};
    };

    std::ostream &operator<<(std::ostream &out, const EventName &name);

}
