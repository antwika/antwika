#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/gameplay/IGame.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::editor
{

    class GameModule final
    {
    public:
        GameModule(
            log::ILogger &logger,
            ecs::World &world,
            const std::set<voxel::VoxelCell> &solidCells,
            const std::vector<std::vector<voxel::VoxelCell>>
                &patrolCells);

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
                const std::set<voxel::VoxelCell> *,
                const std::vector<std::vector<voxel::VoxelCell>> *),
            void (**takeDown)(gameplay::IGame *));

        [[nodiscard]] std::optional<std::string> copied();

        static void sweep();
#endif

        std::filesystem::file_time_type loadedAt{};

        std::string openedPath;

        log::ILogger *logger = nullptr;
        ecs::World *world = nullptr;
        const std::set<voxel::VoxelCell> *solidCells = nullptr;
        const std::vector<std::vector<voxel::VoxelCell>> *patrolCells =
            nullptr;
        void *library = nullptr;
        gameplay::IGame *(*setUp)(
            log::ILogger *,
            ecs::World *,
            const std::set<voxel::VoxelCell> *,
            const std::vector<std::vector<voxel::VoxelCell>> *)
            = nullptr;
        void (*takeDown)(gameplay::IGame *) = nullptr;
        gameplay::IGame *madeGame = nullptr;
    };

}
