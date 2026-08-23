#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Lamplight.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/light/ActiveLight.hpp>

namespace antwika::light
{

    std::vector<ActiveLight> getActiveLights(
        const ecs::World &world,
        const std::vector<ActiveLight> &folkLights,
        const std::vector<Lamp> &lamps)
    {
        std::vector<ActiveLight> lights;

        for (const auto entity :
             world.view<component::Position, component::CarriedLight>())
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            const auto stoodPosition = world.get<component::Position>(entity);
            const auto carriedLight =
                world.get<component::CarriedLight>(entity);

            lights.push_back(
                ActiveLight{ // GCOVR_EXCL_LINE
                        .position =
                            gfx::Vec3{
                                stoodPosition.x,
                                stoodPosition.y + carriedLight.aboveHeight,
                                stoodPosition.z},
                        .tintColor = carriedLight.tintColor,
                        .reach = carriedLight.reach});
        }

        for (const auto entity :
             world.view<component::Position, component::FillLight>())
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            const auto stoodPosition = world.get<component::Position>(entity);
            const auto fillLight = world.get<component::FillLight>(entity);

            lights.push_back(
                ActiveLight{ // GCOVR_EXCL_LINE
                        .position =
                            gfx::Vec3{
                                stoodPosition.x,
                                stoodPosition.y + fillLight.aboveHeight,
                                stoodPosition.z},
                        .tintColor = fillLight.tintColor,
                        .reach = fillLight.reach,
                        .castsShadows = false});
        }

        for (const auto &light : folkLights)
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            lights.push_back(light);
        }

        for (const auto lamp : lamps)
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            lights.push_back(
                ActiveLight{
                    .position = getLampPosition(lamp),
                    .tintColor = lamp.tintColor,
                    .reach = component::kLampRange});
        }

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

        for (const auto &light : folkLights)
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            lights.push_back(light);
        }

        for (const auto lamp : lamps)
        {
            if (lights.size() >= kMaxLamps)
            {
                return lights;
            }

            lights.push_back(
                ActiveLight{
                    .position = getLampPosition(lamp),
                    .tintColor = lamp.tintColor,
                    .reach = component::kLampRange});
        }

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
             world.view<component::Position, component::CarriedLight>())
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
