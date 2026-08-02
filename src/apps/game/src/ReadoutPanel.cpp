#include "antwika/game/ReadoutPanel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/TextLayout.hpp>
#include <antwika/i18n/MessageId.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceBar.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr std::int32_t kPadding = 4;

        // Clear of the cursor, so the panel does not sit under it.
        constexpr std::int32_t kPointerOffset = 12;

        using antwika::i18n::MessageId;

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
                MessageId::GameBuildingHouse,
                MessageId::GameBuildingFarm,
                MessageId::GameBuildingClayPit,
                MessageId::GameBuildingWorkshop,
                MessageId::GameBuildingStorage,
                MessageId::GameBuildingMarket,
                MessageId::GameBuildingWell,
                MessageId::GameBuildingDoctor,
                MessageId::GameBuildingFireStation,
                MessageId::GameBuildingEngineerPost};

        constexpr std::array<MessageId, kWalkerKindCount>
            kWalkerLabels{
                MessageId::GameWalkerWaterCarrier,
                MessageId::GameWalkerDoctor,
                MessageId::GameWalkerFireman,
                MessageId::GameWalkerEngineer,
                MessageId::GameWalkerCartPusher,
                MessageId::GameWalkerMarketBuyer,
                MessageId::GameWalkerMarketSeller};

        constexpr std::array<MessageId, kResourceCount> kResourceLabels{
            MessageId::GameResourceFood,
            MessageId::GameResourceClay,
            MessageId::GameResourcePottery};

        constexpr std::array<MessageId, kHousingLevelCount>
            kHousingLabels{
                MessageId::GameHousingTent,
                MessageId::GameHousingShack,
                MessageId::GameHousingHovel,
                MessageId::GameHousingCottage};

        constexpr std::array<MessageId, kServiceCount> kServiceLabels{
            MessageId::GameServiceWater,
            MessageId::GameServiceHealth,
            MessageId::GameServiceSafety,
            MessageId::GameServiceStructure};

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

            return translator.formatted(MessageId::GameReadoutAmount, args);
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
                MessageId::GameReadoutLevel, args);
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
                MessageId::GameReadoutOccupancy, args);
        }

        // Per cent rather than the ticks the component counts.
        // A countdown in ticks is a number about the simulation.
        // What a reader wants is how much of the service is left.
        [[nodiscard]] std::string coverageText(
            const Translator &translator, Service service, std::int32_t left)
        {
            const auto named = translator.text(
                kServiceLabels[serviceIndex(service) % kServiceCount]);
            const auto share =
                std::to_string(left * 100 / kCoverageFull);
            const std::array<std::string_view, 2> args{named, share};

            return translator.formatted(
                MessageId::GameReadoutCoverage, args);
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
                }

                // The very rule the bars follow -- see buildingBars().
                if (consumes(building.kind))
                {
                    for (std::size_t slot = 0; slot < kResourceCount;
                         ++slot)
                    {
                        said.push_back(
                            Said{ // GCOVR_EXCL_LINE
                                .text = amountText(
                                    translator,
                                    kResources[slot],
                                    building.stock[slot],
                                    kStockCapacity),
                                .colour =
                                    resourceColour(kResources[slot])});
                    }
                }

                // Every kind of building, rather than only a house.
                // Risk is a fact about any building at all.
                // And coverage is what holds risk off.
                // A service that has lapsed is simply not listed.
                // The same way a source with no larder lists no stock.
                // Absent and zero say one thing, so one line will do.
                for (const auto service : kServices)
                {
                    const auto left =
                        building.coverage[serviceIndex(service)];

                    if (left <= 0)
                    {
                        continue;
                    }

                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = coverageText(
                                translator, service, left),
                            .colour = serviceColour(service)});
                }
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
