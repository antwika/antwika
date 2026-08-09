#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>

namespace antwika::game
{

    enum class Service : std::uint8_t
    {
        Water = 0,
        Health,
    };

    [[nodiscard]] constexpr Service enumBound(Service) noexcept
    {
        return Service::Health;
    }

    inline constexpr std::size_t kServiceCount =
        antwika::enums::kCount<Service>;

    [[nodiscard]] constexpr std::size_t serviceIndex(
        const Service service) noexcept
    {
        return antwika::enums::index(service);
    }

    inline constexpr auto kServices = antwika::enums::kAll<Service>;

    inline constexpr antwika::enums::NameTable<Service> kServiceNames{
        {"water", "health"}};

    [[nodiscard]] constexpr std::string_view serviceName(
        const Service service) noexcept
    {
        return kServiceNames.name(service);
    }

    static_assert(serviceName(Service::Water) == "water");
    static_assert(serviceName(Service::Health) == "health");

}
