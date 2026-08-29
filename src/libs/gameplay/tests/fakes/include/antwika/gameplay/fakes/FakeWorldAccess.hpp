#pragma once

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>

#include <antwika/gameplay/IWorldAccess.hpp>

namespace antwika::gameplay::fakes
{

    class FakeWorldAccess final : public IWorldAccess
    {
    public:
        FakeWorldAccess(
            ecs::World &worldGiven, const ecs::Entity playerEntityGiven) noexcept
            : world(&worldGiven), playerEntity(playerEntityGiven)
        {
        }

        [[nodiscard]] ecs::World &getWorld() noexcept override
        {
            return *world;
        }

        [[nodiscard]] const ecs::World &getWorld() const noexcept override
        {
            return *world;
        }

        [[nodiscard]] ecs::Entity getEye() const noexcept override
        {
            return playerEntity;
        }

        [[nodiscard]] ecs::Entity getPlayer() const noexcept override
        {
            return playerEntity;
        }

        void setPlayer(const ecs::Entity entity) noexcept override
        {
            playerEntity = entity;
        }

        void standPlayer() override
        {
            ++stoodCount;
        }

        [[nodiscard]] gfx::Vec3 playerAt() const override
        {
            return gfx::Vec3{};
        }

        std::size_t stoodCount = 0;

    private:
        ecs::World *world;
        ecs::Entity playerEntity;
    };

}
