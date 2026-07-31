#include "antwika/game/SessionStore.hpp"

#include <vector>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;

    SessionStore::SessionStore(
        World &world,
        PathIndex &paths,
        Camera &camera,
        GameState &state,
        GridExtent extent,
        std::uint64_t seed)
        : world(world),
          paths(paths),
          camera(camera),
          state(state),
          extent(extent),
          seed(seed)
    {
    }

    SaveGame SessionStore::take() const
    {
        const auto frame = snapshotOf(world, paths, camera, extent);

        // Every branch left on the excluded line is the allocator's.
        // Three are the throw edges of copying the three vectors.
        // The rest are heap branches nothing here is big enough for.
        // And the unwind paths that go with them.
        // Confirmed with gcov -b, as the coverage doc requires.
        return saveGameOf(
            GameSummary{ // GCOVR_EXCL_LINE
                .state = state,
                .paths = frame.paths,
                .walkers = frame.walkers,
                .buildings = frame.buildings,
                .camera = camera},
            extent,
            seed);
    } // GCOVR_EXCL_LINE

    void SessionStore::restore(const SaveGame &save)
    {
        // Collected before anything is staged.
        // Destroying mid-walk would be walking what is being changed.
        std::vector<Entity> standing;
        for (const auto entity : world.view<Cell>())
        {
            standing.push_back(entity);
        }

        for (const auto entity : standing)
        {
            world.destroy(entity);
        }

        paths = pathIndexOf(save);
        camera = save.camera;
        state = save.state;

        for (const auto cell : save.paths)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
        }

        for (const auto &walker : save.walkers)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, walker.at);
            world.add<Walker>(entity, Walker{.facing = walker.facing});
        }
    }

} // namespace antwika::game
