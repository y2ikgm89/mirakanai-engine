// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_authoring.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::ui::RuntimeUiAuthoringDiagnostic>& diagnostics,
                                  mirakana::ui::RuntimeUiAuthoringDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

constexpr std::string_view valid_ui_document = R"(GameEngine.UiDocument.v1
element id=root parent= role=root widget=screen localization=ui.main.root accessibility=Main_Menu style=screen
element id=resume parent=root role=button widget=resume_button localization=ui.main.resume accessibility=Resume style=primary_button
element id=volume parent=root role=slider widget=volume_slider localization=ui.settings.volume accessibility=Volume style=slider
widget id=screen kind=menu_stack label=Main_Menu
widget id=resume_button kind=button label=Resume
widget id=volume_slider kind=slider label=Volume
binding id=bind.resume element=resume source=menu.resume target=text
binding id=bind.volume element=volume source=settings.volume target=enabled
)";

constexpr std::string_view valid_ui_theme = R"(GameEngine.UiTheme.v1
color id=primary value=#2f6fed
spacing id=panel_padding value=12
typography id=body value=font.default.16
border id=focus value=2
opacity id=disabled value=0.50
transition id=fade_fast value=120ms
controller_glyph id=accept value=gamepad.south
)";

} // namespace

MK_TEST("runtime ui authoring parses and writes deterministic first-party document and theme files") {
    using namespace mirakana::ui;

    const auto document_parse = parse_runtime_ui_document(valid_ui_document);
    MK_REQUIRE(document_parse.ready);
    MK_REQUIRE(document_parse.document.format_id == "GameEngine.UiDocument.v1");
    MK_REQUIRE(document_parse.document.elements.size() == 3U);
    MK_REQUIRE(document_parse.document.widgets.size() == 3U);
    MK_REQUIRE(document_parse.document.bindings.size() == 2U);
    MK_REQUIRE(document_parse.diagnostics.empty());

    const auto document_round_trip = parse_runtime_ui_document(write_runtime_ui_document(document_parse.document));
    MK_REQUIRE(document_round_trip.ready);
    MK_REQUIRE(document_round_trip.document.elements.size() == document_parse.document.elements.size());
    MK_REQUIRE(document_round_trip.document.widgets.size() == document_parse.document.widgets.size());
    MK_REQUIRE(document_round_trip.document.bindings.size() == document_parse.document.bindings.size());
    MK_REQUIRE(write_runtime_ui_document(document_round_trip.document) ==
               write_runtime_ui_document(document_parse.document));

    const auto theme_parse = parse_runtime_ui_theme(valid_ui_theme);
    MK_REQUIRE(theme_parse.ready);
    MK_REQUIRE(theme_parse.theme.format_id == "GameEngine.UiTheme.v1");
    MK_REQUIRE(theme_parse.theme.tokens.size() == 7U);
    MK_REQUIRE(theme_parse.diagnostics.empty());

    const auto theme_round_trip = parse_runtime_ui_theme(write_runtime_ui_theme(theme_parse.theme));
    MK_REQUIRE(theme_round_trip.ready);
    MK_REQUIRE(theme_round_trip.theme.tokens.size() == theme_parse.theme.tokens.size());
    MK_REQUIRE(write_runtime_ui_theme(theme_round_trip.theme) == write_runtime_ui_theme(theme_parse.theme));
}

MK_TEST("runtime ui authoring fails closed for document identity hierarchy metadata and widget errors") {
    using namespace mirakana::ui;

    const std::string bad_document = R"(GameEngine.UiDocument.v1
element id=root parent= role=root widget=screen localization=ui.main.root accessibility=Main_Menu style=screen
element id=root parent= role=panel widget=screen localization=ui.duplicate accessibility=Duplicate style=panel
element id=orphan parent=missing role=button widget=resume_button localization=ui.orphan accessibility=Orphan style=primary_button
element id=missing_locale parent=root role=button widget=resume_button localization= accessibility=Missing_Locale style=primary_button
element id=missing_accessibility parent=root role=button widget=resume_button localization=ui.missing.accessibility accessibility= style=primary_button
element id=unknown_widget parent=root role=button widget=missing_widget localization=ui.unknown.widget accessibility=Unknown_Widget style=primary_button
widget id=screen kind=menu_stack label=Main_Menu
widget id=resume_button kind=button label=Resume
)";

    const auto parse = parse_runtime_ui_document(bad_document);

    MK_REQUIRE(!parse.ready);
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::duplicate_element_id));
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::unknown_parent_id));
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::missing_localization_key));
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::missing_accessibility_name));
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::unknown_widget_id));
}

MK_TEST("runtime ui authoring rejects unsafe native middleware path and external engine tokens") {
    using namespace mirakana::ui;

    const std::string bad_document = R"(GameEngine.UiDocument.v1
element id=root parent= role=root widget=screen localization=ui.main.root accessibility=Main_Menu style=../themes/unity
widget id=screen kind=menu_stack label=HWND
binding id=bind.resume element=root source=UXML target=text
)";

    const auto parse = parse_runtime_ui_document(bad_document);

    MK_REQUIRE(!parse.ready);
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::unsafe_token));
}

MK_TEST("runtime ui authoring rejects invalid theme tokens") {
    using namespace mirakana::ui;

    const std::string bad_theme = R"(GameEngine.UiTheme.v1
color id=primary value=blue
spacing id=panel_padding value=-4
typography id=body value=../fonts/body
border id=focus value=wide
opacity id=disabled value=1.50
transition id=fade_fast value=fast
controller_glyph id=accept value=UnityEngine.UI
)";

    const auto parse = parse_runtime_ui_theme(bad_theme);

    MK_REQUIRE(!parse.ready);
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::invalid_theme_token));
    MK_REQUIRE(has_diagnostic(parse.diagnostics, RuntimeUiAuthoringDiagnosticCode::unsafe_token));
}

int main() {
    return mirakana::test::run_all();
}
