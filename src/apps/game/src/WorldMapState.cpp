#include "antwika/game/WorldMapState.hpp"

#include <utility>

#include "antwika/game/WorldMapError.hpp"

namespace antwika::game
{

    WorldMapState::WorldMapState(WorldMap world)
        : map(std::move(world))
    {
    }

    const WorldMap &WorldMapState::world() const noexcept
    {
        return map;
    }

    MapView WorldMapState::view() const noexcept
    {
        return showing;
    }

    std::size_t WorldMapState::openCity() const
    {
        if (showing != MapView::City)
        {
            throw WorldMapError("No city is open");
        }
        return city;
    }

    void WorldMapState::openCityAt(std::size_t index)
    {
        requireCity(index);
        city = index;
        showing = MapView::City;
    }

    void WorldMapState::closeCity()
    {
        showing = MapView::World;
    }

    PathIndex &WorldMapState::cityPaths(std::size_t index)
    {
        requireCity(index);
        return paths[index];
    }

    const PathIndex &WorldMapState::cityPaths(std::size_t index) const
    {
        requireCity(index);
        return paths[index];
    }

    Camera &WorldMapState::cityCamera(std::size_t index)
    {
        requireCity(index);
        return cameras[index];
    }

    const Camera &WorldMapState::cityCamera(std::size_t index) const
    {
        requireCity(index);
        return cameras[index];
    }

    void WorldMapState::requireCity(std::size_t index) const
    {
        if (index >= kCityCount)
        {
            throw WorldMapError("No such city on this world map");
        }
    }

} // namespace antwika::game
