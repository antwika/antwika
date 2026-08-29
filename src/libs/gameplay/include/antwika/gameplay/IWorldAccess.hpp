#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>

namespace antwika::gameplay
{

    class IWorldAccess
    {
    public:
        IWorldAccess() = default;

        virtual ~IWorldAccess() = default;

        IWorldAccess(const IWorldAccess &) = delete;
        IWorldAccess(IWorldAccess &&) = delete;

        IWorldAccess &operator=(const IWorldAccess &) = delete;
        IWorldAccess &operator=(IWorldAccess &&) = delete;

        [[nodiscard]] virtual ecs::World &getWorld() noexcept = 0;

        [[nodiscard]] virtual const ecs::World &getWorld()
            const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity getEye() const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity getPlayer() const noexcept = 0;

        virtual void setPlayer(ecs::Entity entity) noexcept = 0;

        /**
         * @brief Stands a walker to play as, where the world's own
         * start pad or the checkpoint reached says it belongs.
         */
        virtual void standPlayer() = 0;

        [[nodiscard]] virtual gfx::Vec3 playerAt() const = 0;
    };

}
