#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    enum class MapView : std::uint8_t
    {
        Normal = 0,
        Desirability,
        Food,
        Water,
        Medicine,
        Fire,
        Damage,
    };

    [[nodiscard]] constexpr MapView enumBound(MapView) noexcept
    {
        return MapView::Damage;
    }

    inline constexpr std::size_t kMapViewCount =
        antwika::enums::kCount<MapView>;

    [[nodiscard]] constexpr std::size_t mapViewIndex(MapView view) noexcept
    {
        return static_cast<std::size_t>(view);
    }

    [[nodiscard]] constexpr std::optional<Service> mapViewService(
        MapView view) noexcept
    {
        constexpr std::array<std::optional<Service>, kMapViewCount>
            services{
                std::nullopt,
                std::nullopt,
                std::nullopt,
                Service::Water,
                Service::Health,
                std::nullopt,
                std::nullopt};

        return antwika::enums::pick(services, view);
    }

    [[nodiscard]] constexpr std::optional<Resource> mapViewResource(
        MapView view) noexcept
    {
        constexpr std::array<std::optional<Resource>, kMapViewCount>
            resources{
                std::nullopt,
                std::nullopt,
                Resource::Food,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                std::nullopt};

        return antwika::enums::pick(resources, view);
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kMapViewCount; ++index)
            {
                const auto view = static_cast<MapView>(index);

                if (mapViewService(view).has_value()
                    && mapViewResource(view).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "a view may paint a good or a service, not both");

    static_assert(
        []
        {
            for (const auto service : kServices)
            {
                bool painted = false;

                for (std::size_t index = 0; index < kMapViewCount; ++index)
                {
                    painted = painted
                        || mapViewService(static_cast<MapView>(index))
                            == service;
                }

                if (!painted)
                {
                    return false;
                }
            }

            return true;
        }(),
        "every service must have a view that paints it");

    static_assert(!mapViewService(MapView::Normal).has_value());
    static_assert(mapViewService(MapView::Medicine) == Service::Health);
    static_assert(!mapViewService(MapView::Fire).has_value());
    static_assert(mapViewResource(MapView::Food) == Resource::Food);

    [[nodiscard]] constexpr MessageId mapViewLabel(MapView view) noexcept
    {
        constexpr std::array<MessageId, kMapViewCount> labels{
            MessageId::ViewNormal,
            MessageId::ViewDesirability,
            MessageId::ViewFood,
            MessageId::ViewWater,
            MessageId::ViewMedicine,
            MessageId::ViewFire,
            MessageId::ViewDamage};

        return antwika::enums::pick(labels, view);
    }

    class MapViewState final
    {
    public:
        [[nodiscard]] MapView view() const noexcept;

        void set(MapView wanted) noexcept;

    private:
        MapView showing = MapView::Normal;
    };

}
