#include "antwika/editor/editor/GameModule.hpp"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <string>

#include <antwika/voxel/VoxelPosition.hpp>
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
        const voxel::Voxels &solidVoxels,
        const std::vector<std::vector<voxel::VoxelPosition>> &patrolPositions)
#ifdef ANTWIKA_GAME_SHARED
        : logger(&logger),
          world(&world),
          solidVoxels(&solidVoxels),
          patrolPositions(&patrolPositions)
#endif
    {
        claimPlayComponents(world);

#ifdef ANTWIKA_GAME_SHARED
        open();
#else
        setUp = &antwikaGameCreate;
        takeDown = &antwikaGameDestroy;
#endif

        try
        {
            madeGame =
                setUp(&logger, &world, &solidVoxels, &patrolPositions);
        }
        catch (...)
        {
            letGo();

            throw;
        }
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

#ifdef ANTWIKA_GAME_SHARED
    void *GameModule::opened(
        const std::string &path,
        std::string &why,
        gameplay::IGame *(**setUp)(
            log::ILogger *,
            ecs::World *,
            const voxel::Voxels *,
            const std::vector<std::vector<voxel::VoxelPosition>> *),
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
        const auto source = io::getAssetPath(std::string(kModuleName));
        static std::size_t loads = 0;
        const auto target = io::getAssetPath(
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

            if (library != nullptr)
            {
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
                + why);
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

        takeDown(madeGame);
        madeGame = nullptr;

        dlclose(library);
        library = nullptr;

        std::error_code errorCode;
        std::filesystem::remove(openedPath, errorCode);

        library = fresh;
        openedPath = *path;
        setUp = freshSetUp;
        takeDown = freshTakeDown;
        madeGame = setUp(logger, world, solidVoxels, patrolPositions);

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
