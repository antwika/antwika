#include "antwika/game/OptionsScene.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        // Wide enough for an action's name and a key's beside it.
        // Wider than the menu's card, since a row carries two words.
        constexpr std::uint32_t kCardWidth = 360;

        // What the line under the rows says.
        // A message per outcome rather than one that stays silent.
        // A refusal nobody is told about looks like a key that failed.
        [[nodiscard]] MessageId noticeMessage(
            std::optional<BindOutcome> outcome) noexcept
        {
            // An if-chain rather than a switch.
            // For HotkeySink's reason exactly.
            // A switch carries an arm no enumerator names.
            if (!outcome.has_value())
            {
                return MessageId::OptionsHint;
            }

            if (*outcome == BindOutcome::Reserved)
            {
                return MessageId::OptionsReserved;
            }

            if (*outcome == BindOutcome::Taken)
            {
                return MessageId::OptionsTaken;
            }

            // Bound and Unchanged say one thing.
            // The key the player reached for is the key it is now.
            return MessageId::OptionsBound;
        }
    } // namespace

    OptionsScene::OptionsScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame OptionsScene::describe(
        Size canvas, Pointer pointer, const OptionsState &state) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .cross = Alignment::Center});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(
                    {.width = fixedSize(kCardWidth), .height = kFit});

                ui.label(translator.text(MessageId::OptionsTitle));

                for (const auto action : kActions)
                {
                    // The key's own persisted name, quoted in.
                    // Rather than translated: it is a format's word.
                    // A name changed to suit a caption breaks a file.
                    std::string bound{
                        antwika::input::toString(
                            state.bindings().keyFor(action))};

                    if (state.awaiting() == action)
                    {
                        bound = translator.text(
                            MessageId::OptionsPress);
                    }

                    const auto name =
                        translator.text(actionLabel(action));
                    const std::array<std::string_view, 2> parts{
                        name, bound};

                    ui.button(
                        translator.formatted(
                            MessageId::OptionsRow, parts),
                        {.id = optionsWidgets::actionWidget(action),
                         .width = kGrow});
                }

                // Always declared, so the card is always one height.
                // Nothing then jumps under a click after a refusal.
                ui.label(
                    translator.text(noticeMessage(state.notice())),
                    ui.theme().muted);

                ui.button(
                    translator.text(MessageId::OptionsBack),
                    {.id = optionsWidgets::kBack, .width = kGrow});
            }

            ui.spacer(kGrow);
        }

        return ui.finish();
    }

} // namespace antwika::game
