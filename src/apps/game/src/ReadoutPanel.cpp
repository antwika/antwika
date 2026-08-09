#include "antwika/game/ReadoutPanel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/game/Workforce.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr std::int32_t kPadding = 4;

        constexpr std::int32_t kPointerOffset = 12;

        constexpr std::array<MessageId, kBuildingKindCount>
            kBuildingLabels{
                MessageId::BuildingHouse,
                MessageId::BuildingFarm,
                MessageId::BuildingClayPit,
                MessageId::BuildingWorkshop,
                MessageId::BuildingStorage,
                MessageId::BuildingMarket,
                MessageId::BuildingWell,
                MessageId::BuildingDoctor,
                MessageId::BuildingFireStation,
                MessageId::BuildingEngineerPost};

        constexpr std::array<MessageId, kWalkerKindCount>
            kWalkerLabels{
                MessageId::WalkerWaterCarrier,
                MessageId::WalkerDoctor,
                MessageId::WalkerFireman,
                MessageId::WalkerEngineer,
                MessageId::WalkerCartPusher,
                MessageId::WalkerMarketBuyer,
                MessageId::WalkerMarketSeller,
                MessageId::WalkerMigrant,
                MessageId::WalkerLabourer};

        constexpr std::array<MessageId, kResourceCount> kResourceLabels{
            MessageId::ResourceFood,
            MessageId::ResourceClay,
            MessageId::ResourcePottery};

        constexpr std::array<MessageId, kHousingLevelCount>
            kHousingLabels{
                MessageId::HousingTent,
                MessageId::HousingShack,
                MessageId::HousingHovel,
                MessageId::HousingCottage};

        [[nodiscard]] MessageId labelOf(BuildingKind kind) noexcept
        {
            return antwika::enums::pick(kBuildingLabels, kind);
        }

        [[nodiscard]] MessageId labelOf(WalkerKind kind) noexcept
        {
            return antwika::enums::pick(kWalkerLabels, kind);
        }

        [[nodiscard]] std::string amountText(
            const Translator &translator,
            Resource resource,
            std::int32_t held,
            std::int32_t capacity)
        {
            const auto named = translator.text(
                kResourceLabels[resourceIndex(resource) % kResourceCount]);
            const auto amount = std::to_string(held);
            const auto most = std::to_string(capacity);
            const std::array<std::string_view, 3> args{named, amount, most};

            return translator.formatted(MessageId::ReadoutAmount, args);
        }

        [[nodiscard]] std::string servedText(
            const Translator &translator,
            MessageId named,
            std::int32_t left)
        {
            const auto name = translator.text(named);
            const auto amount =
                std::to_string(left * kStockCapacity / kCoverageFull);
            const auto most = std::to_string(kStockCapacity);
            const std::array<std::string_view, 3> args{
                name, amount, most};

            return translator.formatted(MessageId::ReadoutAmount, args);
        }

        [[nodiscard]] std::string levelText(
            const Translator &translator, HousingLevel level)
        {
            const auto named = translator.text(
                kHousingLabels[
                    housingLevelIndex(level) % kHousingLevelCount]);
            const std::array<std::string_view, 1> args{named};

            return translator.formatted(
                MessageId::ReadoutLevel, args);
        }

        [[nodiscard]] std::string occupancyText(
            const Translator &translator,
            HousingLevel level,
            std::int32_t living)
        {
            const auto people = std::to_string(living);
            const auto room = std::to_string(populationCapacityOf(level));
            const std::array<std::string_view, 2> args{people, room};

            return translator.formatted(
                MessageId::ReadoutOccupancy, args);
        }

        [[nodiscard]] std::string unemployedText(
            const Translator &translator,
            std::int32_t living,
            std::int32_t employed)
        {
            const auto idle = std::to_string(
                std::max(living - employed, 0));
            const auto everybody = std::to_string(living);
            const std::array<std::string_view, 2> args{idle, everybody};

            return translator.formatted(
                MessageId::ReadoutUnemployed, args);
        }

        [[nodiscard]] std::string staffText(
            const Translator &translator,
            std::int32_t working,
            std::int32_t wanted)
        {
            const auto filled = std::to_string(working);
            const auto asked = std::to_string(wanted);
            const std::array<std::string_view, 2> args{filled, asked};

            return translator.formatted(MessageId::ReadoutStaff, args);
        }

        [[nodiscard]] std::string grouped(std::string text)
        {
            return "  " + text;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::string riskText(
            const Translator &translator,
            MessageId caption,
            std::int32_t risk)
        {
            const auto share = std::to_string(risk * 100 / kMaxRisk);
            const std::array<std::string_view, 1> args{share};

            return translator.formatted(caption, args);
        }

        struct Said final
        {
            std::string text;
            Color colour;
        };

        [[nodiscard]] std::vector<Said> saidBy(
            const HoverReadout &readout, const Translator &translator)
        {
            std::vector<Said> said;

            if (readout.building.has_value())
            {
                const auto &building = *readout.building;
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(labelOf(building.kind)),
                        .colour = kReadoutTitle});

                if (housesPeople(building.kind))
                {
                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text =
                                levelText(translator, building.level),
                            .colour = kReadoutTitle});

                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = occupancyText(
                                translator,
                                building.level,
                                building.population),
                            .colour = kReadoutTitle});

                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = unemployedText(
                                translator,
                                building.population,
                                building.employed),
                            .colour = kReadoutTitle});
                }

                if (workersWantedBy(building.kind) > 0)
                {
                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = staffText(
                                translator,
                                building.employed,
                                workersWantedBy(building.kind)),
                            .colour = kReadoutTitle});
                }

                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(
                            MessageId::ReadoutResourcesTitle),
                        .colour = kReadoutTitle});

                if (consumes(building.kind))
                {
                    const auto shelf =
                        stockCapacityOf(building.level);

                    for (std::size_t slot = 0; slot < kResourceCount;
                         ++slot)
                    {
                        said.push_back(
                            Said{ // GCOVR_EXCL_LINE
                                .text = grouped(amountText(
                                    translator,
                                    kResources[slot],
                                    building.stock[slot],
                                    shelf)),
                                .colour =
                                    resourceColour(kResources[slot])});
                    }
                }

                if (housesPeople(building.kind))
                {
                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = grouped(servedText(
                                translator,
                                MessageId::ServiceWater,
                                building.coverage[
                                    serviceIndex(Service::Water)])),
                            .colour = serviceColour(Service::Water)});
                }

                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(servedText(
                            translator,
                            MessageId::ServiceMedicine,
                            building.coverage[
                                serviceIndex(Service::Health)])),
                        .colour = serviceColour(Service::Health)});

                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(
                            MessageId::ReadoutRiskTitle),
                        .colour = kReadoutTitle});
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(riskText(
                            translator,
                            MessageId::ReadoutFireRisk,
                            building.fireRisk)),
                        .colour = kFireRiskInk});
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(riskText(
                            translator,
                            MessageId::ReadoutCollapseRisk,
                            building.collapseRisk)),
                        .colour = kCollapseRiskInk});
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(riskText(
                            translator,
                            MessageId::ReadoutDiseaseRisk,
                            building.diseaseRisk)),
                        .colour = kDiseaseRiskInk});
            }

            if (readout.ruin.has_value())
            {
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(
                            readout.ruin->state == RuinState::Burning
                                ? MessageId::RuinOnFire
                                : MessageId::RuinDebris),
                        .colour = kReadoutTitle});
            }

            if (readout.walker.has_value())
            {
                const auto &walker = *readout.walker;
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(labelOf(walker.kind)),
                        .colour = kReadoutTitle});

                const auto carries = walker.carrying;

                if (carries.has_value())
                {
                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = amountText(
                                translator,
                                *carries,
                                walker.carried,
                                kWalkerLoad),
                            .colour = resourceColour(*carries)});
                }
            }

            return said;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] Point cornerFor(
            Point anchor, Size panel, Size canvas) noexcept
        {
            const auto width = static_cast<std::int32_t>(panel.width);
            const auto height = static_cast<std::int32_t>(panel.height);

            const auto right = static_cast<std::int32_t>(canvas.width);
            const auto bottom = static_cast<std::int32_t>(canvas.height);

            return Point{
                .x = std::max(
                    0,
                    std::min(anchor.x + kPointerOffset, right - width)),
                .y = std::max(
                    0,
                    std::min(anchor.y + kPointerOffset, bottom - height))};
        }
    }

    ReadoutPanel readoutPanel(
        const HoverReadout &readout,
        Size canvas,
        const Translator &translator)
    {
        const auto said = saidBy(readout, translator);

        if (said.empty())
        {
            return ReadoutPanel{};
        }

        const auto lineHeight =
            antwika::gfx::textSize("x", kReadoutTextScale).height;

        std::uint32_t widest = 0;
        for (const auto &line : said)
        {
            const auto measured =
                antwika::gfx::textSize(line.text, kReadoutTextScale);

            widest = std::max(widest, measured.width);
        }

        const Size panel{
            .width = widest + 2 * static_cast<std::uint32_t>(kPadding),
            .height = static_cast<std::uint32_t>(said.size()) * lineHeight
                + 2 * static_cast<std::uint32_t>(kPadding)};

        const auto corner = cornerFor(readout.anchor, panel, canvas);

        ReadoutPanel laid{
            .box = Rect{.origin = corner, .size = panel}, .lines = {}};
        laid.lines.reserve(said.size());

        for (std::size_t line = 0; line < said.size(); ++line)
        {
            laid.lines.push_back(
                ReadoutLine{ // GCOVR_EXCL_LINE
                    .text = said[line].text,
                    .origin =
                        Point{
                            .x = corner.x + kPadding,
                            .y = corner.y + kPadding
                                + static_cast<std::int32_t>(
                                    line * lineHeight)},
                    .colour = said[line].colour});
        }

        return laid;
    } // GCOVR_EXCL_LINE

}
