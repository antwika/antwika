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
        return live;
    }

    void WorldMapState::openCityAt(
        std::size_t index, PathIndex &livePaths, Camera &liveCamera)
    {
        requireCity(index);
        closeCity(livePaths, liveCamera);

        live = index;
        open = true;
        livePaths = paths[index];
        liveCamera = cameras[index];
    }

    void WorldMapState::closeCity(
        PathIndex &livePaths, Camera &liveCamera)
    {
        if (!open)
        {
            return;
        }

        paths[live] = livePaths;
        cameras[live] = liveCamera;
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

} // namespace antwika::game
