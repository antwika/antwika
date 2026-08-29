#include "antwika/editor/editor/GameModule.hpp"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/io/AssetPath.hpp>

#include "antwika/gameplay/ComponentNames.hpp"
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

        [[nodiscard]] gameplay::IGame *heldGameOf(
            gameplay::IGame *const madeGame)
        {
            if (madeGame == nullptr)
            {
                // GCOVR_EXCL_START
                throw std::runtime_error(
                    "the game module holds no game");
                // GCOVR_EXCL_STOP
            }

            return madeGame;
        }
    }

    GameModule::GameModule(
        log::ILogger &logger,
        ecs::World &world,
        const map::Map &laidMap,
        const voxel::Voxels &solidVoxels,
        const std::vector<std::vector<voxel::VoxelPosition>> &patrolPositions)
        : logger(&logger),
          world(&world),
          laidMap(&laidMap),
          solidVoxels(&solidVoxels),
          patrolPositions(&patrolPositions)
    {
        gameplay::claimModuleComponents(world);

#ifdef ANTWIKA_GAME_SHARED
        open();
#else
        setUp = &antwikaGameCreate;
        takeDown = &antwikaGameDestroy;
#endif

        createGame();
    }

    GameModule::~GameModule()
    {
        letGo();
    }

    void GameModule::letGo() noexcept
    {
        if (madeGame != nullptr)
        {
            takeDown(madeGame);
            madeGame = nullptr;
        }

#ifdef ANTWIKA_GAME_SHARED
        if (library != nullptr)
        {
            dlclose(library);
            library = nullptr;
        }

        if (!openedPath.empty())
        {
            std::error_code errorCode;
            std::filesystem::remove(openedPath, errorCode);
            openedPath.clear();
        }
#endif
    }

    void GameModule::createGame()
    {
        try
        {
            madeGame = setUp(
                logger, world, laidMap, solidVoxels, patrolPositions);
        }
        catch (...)
        {
            letGo();

            throw;
        }
    }

#ifdef ANTWIKA_GAME_SHARED
    GameModule::OpenedModule GameModule::openedModuleAt(const std::string &path)
    {
        auto *const library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

        if (library == nullptr)
        {
            return OpenedModule{.entry = std::nullopt, .why = dlerror()};
        }

        ModuleEntry entry{
            .library = library,
            .setUp = reinterpret_cast<GameSetUp>(
                dlsym(library, "antwikaGameCreate")),
            .takeDown = reinterpret_cast<GameTakeDown>(
                dlsym(library, "antwikaGameDestroy"))};

        auto *const stampOf = reinterpret_cast<decltype(&antwikaGameStamp)>(
            dlsym(library, "antwikaGameStamp"));

        if (entry.setUp == nullptr || entry.takeDown == nullptr
            || stampOf == nullptr)
        {
            dlclose(library);

            return OpenedModule{
                .entry = std::nullopt,
                .why = "it holds no entry points"};
        }

        if (stampOf() != gameplay::kSeamStamp)
        {
            dlclose(library);

            return OpenedModule{
                .entry = std::nullopt,
                .why = "it was built against other headers than this app"};
        }

        return OpenedModule{.entry = entry, .why = {}};
    }

    void GameModule::sweep()
    {
        const auto modulePath = std::filesystem::path(
                                io::getAssetPath(std::string(kModuleName)))
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
        static std::atomic<std::size_t> loadCount{0};

        const auto source = io::getAssetPath(std::string(kModuleName));
        const auto target = io::getAssetPath(
            "libantwika_gameplay_module.loaded-"
            + std::to_string(loadCount.fetch_add(1)) + ".so");

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

        return target;
    }

    void GameModule::open()
    {
        sweep();

        const auto path = copied();
        auto openedModule = OpenedModule{
            .entry = std::nullopt,
            .why = "the module is not beside the app"};

        if (path.has_value())
        {
            openedModule = openedModuleAt(*path);

            if (openedModule.entry.has_value())
            {
                library = openedModule.entry->library;
                setUp = openedModule.entry->setUp;
                takeDown = openedModule.entry->takeDown;
                openedPath = *path;
            }
            else
            {
                std::error_code errorCode;
                std::filesystem::remove(*path, errorCode);
            }
        }

        if (library == nullptr)
        {
            throw std::runtime_error(
                "the game module would not open: "
                + io::getAssetPath(std::string(kModuleName)) + ": "
                + openedModule.why);
        }
    }

    bool GameModule::hasChanged() const
    {
        std::error_code errorCode;
        const auto writeTime = std::filesystem::last_write_time(
            io::getAssetPath(std::string(kModuleName)), errorCode);

        return !errorCode && writeTime != loadedAt;
    }

    bool GameModule::reload()
    {
        const auto path = copied();

        if (!path.has_value())
        {
            return false;
        }

        const auto freshModule = openedModuleAt(*path);

        if (!freshModule.entry.has_value())
        {
            logger->log(
                log::Level::Warning,
                "the game module was left as it was: " + freshModule.why);

            std::error_code errorCode;
            std::filesystem::remove(*path, errorCode);

            return false;
        }

        takeDown(madeGame);
        madeGame = nullptr;

        dlclose(library);

        std::error_code errorCode;
        std::filesystem::remove(openedPath, errorCode);

        library = freshModule.entry->library;
        openedPath = *path;
        setUp = freshModule.entry->setUp;
        takeDown = freshModule.entry->takeDown;

        createGame();

        return true;
    }
#endif

    gameplay::IGame *GameModule::operator->()
    {
        return heldGameOf(madeGame);
    }

    const gameplay::IGame *GameModule::operator->() const
    {
        return heldGameOf(madeGame);
    }

    gameplay::IGame &GameModule::operator*()
    {
        return *heldGameOf(madeGame);
    }

    const gameplay::IGame &GameModule::operator*() const
    {
        return *heldGameOf(madeGame);
    }

}
