#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Expand.hpp>
#include <antwika/worldgen/Grow.hpp>
#include <antwika/worldgen/Ruleset.hpp>

namespace
{

    using antwika::worldgen::ChunkOutcome;
    using antwika::worldgen::ChunkRequest;
    using antwika::worldgen::ChunkShape;
    using antwika::worldgen::chunkVoxels;
    using antwika::worldgen::cityRuleset;
    using antwika::worldgen::CompiledRuleset;
    using antwika::worldgen::cubeCount;
    using antwika::worldgen::growChunk;

    using Clock = std::chrono::steady_clock;

    [[nodiscard]] double millisSince(const Clock::time_point began)
    {
        return std::chrono::duration<double, std::milli>(
                   Clock::now() - began)
            .count();
    }

    void timeOne(
        const CompiledRuleset &rules,
        const ChunkShape shape,
        const std::uint64_t seed)
    {
        const auto began = Clock::now();
        const auto result =
            growChunk(rules, ChunkRequest{.seed = seed, .shape = shape});
        const double grew = millisSince(began);

        const auto laying = Clock::now();
        const auto voxels = chunkVoxels(result.cubeCells);
        const double laid = millisSince(laying);

        std::printf(
            "%2dx%2dx%-3d cells=%6zu seed=%2llu %-13s grow=%8.2fms "
            "lay=%6.2fms cubes=%6zu voxels=%7zu\n",
            shape.width,
            shape.depth,
            shape.height,
            cubeCount(shape),
            static_cast<unsigned long long>(seed),
            result.outcome == ChunkOutcome::Grown ? "grown"
                                                  : "not grown",
            grew,
            laid,
            result.cubeCells.size(),
            voxels.size());
    }

}

int main()
{
    const auto compiling = Clock::now();
    const CompiledRuleset rules(cityRuleset());
    std::printf(
        "ruleset of %zu pieces compiled in %.2fms\n\n",
        rules.size(),
        millisSince(compiling));

    for (const ChunkShape shape :
         {ChunkShape{.width = 16, .depth = 16, .height = 16},
          ChunkShape{.width = 16, .depth = 16, .height = 32},
          ChunkShape{.width = 16, .depth = 16, .height = 64},
          ChunkShape{.width = 16, .depth = 16, .height = 128}})
    {
        for (std::uint64_t seed = 1; seed <= 3; ++seed)
        {
            timeOne(rules, shape, seed);
        }

        std::printf("\n");
    }

    return 0;
}
