#include "antwika/game/ReadoutPanel.hpp"

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
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceBar.hpp"
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
        // The static_asserts keep either from missing a kind.
        constexpr std::array<std::string_view, kBuildingKindCount>
            kBuildingLabels{
                "house",
                "food source",
                "water source",
                "fire station",
                "architect post"};

        constexpr std::array<std::string_view, kWalkerKindCount>
            kWalkerLabels{
                "food walker",
                "water walker",
                "fireman",
                "architect"};

        static_assert(kBuildingLabels.size() == kBuildingKindCount);
        static_assert(kWalkerLabels.size() == kWalkerKindCount);

        [[nodiscard]] std::string_view labelOf(BuildingKind kind) noexcept
        {
            return kBuildingLabels[
                buildingKindIndex(kind) % kBuildingKindCount];
        }

        [[nodiscard]] std::string_view labelOf(WalkerKind kind) noexcept
        {
            return kWalkerLabels[walkerKindIndex(kind) % kWalkerKindCount];
        }

        [[nodiscard]] std::string amountText(
            Resource resource, std::int32_t held, std::int32_t capacity)
        {
            return std::string(resourceName(resource)) + " "
                + std::to_string(held) + "/" + std::to_string(capacity);
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
        [[nodiscard]] std::vector<Said> saidBy(const HoverReadout &readout)
        {
            std::vector<Said> said;

            if (readout.building.has_value())
            {
                const auto &building = *readout.building;
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = std::string(labelOf(building.kind)),
                        .colour = kReadoutTitle});

                // The very rule the bars follow -- see buildingBars().
                if (consumes(building.kind))
                {
                    for (std::size_t slot = 0; slot < kResourceCount;
                         ++slot)
                    {
                        said.push_back(
                            Said{ // GCOVR_EXCL_LINE
                                .text = amountText(
                                    kResources[slot],
                                    building.stock[slot],
                                    kStockCapacity),
                                .colour =
                                    resourceColour(kResources[slot])});
                    }
                }
            }

            if (readout.walker.has_value())
            {
                const auto &walker = *readout.walker;
                said.push_back(
                    Said{ // GCOVR_EXCL_LINE
                        .text = std::string(labelOf(walker.kind)),
                        .colour = kReadoutTitle});

                const auto carries = carriedResource(walker.kind);

                if (carries.has_value())
                {
                    said.push_back(
                        Said{ // GCOVR_EXCL_LINE
                            .text = amountText(
                                *carries, walker.carried, kWalkerLoad),
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

    ReadoutPanel readoutPanel(const HoverReadout &readout, Size canvas)
    {
        const auto said = saidBy(readout);

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
