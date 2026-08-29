#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/gameplay/IGame.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    class GameModule final
    {
    public:
        GameModule(
            log::ILogger &logger,
            ecs::World &world,
            const map::Map &laidMap,
            const voxel::Voxels &solidVoxels,
            const std::vector<std::vector<voxel::VoxelPosition>>
                &patrolPositions);

        ~GameModule();

        GameModule(const GameModule &) = delete;
        GameModule(GameModule &&) = delete;

        GameModule &operator=(const GameModule &) = delete;
        GameModule &operator=(GameModule &&) = delete;

        [[nodiscard]] gameplay::IGame *operator->();

        [[nodiscard]] const gameplay::IGame *operator->() const;

        [[nodiscard]] gameplay::IGame &operator*();

        [[nodiscard]] const gameplay::IGame &operator*() const;

#ifdef ANTWIKA_GAME_SHARED
        [[nodiscard]] bool hasChanged() const;

        [[nodiscard]] bool reload();
#endif

    private:
        using GameSetUp = gameplay::IGame *(*)(
            log::ILogger *,
            ecs::World *,
            const map::Map *,
            const voxel::Voxels *,
            const std::vector<std::vector<voxel::VoxelPosition>> *);

        using GameTakeDown = void (*)(gameplay::IGame *);

        void letGo() noexcept;

        void createGame();

#ifdef ANTWIKA_GAME_SHARED
        struct ModuleEntry final
        {
            void *library = nullptr;

            GameSetUp setUp = nullptr;

            GameTakeDown takeDown = nullptr;
        };

        struct OpenedModule final
        {
            std::optional<ModuleEntry> entry;

            std::string why;
        };

        void open();

        [[nodiscard]] static OpenedModule openedModuleAt(const std::string &path);

        [[nodiscard]] std::optional<std::string> copied();

        static void sweep();

        std::filesystem::file_time_type loadedAt{};

        std::string openedPath;

        void *library = nullptr;
#endif

        log::ILogger *logger = nullptr;
        ecs::World *world = nullptr;
        const map::Map *laidMap = nullptr;
        const voxel::Voxels *solidVoxels = nullptr;
        const std::vector<std::vector<voxel::VoxelPosition>> *patrolPositions =
            nullptr;

        GameSetUp setUp = nullptr;
        GameTakeDown takeDown = nullptr;
        gameplay::IGame *madeGame = nullptr;
    };

}
