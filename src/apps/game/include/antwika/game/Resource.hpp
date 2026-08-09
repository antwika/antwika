#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>

namespace antwika::game
{

    enum class Resource : std::uint8_t
    {
        Food = 0,
        Clay,
        Pottery,
    };

    [[nodiscard]] constexpr Resource enumBound(Resource) noexcept
    {
        return Resource::Pottery;
    }

    inline constexpr std::size_t kResourceCount =
        antwika::enums::kCount<Resource>;

    [[nodiscard]] constexpr std::size_t resourceIndex(
        const Resource resource) noexcept
    {
        return antwika::enums::index(resource);
    }

    inline constexpr auto kResources = antwika::enums::kAll<Resource>;

    inline constexpr antwika::enums::NameTable<Resource> kResourceNames{
        {"food", "clay", "pottery"}};

    [[nodiscard]] constexpr std::string_view resourceName(
        const Resource resource) noexcept
    {
        return kResourceNames.name(resource);
    }

    [[nodiscard]] constexpr std::optional<Resource> resourceFromName(
        const std::string_view name) noexcept
    {
        return kResourceNames.from(name);
    }

    [[nodiscard]] constexpr bool sustains(const Resource resource) noexcept
    {
        constexpr std::array<bool, kResourceCount> sustaining{
            true,
            false,
            false,
        };

        return antwika::enums::pick(sustaining, resource);
    }

    static_assert(resourceName(Resource::Food) == "food");
    static_assert(resourceName(Resource::Pottery) == "pottery");
    static_assert(resourceFromName("pottery") == Resource::Pottery);
    static_assert(!resourceFromName("marble").has_value());
    static_assert(sustains(Resource::Food));
    static_assert(!sustains(Resource::Clay));

}
