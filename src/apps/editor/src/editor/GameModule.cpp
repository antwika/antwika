#include "antwika/editor/editor/GameModule.hpp"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <string>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/io/AssetPath.hpp>

#include "antwika/editor/editor/PlayComponents.hpp"
#include "antwika/gameplay/SeamStamp.hpp"
#include "antwika/gameplay/Module.hpp"

#ifdef ANTWIKA_GAME_SHARED
#include <dlfcn.h>
#endif

namespace antwika::editor
{

    namespace
    {
#ifdef ANTWIKA_GAME_SHARED
        constexpr std::string_view kModuleName =
            "libantwika_gameplay_module.so";
#endif
    }

    GameModule::GameModule(
        log::ILogger &logger,
        ecs::World &world,
        const std::set<voxel::VoxelCell> &solidCells,
        const std::vector<std::vector<voxel::VoxelCell>> &patrolCells)
        : logger(&logger),
          world(&world),
          solidCells(&solidCells),
          patrolCells(&patrolCells)
    {
        claimPlayComponents(world);

#ifdef ANTWIKA_GAME_SHARED
        open();
#else
        setUp = &antwikaGameCreate;
        takeDown = &antwikaGameDestroy;
#endif

        madeGame = setUp(&logger, &world, &solidCells, &patrolCells);
    }

    GameModule::~GameModule()
    {
        if (madeGame != nullptr)
        {
            takeDown(madeGame);
        }

#ifdef ANTWIKA_GAME_SHARED
        if (library != nullptr)
        {
            dlclose(library);
        }

        if (!openedPath.empty())
        {
            std::error_code errorCode;
            std::filesystem::remove(openedPath, errorCode);
        }
#endif
    }

#ifdef ANTWIKA_GAME_SHARED
    void *GameModule::opened(
        const std::string &path,
        std::string &why,
        gameplay::IGame *(**setUp)(
            log::ILogger *,
            ecs::World *,
            const std::set<voxel::VoxelCell> *,
            const std::vector<std::vector<voxel::VoxelCell>> *),
        void (**takeDown)(gameplay::IGame *))
    {
        auto *const library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

        if (library == nullptr)
        {
            why = dlerror();

            return nullptr;
        }

        *setUp = reinterpret_cast<std::remove_reference_t<
            decltype(*setUp)>>(dlsym(library, "antwikaGameCreate"));
        *takeDown = reinterpret_cast<std::remove_reference_t<
            decltype(*takeDown)>>(
            dlsym(library, "antwikaGameDestroy"));

        auto *const stampOf = reinterpret_cast<decltype(&antwikaGameStamp)>(
            dlsym(library, "antwikaGameStamp"));

        if (*setUp == nullptr || *takeDown == nullptr
            || stampOf == nullptr)
        {
            dlclose(library);
            why = "it holds no entry points";

            return nullptr;
        }

        if (stampOf() != gameplay::kSeamStamp)
        {
            dlclose(library);
            why = "it was built against other headers than this app";

            return nullptr;
        }

        return library;
    }

    void GameModule::sweep()
    {
        const auto modulePath = std::filesystem::path(
                                io::assetPath(std::string(kModuleName)))
                                .parent_path();

        std::error_code errorCode;

        for (const auto &entry :
             std::filesystem::directory_iterator(modulePath, errorCode))
        {
            const auto name = entry.path().filename().string();

            if (name.starts_with("libantwika_gameplay_module.loaded-"))
            {
                std::filesystem::remove(entry.path(), errorCode);
            }
        }
    }

    std::optional<std::string> GameModule::copied()
    {
        const auto source = io::assetPath(std::string(kModuleName));
        static std::size_t loads = 0;
        const auto target = io::assetPath(
            "libantwika_gameplay_module.loaded-"
            + std::to_string(loads) + ".so");

        std::error_code errorCode;
        loadedAt = std::filesystem::last_write_time(source, errorCode);

        if (errorCode)
        {
            return std::nullopt;
        }

        std::filesystem::copy_file(
            source,
            target,
            std::filesystem::copy_options::overwrite_existing,
            errorCode);

        if (errorCode)
        {
            return std::nullopt;
        }

        ++loads;

        return target;
    }

    void GameModule::open()
    {
        sweep();

        const auto path = copied();
        std::string why = "the module is not beside the app";

        if (path.has_value())
        {
            library = opened(*path, why, &setUp, &takeDown);
            openedPath = *path;
        }

        if (library == nullptr)
        {
            throw std::runtime_error(
                "the game module would not open: "
                + io::assetPath(std::string(kModuleName)) + ": "
                + why);
        }
    }

    bool GameModule::hasChanged() const
    {
        std::error_code errorCode;
        const auto writeTime = std::filesystem::last_write_time(
            io::assetPath(std::string(kModuleName)), errorCode);

        return !errorCode && writeTime != loadedAt;
    }

    bool GameModule::reload()
    {
        const auto path = copied();

        if (!path.has_value())
        {
            return false;
        }

        decltype(setUp) freshSetUp = nullptr;
        decltype(takeDown) freshTakeDown = nullptr;
        std::string why;
        auto *const fresh =
            opened(*path, why, &freshSetUp, &freshTakeDown);

        if (fresh == nullptr)
        {
            logger->log(
                log::Level::Warning,
                "the game module was left as it was: " + why);

            std::error_code errorCode;
            std::filesystem::remove(*path, errorCode);

            return false;
        }

        takeDown(loadedModule);
        dlclose(library);

        std::error_code errorCode;
        std::filesystem::remove(openedPath, errorCode);

        library = fresh;
        openedPath = *path;
        setUp = freshSetUp;
        takeDown = freshTakeDown;
        loadedModule = setUp(logger, world, solids, patrolStops);

        return true;
    }
#endif

    gameplay::IGame *GameModule::operator->() noexcept
    {
        return madeGame;
    }

    const gameplay::IGame *GameModule::operator->() const noexcept
    {
        return madeGame;
    }

}
