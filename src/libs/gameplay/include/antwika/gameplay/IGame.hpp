#pragma once

#include "antwika/gameplay/ICameraRig.hpp"
#include "antwika/gameplay/ICheckpointProgress.hpp"
#include "antwika/gameplay/ISimulation.hpp"
#include "antwika/gameplay/IWalkPath.hpp"
#include "antwika/gameplay/IWorldAccess.hpp"
#include "antwika/gameplay/SpawnContext.hpp"

namespace antwika::gameplay
{

    class IGame
        : public IWorldAccess,
          public ISimulation,
          public ICameraRig,
          public IWalkPath,
          public ICheckpointProgress
    {
    public:
        IGame() = default;

        ~IGame() override = default;

        IGame(const IGame &) = delete;
        IGame(IGame &&) = delete;

        IGame &operator=(const IGame &) = delete;
        IGame &operator=(IGame &&) = delete;
    };

}
