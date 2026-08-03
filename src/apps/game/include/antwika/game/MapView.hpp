#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    /**
     * @brief Which picture of the city is being looked at.
     *
     * **Normal is the city itself, and every other value is one number
     * about it painted over the ground.** A player builds in the first
     * and reads in the rest, which is the whole distinction: nothing in
     * an overlay is a thing to click on, and the city goes on running
     * and taking presses underneath whichever is showing.
     *
     * Values are contiguous from zero, so a view can index a table, and
     * Normal is zero so a default-constructed state is the city.
     */
    enum class MapView : std::uint8_t
    {
        Normal = 0,     ///< The city, with nothing painted over it.
        Desirability,   ///< How nice it is to live on each cell.
        Food,           ///< How well fed each building is.
        Water,          ///< How well Service::Water still reaches.
        Health,         ///< How well Service::Health still reaches.
        Fire,           ///< How well Service::Safety still reaches.
        Damage,         ///< How well Service::Structure still reaches.
    };

    /**
     * @brief How many views there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kMapViewCount =
        static_cast<std::size_t>(MapView::Damage) + 1;

    /**
     * @brief Get a view's index, for addressing a per-view table.
     * @param view The view to index.
     * @return The index, always below kMapViewCount.
     */
    [[nodiscard]] constexpr std::size_t mapViewIndex(MapView view) noexcept
    {
        return static_cast<std::size_t>(view);
    }

    /**
     * @brief Get which service a view paints, if it paints one.
     *
     * **The one crossing between a view and a service**, so the
     * dropdown's list and what gets painted cannot name two different
     * things. Four of the seven views are a service each; the other
     * three are not services at all.
     *
     * @param view The view to ask about.
     * @return The service it paints, or nullopt.
     */
    [[nodiscard]] constexpr std::optional<Service> mapViewService(
        MapView view) noexcept
    {
        constexpr std::array<std::optional<Service>, kMapViewCount>
            services{
                std::nullopt,        // Normal
                std::nullopt,        // Desirability
                std::nullopt,        // Food
                Service::Water,      // Water
                Service::Health,     // Health
                Service::Safety,     // Fire
                Service::Structure}; // Damage

        return services[mapViewIndex(view) % kMapViewCount];
    }

    /**
     * @brief Get which resource a view paints, if it paints one.
     *
     * mapViewService()'s counterpart, and separate from it because a
     * good is an amount and a service is a state -- the very
     * distinction Service.hpp is written round. A view answers at most
     * one of the two, which the static_assert below holds it to.
     *
     * @param view The view to ask about.
     * @return The resource it paints, or nullopt.
     */
    [[nodiscard]] constexpr std::optional<Resource> mapViewResource(
        MapView view) noexcept
    {
        constexpr std::array<std::optional<Resource>, kMapViewCount>
            resources{
                std::nullopt,     // Normal
                std::nullopt,     // Desirability
                Resource::Food,   // Food
                std::nullopt,     // Water
                std::nullopt,     // Health
                std::nullopt,     // Fire
                std::nullopt};    // Damage

        return resources[mapViewIndex(view) % kMapViewCount];
    }

    // A view paints an amount or a state, never both.
    // Two tables are where that could quietly stop being true.
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

    // Every service has a view of its own.
    // A service nobody can look at is one nobody can act on.
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
    static_assert(mapViewService(MapView::Fire) == Service::Safety);
    static_assert(mapViewResource(MapView::Food) == Resource::Food);

    /**
     * @brief Get what a view is called in the dropdown.
     *
     * A MessageId rather than the English word, for toolLabel()'s
     * reason: a caption is read by a person and goes through
     * antwika::i18n like every other one.
     *
     * @param view The view to name.
     * @return The label's id, in MapView order.
     */
    [[nodiscard]] constexpr MessageId mapViewLabel(MapView view) noexcept
    {
        constexpr std::array<MessageId, kMapViewCount> labels{
            MessageId::ViewNormal,
            MessageId::ViewDesirability,
            MessageId::ViewFood,
            MessageId::ViewWater,
            MessageId::ViewHealth,
            MessageId::ViewFire,
            MessageId::ViewDamage};

        return labels[mapViewIndex(view) % kMapViewCount];
    }

    /**
     * @brief Which picture of the city is showing.
     *
     * **PauseState's shape exactly, and it is simulation state on the
     * same terms.** It is written by UiSink inside the tick path from a
     * click nothing records, and regenerated by a replay from that same
     * click, so it is owned by main.cpp beside the camera and the pause
     * rather than by either the sink or the renderer -- a renderer built
     * before the run has to read it.
     *
     * Nothing about it is persisted. A save holds a city rather than
     * which way somebody was looking at it, exactly as it holds no
     * pause; and it is not in GameSummary either, since no overlay can
     * change a single thing a run computes.
     */
    class MapViewState final
    {
    public:
        /**
         * @brief Get which picture is showing.
         * @return The selected view; Normal until something selects one.
         */
        [[nodiscard]] MapView view() const noexcept;

        /**
         * @brief Choose which picture shows.
         *
         * Absolute rather than a toggle, for PauseState's reason: a
         * value arriving twice settles on one answer instead of undoing
         * itself.
         *
         * @param wanted The view to show.
         */
        void set(MapView wanted) noexcept;

    private:
        MapView showing = MapView::Normal;
    };

} // namespace antwika::game
