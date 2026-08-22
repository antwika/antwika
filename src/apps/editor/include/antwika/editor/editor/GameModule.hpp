#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/gameplay/IGame.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    class GameModule final
    {
    public:
        GameModule(
            log::ILogger &logger,
            ecs::World &world,
            const voxel::Voxels &solidVoxels,
            const std::vector<std::vector<voxel::VoxelPosition>>
                &patrolPositions);

        ~GameModule();

        GameModule(const GameModule &) = delete;
        GameModule(GameModule &&) = delete;

        GameModule &operator=(const GameModule &) = delete;
        GameModule &operator=(GameModule &&) = delete;

        [[nodiscard]] gameplay::IGame *operator->() noexcept;

        [[nodiscard]] const gameplay::IGame *operator->() const noexcept;

#ifdef ANTWIKA_GAME_SHARED
        [[nodiscard]] bool hasChanged() const;

        [[nodiscard]] bool reload();
#endif

    private:
#ifdef ANTWIKA_GAME_SHARED
        void open();

        [[nodiscard]] static void *opened(
            const std::string &path,
            std::string &why,
            gameplay::IGame *(**setUp)(
                log::ILogger *,
                ecs::World *,
                const voxel::Voxels *,
                const std::vector<std::vector<voxel::VoxelPosition>> *),
            void (**takeDown)(gameplay::IGame *));

        [[nodiscard]] std::optional<std::string> copied();

        static void sweep();

        std::filesystem::file_time_type loadedAt{};

        std::string openedPath;

        log::ILogger *logger = nullptr;
        ecs::World *world = nullptr;
        const voxel::Voxels *solidVoxels = nullptr;
        const std::vector<std::vector<voxel::VoxelPosition>> *patrolPositions =
            nullptr;
        void *library = nullptr;
#endif

        gameplay::IGame *(*setUp)(
            log::ILogger *,
            ecs::World *,
            const voxel::Voxels *,
            const std::vector<std::vector<voxel::VoxelPosition>> *)
            = nullptr;
        void (*takeDown)(gameplay::IGame *) = nullptr;
        gameplay::IGame *madeGame = nullptr;
    };

}
