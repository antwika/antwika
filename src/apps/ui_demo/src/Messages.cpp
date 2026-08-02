#include "antwika/ui_demo/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

#include "antwika/ui_demo/MessageId.hpp"

namespace antwika::ui_demo
{

    namespace
    {

        using i18n::Catalogue;
        using i18n::CatalogueEntry;
        using i18n::Locale;
        using i18n::MessageName;

        constexpr std::array<MessageName<MessageId>, 67> kNames{{
            {MessageId::Title, "Title"},
            {MessageId::PickPage, "PickPage"},
            {MessageId::PageLabels, "PageLabels"},
            {MessageId::PageButtons, "PageButtons"},
            {MessageId::PageLayout, "PageLayout"},
            {MessageId::PageTextField, "PageTextField"},
            {MessageId::PageDropdown, "PageDropdown"},
            {MessageId::PageFocus, "PageFocus"},
            {MessageId::PageTheme, "PageTheme"},
            {MessageId::PageRects, "PageRects"},
            {MessageId::PageShrink, "PageShrink"},
            {MessageId::LabelsLine, "LabelsLine"},
            {MessageId::LabelsMuted, "LabelsMuted"},
            {MessageId::LabelsOwnInk, "LabelsOwnInk"},
            {MessageId::SpacerLeft, "SpacerLeft"},
            {MessageId::SpacerRight, "SpacerRight"},
            {MessageId::ButtonsPress, "ButtonsPress"},
            {MessageId::ButtonCount, "ButtonCount"},
            {MessageId::ButtonReset, "ButtonReset"},
            {MessageId::PressedCount, "PressedCount"},
            {MessageId::ButtonsForced, "ButtonsForced"},
            {MessageId::ButtonIdle, "ButtonIdle"},
            {MessageId::ButtonHovered, "ButtonHovered"},
            {MessageId::ButtonPressed, "ButtonPressed"},
            {MessageId::ButtonUnnamed, "ButtonUnnamed"},
            {MessageId::ButtonsWidths, "ButtonsWidths"},
            {MessageId::ButtonFit, "ButtonFit"},
            {MessageId::ButtonFixed, "ButtonFixed"},
            {MessageId::ButtonGrow, "ButtonGrow"},
            {MessageId::LayoutNest, "LayoutNest"},
            {MessageId::AlignStart, "AlignStart"},
            {MessageId::AlignCenter, "AlignCenter"},
            {MessageId::AlignEnd, "AlignEnd"},
            {MessageId::AcrossAxis, "AcrossAxis"},
            {MessageId::PanelIsColumn, "PanelIsColumn"},
            {MessageId::PanelInset, "PanelInset"},
            {MessageId::FieldOwned, "FieldOwned"},
            {MessageId::FieldPlaceholder, "FieldPlaceholder"},
            {MessageId::FieldKeys, "FieldKeys"},
            {MessageId::FieldHolding, "FieldHolding"},
            {MessageId::ListOpenBit, "ListOpenBit"},
            {MessageId::NoneChosen, "NoneChosen"},
            {MessageId::ListOverlay, "ListOverlay"},
            {MessageId::AccentAmber, "AccentAmber"},
            {MessageId::AccentMint, "AccentMint"},
            {MessageId::AccentRose, "AccentRose"},
            {MessageId::FocusKeys, "FocusKeys"},
            {MessageId::ButtonFirst, "ButtonFirst"},
            {MessageId::ButtonSecond, "ButtonSecond"},
            {MessageId::ButtonThird, "ButtonThird"},
            {MessageId::FocusRingFills, "FocusRingFills"},
            {MessageId::FocusedId, "FocusedId"},
            {MessageId::ThemeColours, "ThemeColours"},
            {MessageId::RectsSays, "RectsSays"},
            {MessageId::RowIsNamed, "RowIsNamed"},
            {MessageId::BarFromRect, "BarFromRect"},
            {MessageId::UndeclaredId, "UndeclaredId"},
            {MessageId::ShrinkProportion, "ShrinkProportion"},
            {MessageId::TooWide, "TooWide"},
            {MessageId::AlsoTooWide, "AlsoTooWide"},
            {MessageId::NoClipping, "NoClipping"},
            {MessageId::LayoutsJob, "LayoutsJob"},
            {MessageId::Showing, "Showing"},
            {MessageId::AccentChosen, "AccentChosen"},
            {MessageId::Submitted, "Submitted"},
            {MessageId::Cancelled, "Cancelled"},
            {MessageId::PressedWidget, "PressedWidget"},
        }};

        static_assert(
            kNames.size() == static_cast<std::size_t>(MessageId::Count),
            "every MessageId must appear in kNames exactly once");

        // Both arrays list every id, in the same order.
        // MessagesTest asserts they cover exactly kNames.
        // That assertion is the point of keying by id.
        // A forgotten Swedish entry is a red build, not a wrong label.
        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kEnglishEntries{{
                {MessageId::Title, "antwika::ui showcase"},
                {MessageId::PickPage, "pick an element"},
                {MessageId::PageLabels, "labels"},
                {MessageId::PageButtons, "buttons"},
                {MessageId::PageLayout, "layout"},
                {MessageId::PageTextField, "text field"},
                {MessageId::PageDropdown, "dropdown"},
                {MessageId::PageFocus, "focus ring"},
                {MessageId::PageTheme, "theme"},
                {MessageId::PageRects, "widget rects"},
                {MessageId::PageShrink, "shrink"},
                {MessageId::LabelsLine,
                 "label() draws one line in the theme's colour"},
                {MessageId::LabelsMuted, "a muted line reads as an aside"},
                {MessageId::LabelsOwnInk, "and a caller may hand it its own"},
                {MessageId::SpacerLeft, "a growing spacer"},
                {MessageId::SpacerRight, "pushes these apart"},
                {MessageId::ButtonsPress,
                 "a button activates on the press, not a release"},
                {MessageId::ButtonCount, "count"},
                {MessageId::ButtonReset, "reset"},
                {MessageId::PressedCount, "pressed {0}"},
                {MessageId::ButtonsForced,
                 "an appearance can be forced by the caller"},
                {MessageId::ButtonIdle, "idle"},
                {MessageId::ButtonHovered, "hovered"},
                {MessageId::ButtonPressed, "pressed"},
                {MessageId::ButtonUnnamed, "unnamed"},
                {MessageId::ButtonsWidths, "and a width is fit, fixed or grow"},
                {MessageId::ButtonFit, "fit"},
                {MessageId::ButtonFixed, "fixed"},
                {MessageId::ButtonGrow, "grow"},
                {MessageId::LayoutNest,
                 "row, column and panel nest as deep as you like"},
                {MessageId::AlignStart, "start"},
                {MessageId::AlignCenter, "center"},
                {MessageId::AlignEnd, "end"},
                {MessageId::AcrossAxis, "across the axis"},
                {MessageId::PanelIsColumn,
                 "a panel is a column with the theme's fill"},
                {MessageId::PanelInset,
                 "and the theme's inset, and nothing else"},
                {MessageId::FieldOwned,
                 "the characters belong to the application"},
                {MessageId::FieldPlaceholder, "tab here and type"},
                {MessageId::FieldKeys,
                 "Enter submits it, Escape gives up on it"},
                {MessageId::FieldHolding, "holding: {0}"},
                {MessageId::ListOpenBit,
                 "whether a list is open is the caller's bit"},
                {MessageId::NoneChosen, "none chosen"},
                {MessageId::ListOverlay,
                 "an open list is an overlay, hit first"},
                {MessageId::AccentAmber, "amber"},
                {MessageId::AccentMint, "mint"},
                {MessageId::AccentRose, "rose"},
                {MessageId::FocusKeys,
                 "Tab, Shift+Tab and Enter walk these three"},
                {MessageId::ButtonFirst, "first"},
                {MessageId::ButtonSecond, "second"},
                {MessageId::ButtonThird, "third"},
                {MessageId::FocusRingFills,
                 "the ring is four fills, drawn after everything"},
                {MessageId::FocusedId, "focused id {0}"},
                {MessageId::ThemeColours,
                 "every colour a widget picks without being told"},
                {MessageId::RectsSays,
                 "Frame::rects says where a named widget went"},
                {MessageId::RowIsNamed, "this row is named"},
                {MessageId::BarFromRect, "the bar is placed from its rect"},
                {MessageId::UndeclaredId,
                 "an id no frame declared answers nothing at all"},
                {MessageId::ShrinkProportion,
                 "too little room shrinks children in proportion"},
                {MessageId::TooWide, "far too wide"},
                {MessageId::AlsoTooWide, "also too wide"},
                {MessageId::NoClipping,
                 "there is no clipping, so containment is the"},
                {MessageId::LayoutsJob,
                 "layout's job rather than a renderer's"},
                {MessageId::Showing, "showing {0}"},
                {MessageId::AccentChosen, "accent {0} chosen"},
                {MessageId::Submitted, "submitted: {0}"},
                {MessageId::Cancelled, "cancelled"},
                {MessageId::PressedWidget, "pressed widget {0}"},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::Title, "antwika::ui-uppvisning"},
                {MessageId::PickPage, "välj ett element"},
                {MessageId::PageLabels, "etiketter"},
                {MessageId::PageButtons, "knappar"},
                {MessageId::PageLayout, "layout"},
                {MessageId::PageTextField, "textfält"},
                {MessageId::PageDropdown, "rullgardin"},
                {MessageId::PageFocus, "fokusring"},
                {MessageId::PageTheme, "tema"},
                {MessageId::PageRects, "widgetrutor"},
                {MessageId::PageShrink, "krympning"},
                {MessageId::LabelsLine, "label() ritar en rad i temats färg"},
                {MessageId::LabelsMuted, "en dämpad rad läses som en parentes"},
                {MessageId::LabelsOwnInk, "och anroparen får ge den en egen"},
                {MessageId::SpacerLeft, "en växande utfyllnad"},
                {MessageId::SpacerRight, "skjuter isär de här"},
                {MessageId::ButtonsPress,
                 "en knapp utlöses vid trycket, inte vid släppet"},
                {MessageId::ButtonCount, "räkna"},
                {MessageId::ButtonReset, "nollställ"},
                {MessageId::PressedCount, "tryckt {0}"},
                {MessageId::ButtonsForced,
                 "anroparen kan tvinga fram ett utseende"},
                {MessageId::ButtonIdle, "vilande"},
                {MessageId::ButtonHovered, "under pekaren"},
                {MessageId::ButtonPressed, "nedtryckt"},
                {MessageId::ButtonUnnamed, "namnlös"},
                {MessageId::ButtonsWidths,
                 "och en bredd är anpassad, fast eller växande"},
                {MessageId::ButtonFit, "anpassad"},
                {MessageId::ButtonFixed, "fast"},
                {MessageId::ButtonGrow, "växande"},
                {MessageId::LayoutNest,
                 "rad, kolumn och panel nästlas hur djupt som helst"},
                {MessageId::AlignStart, "början"},
                {MessageId::AlignCenter, "mitten"},
                {MessageId::AlignEnd, "slutet"},
                {MessageId::AcrossAxis, "tvärs axeln"},
                {MessageId::PanelIsColumn,
                 "en panel är en kolumn med temats fyllning"},
                {MessageId::PanelInset, "och temats indrag, och inget annat"},
                {MessageId::FieldOwned, "tecknen tillhör programmet"},
                {MessageId::FieldPlaceholder, "tabba hit och skriv"},
                {MessageId::FieldKeys, "Enter skickar in, Escape ger upp"},
                {MessageId::FieldHolding, "innehåller: {0}"},
                {MessageId::ListOpenBit,
                 "om en lista är öppen är anroparens bit"},
                {MessageId::NoneChosen, "inget valt"},
                {MessageId::ListOverlay,
                 "en öppen lista ritas överst och träffas först"},
                {MessageId::AccentAmber, "bärnsten"},
                {MessageId::AccentMint, "mynta"},
                {MessageId::AccentRose, "ros"},
                {MessageId::FocusKeys,
                 "Tab, Shift+Tab och Enter vandrar mellan tre"},
                {MessageId::ButtonFirst, "första"},
                {MessageId::ButtonSecond, "andra"},
                {MessageId::ButtonThird, "tredje"},
                {MessageId::FocusRingFills,
                 "ringen är fyra fyllningar, ritade sist av allt"},
                {MessageId::FocusedId, "fokuserat id {0}"},
                {MessageId::ThemeColours,
                 "varje färg en widget väljer utan att bli tillsagd"},
                {MessageId::RectsSays,
                 "Frame::rects säger var en namngiven widget hamnade"},
                {MessageId::RowIsNamed, "den här raden är namngiven"},
                {MessageId::BarFromRect, "listen placeras utifrån dess ruta"},
                {MessageId::UndeclaredId,
                 "ett id ingen ram deklarerat svarar inte alls"},
                {MessageId::ShrinkProportion,
                 "för lite plats krymper barnen proportionellt"},
                {MessageId::TooWide, "alldeles för bred"},
                {MessageId::AlsoTooWide, "också för bred"},
                {MessageId::NoClipping,
                 "det finns ingen beskärning, så inneslutning"},
                {MessageId::LayoutsJob,
                 "är layoutens sak snarare än renderarens"},
                {MessageId::Showing, "visar {0}"},
                {MessageId::AccentChosen, "accent {0} vald"},
                {MessageId::Submitted, "inskickat: {0}"},
                {MessageId::Cancelled, "avbrutet"},
                {MessageId::PressedWidget, "tryckte på widget {0}"},
            }};

        constexpr Catalogue<MessageId> kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue<MessageId> kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::span<const i18n::MessageName<MessageId>>
        Messages::names() noexcept
    {
        return kNames;
    }

    const i18n::Catalogue<MessageId> &Messages::catalogueFor(
        i18n::Locale locale) noexcept
    {
        switch (locale)
        {
        case Locale::English:
            return kEnglishCatalogue;
        case Locale::Swedish:
            return kSwedishCatalogue;
        }

        return catalogueFor(i18n::kDefaultLocale);
    }

} // namespace antwika::ui_demo
