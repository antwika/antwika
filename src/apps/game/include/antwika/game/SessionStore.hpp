#pragma once

#include <cstdint>

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    class SessionStore final
    {
    public:
        SessionStore(
            World &world,
            PathIndex &paths,
            BuildingIndex &built,
            Camera &camera,
            GameState &state,
            GridExtent extent,
            std::uint64_t seed);

        SessionStore(const SessionStore &) = delete;
        SessionStore(SessionStore &&) = delete;

        SessionStore &operator=(const SessionStore &) = delete;
        SessionStore &operator=(SessionStore &&) = delete;

        [[nodiscard]] SaveGame take() const;

        void restore(const SaveGame &save);

    private:
        World &world;
        PathIndex &paths;
        BuildingIndex &built;
        Camera &camera;
        GameState &state;
        GridExtent extent;
        std::uint64_t seed;
    };

}
