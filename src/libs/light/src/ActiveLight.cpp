#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Lamplight.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/light/ActiveLight.hpp>

namespace antwika::light
{

    namespace
    {

        [[nodiscard]] bool appendCapped(
            std::vector<ActiveLight> &lights, const ActiveLight &light)
        {
            if (lights.size() >= kMaxLamps)
            {
                return false;
            }

            lights.push_back(light);
            return true;
        }

        template <typename Component>
        [[nodiscard]] auto hungLightView(const ecs::World &world)
        {
            return world.view<component::Position, Component>();
        }

        template <typename Component>
        void appendHungLights(
            std::vector<ActiveLight> &lights,
            const ecs::World &world,
            const bool castsShadows)
        {
            for (const auto entity : hungLightView<Component>(world))
            {
                const auto stoodPosition =
                    world.get<component::Position>(entity);
                const auto hungLight = world.get<Component>(entity);

                const ActiveLight light{ // GCOVR_EXCL_LINE
                        .position =
                            gfx::Vec3{
                                stoodPosition.x,
                                stoodPosition.y + hungLight.aboveHeight,
                                stoodPosition.z},
                        .tintColor = hungLight.tintColor,
                        .reach = hungLight.reach,
                        .castsShadows = castsShadows};

                if (!appendCapped(lights, light))
                {
                    return;
                }
            }
        }

        void appendFolkAndLamps(
            std::vector<ActiveLight> &lights,
            const std::vector<ActiveLight> &folkLights,
            const std::vector<Lamp> &lamps)
        {
            for (const auto &folkLight : folkLights)
            {
                if (!appendCapped(lights, folkLight))
                {
                    return;
                }
            }

            for (const auto lamp : lamps)
            {
                const ActiveLight lampLight{
                    .position = getLampPosition(lamp),
                    .tintColor = lamp.tintColor,
                    .reach = component::kLampRange};

                if (!appendCapped(lights, lampLight))
                {
                    return;
                }
            }
        }

    }

    std::vector<ActiveLight> getActiveLights(
        const ecs::World &world,
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps)
    {
        std::vector<ActiveLight> lights;

        appendHungLights<component::CarriedLight>(lights, world, true);
        appendHungLights<component::FillLight>(lights, world, false);
        appendFolkAndLamps(lights, folkLights, lamps);

        return lights;
    }

    std::vector<ActiveLight> getActiveLights(
        const ecs::World &world, const std::vector<Lamp> &lamps)
    {
        return getActiveLights(world, {}, lamps);
    }

    std::vector<ActiveLight> getActiveLights(
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps)
    {
        std::vector<ActiveLight> lights;

        appendFolkAndLamps(lights, folkLights, lamps);

        return lights;
    } // GCOVR_EXCL_LINE

    std::vector<ActiveLight> getActiveLights(
        const std::vector<Lamp> &lamps)
    {
        return getActiveLights(std::vector<ActiveLight>{}, lamps);
    }

    std::optional<std::size_t> getCarriedLightSlot(
        const ecs::World &world, const ecs::Entity entity)
    {
        std::size_t lightIndex = 0;

        for (const auto carrier :
             hungLightView<component::CarriedLight>(world))
        {
            if (lightIndex >= kMaxLamps)
            {
                return std::nullopt;
            }

            if (carrier == entity)
            {
                return lightIndex;
            }

            ++lightIndex;
        }

        return std::nullopt;
    }

    std::vector<std::size_t> getDirtyShadowSlots(
        const std::vector<ActiveLight> &bakedLight,
        const std::vector<ActiveLight> &currentLight)
    {
        std::vector<std::size_t> lightIndexes;

        for (std::size_t index = 0; index < currentLight.size(); ++index)
        {
            if (!currentLight.at(index).castsShadows)
            {
                continue;
            }

            if (index < bakedLight.size())
            {
                const auto &was = bakedLight.at(index);
                const auto &currentLamp = currentLight.at(index);
                const auto driftVector = currentLamp.position - was.position;

                if (was.tintColor == currentLamp.tintColor
                    && was.reach == currentLamp.reach
                    && glm::dot(driftVector, driftVector)
                           < kShadowRedrawDistance
                                 * kShadowRedrawDistance)
                {
                    continue;
                }
            }

            lightIndexes.push_back(index);
        }

        return lightIndexes;
    } // GCOVR_EXCL_LINE

}
