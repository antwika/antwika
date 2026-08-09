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

    bool WorldMapState::cityOpen() const noexcept
    {
        return open;
    }

    std::size_t WorldMapState::city() const noexcept
    {
        return liveCity;
    }

    void WorldMapState::openCityAt(
        std::size_t index, const LiveGrid &live)
    {
        requireCity(index);
        closeCity(live);

        liveCity = index;
        open = true;
        live.paths = paths[index];
        live.camera = cameras[index];

        restoreCityGrid(live.world, live.built, live.paths, grids[index]);
    }

    void WorldMapState::closeCity(const LiveGrid &live)
    {
        if (!open)
        {
            return;
        }

        live.world.commit();

        paths[liveCity] = live.paths;
        cameras[liveCity] = live.camera;
        grids[liveCity] = cityGridOf(live.world);
        open = false;
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

}
