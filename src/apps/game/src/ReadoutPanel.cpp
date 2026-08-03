#include "antwika/game/ReadoutPanel.hpp"

#include "antwika/game/Workforce.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/TextLayout.hpp>

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

        // Clear of the cursor, so the panel does not sit under it.
        constexpr std::int32_t kPointerOffset = 12;

        // **Captions, and not the names a save file writes.**
        // buildingKindName() and SaveGame's table are a schema.
        // A persisted name may not change to suit a caption.
        // And a caption may not stay ugly because a save froze it.
        // So the two are separate tables.
        // Only one goes through antwika::i18n.
        // A schema is not something a person reads.
        // The array sizes keep either from missing a kind.
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
            return kBuildingLabels[
                buildingKindIndex(kind) % kBuildingKindCount];
        }

        [[nodiscard]] MessageId labelOf(WalkerKind kind) noexcept
        {
            return kWalkerLabels[walkerKindIndex(kind) % kWalkerKindCount];
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

        // A service, said on the stock lines' own scale and format.
        // Ticks of coverage are a number about the simulation.
        // Water and medicine read as amounts a house is holding.
        // Which is the range every other resource line uses.
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

        // Named rather than numbered, and the tier's own caption.
        // "level: 2" would be a number a reader has to look up.
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

        // Against the room the tier has, rather than as a bare count.
        // "people 3" is a number a reader cannot do anything with.
        // Whether a house is full is what decides if the district grows.
        // And the ceiling is the tier's own, which the line above names.
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

        // Idle hands out of everybody living there.
        // What decides whether the house still sends its labourer.
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

        // Who turned up, out of who the kind wants.
        // workedPeriod() is where this same pair becomes a rate.
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

        // A grouped line sits two spaces in under its heading.
        // Presentation rather than translation.
        // So it is said once here, not inside every catalogue string.
        [[nodiscard]] std::string grouped(std::string text)
        {
            return "  " + text;
        } // GCOVR_EXCL_LINE

        // Out of kMaxRisk, which the two risks deliberately share.
        // So one scale serves both lines -- see Building.
        [[nodiscard]] std::string riskText(
            const Translator &translator,
            MessageId caption,
            std::int32_t risk)
        {
            const auto share = std::to_string(risk * 100 / kMaxRisk);
            const std::array<std::string_view, 1> args{share};

            return translator.formatted(caption, args);
        }

        // One line's worth, before anything decides where to put it.
        struct Said
        {
            std::string text;
            Color colour;
        };

        // Every branch left on the four excluded lines is allocation.
        // Two are push_back's throw edge and its growth path.
        // One is the heap branch of a caption far too short to need one.
        // The last is the unwind edge that destroys the temporary.
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

                // Only where somebody lives, since only there is a tier.
                // A well is on HousingLevel::Tent and always will be.
                // Saying so would be saying something untrue about it.
                // The occupancy rides on the very same test as the tier.
                // Both are facts about a household, and a well has none.
                // So one condition answers for the pair of them.
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

                // Staffing is a fact about a workplace alone.
                // The kind's own table says whether it is one.
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

                // On every kind of building, even at zero.
                // The stock lines still follow the bars' rule.
                // Water and medicine are facts about any building.
                // And zero is exactly what a watcher wants warned of.
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = translator.text(
                            MessageId::ReadoutResourcesTitle),
                        .colour = kReadoutTitle});

                // The very rule the bars follow -- see buildingBars().
                if (consumes(building.kind))
                {
                    for (std::size_t slot = 0; slot < kResourceCount;
                         ++slot)
                    {
                        said.push_back(
                            Said{ // GCOVR_EXCL_LINE
                                .text = grouped(amountText(
                                    translator,
                                    kResources[slot],
                                    building.stock[slot],
                                    kStockCapacity)),
                                .colour =
                                    resourceColour(kResources[slot])});
                    }
                }

                // What a carrier and a doctor left behind, as amounts.
                // On the stock lines' own scale and format.
                // A dry house empties, so the water line is always said.
                // And the medicine is what holds the disease off.
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(servedText(
                            translator,
                            MessageId::ServiceWater,
                            building.coverage[
                                serviceIndex(Service::Water)])),
                        .colour = serviceColour(Service::Water)});
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = grouped(servedText(
                            translator,
                            MessageId::ServiceMedicine,
                            building.coverage[
                                serviceIndex(Service::Health)])),
                        .colour = serviceColour(Service::Health)});

                // The three risks, on every building and even at zero.
                // Nothing is a measured fact a watcher wants to read.
                // An unheralded fire is why the section exists.
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
                // One line, and the state rather than what stood there.
                // What a reader can act on is the fire or the debris.
                // The building it was is gone whatever they do.
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

                const auto carries = carriedResource(walker.kind);

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
            // The excluded line is the local vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        // Pinned near the pointer, then pushed back inside the canvas.
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
    } // namespace

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
            // The excluded line's branches are the allocator's again.
            // push_back's throw edge, its growth, and the string's heap.
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
        // The excluded line is the local panel's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
