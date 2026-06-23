# First-Party UI Clean-Room Source Ledger v1

This ledger is an engineering compliance record, not legal advice. Its purpose is to keep MIRAIKANAI runtime UI and editor UI work first-party while allowing official documentation to inform only broad functional categories.

## Status

Active clean-room guard for the candidate First-Party Runtime UI And Editor Platform Production v1 milestone Task 1. This ledger adds provenance and static enforcement only; it does not add a dependency, external asset, compatibility layer, importer, visual/API parity claim, or production runtime UI platform readiness claim.

## Allowed Sources

| Source class | Allowed use | Forbidden use | Evidence |
| --- | --- | --- | --- |
| Unity official category documentation | Identify common runtime UI, editor UI, HUD, styling, layout, and input categories. | Copy Unity source, samples, serialized documents, UI Builder layouts, public type names, visual style, themes, fonts, icons, screenshots, or API shape. | Context7 `/websites/unity3d_6000_0_manual`; official docs at `https://docs.unity3d.com/`. |
| Unreal Engine official category documentation | Identify common HUD, widget, designer, input routing, and accessibility categories. | Copy Unreal source, samples, Blueprint/UMG/Slate structure, public type names, visual style, themes, fonts, icons, screenshots, or API shape. | Context7 `/websites/dev_epicgames_unreal-engine`; official docs at `https://dev.epicgames.com/documentation/unreal-engine/`. |
| Godot official category documentation | Identify common Control-node, container, text entry, focus, accessibility, and HUD overlay categories. | Copy Godot source, scene/control trees, docs examples, default themes, public type names, visual style, fonts, icons, screenshots, or API shape. | Context7 `/godotengine/godot-docs`; official docs at `https://docs.godotengine.org/`. |
| Microsoft SDK documentation | Design private Windows adapter boundaries for DirectWrite, TSF, UI Automation, and D3D12 upload proof. | Expose Win32, COM, DirectWrite, TSF, UIA, D3D12, or DXGI types through public `mirakana::ui`, generated-game, or `MK_editor_core` APIs. | Microsoft Learn pages referenced by the platform production plan. |
| Apple SDK documentation | Plan future private Core Text, InputMethodKit, NSAccessibility, Metal, and mobile adapter boundaries. | Claim Apple readiness without Apple-host evidence or expose Apple SDK types through public runtime/game/editor-core APIs. | Apple Developer documentation referenced by the platform production plan. |
| HarfBuzz documentation | Understand text shaping concepts for future audited dependency adapters. | Vendor or link HarfBuzz without dependency/legal/notice/bootstrap records, or copy sample code/API shapes into public contracts. | `https://harfbuzz.github.io/what-is-harfbuzz.html`. |
| FreeType documentation | Understand glyph rasterization concepts for future audited dependency adapters. | Vendor or link FreeType without dependency/legal/notice/bootstrap records, or copy sample code/API shapes into public contracts. | `https://freetype.org/freetype2/docs/index.html`. |
| AT-SPI2 documentation | Plan future private Linux accessibility bridge boundaries. | Claim Linux accessibility readiness without host evidence or expose AT-SPI types through public runtime/game/editor-core APIs. | `https://www.freedesktop.org/wiki/Accessibility/AT-SPI2/`. |
| Vulkan documentation | Plan future private renderer upload proof boundaries. | Claim Vulkan UI atlas upload readiness without backend execution evidence or expose native Vulkan types through gameplay UI APIs. | `https://docs.vulkan.org/`. |
| W3C accessibility practices | Identify accessible-name, role, state, relationship, focus, and keyboard-operation categories. | Claim WAI-ARIA, browser, or full screen-reader parity from semantic rows alone. | `https://www.w3.org/WAI/ARIA/apg/`. |
| Repository-owned code and docs | Implement first-party contracts, validators, tests, and sample counters. | Reintroduce removed middleware, copied external public UI names, imported external serialized UI formats, or compatibility/parity claims. | Tracked repository files plus focused validation output. |

## Forbidden Inputs

| Forbidden input | Reason | Static or review gate |
| --- | --- | --- |
| External source from Unity, Unreal Engine, Godot, UI middleware, blogs, books, Stack Overflow, GitHub snippets, or license-less repositories | Source copying creates licensing and originality risk. | Code review and license-audit; do not paste or translate implementation code. |
| Documentation examples copied as implementation | Official docs examples are explanatory material, not first-party implementation input. | Review evidence must state category-only use. |
| Sample assets, marketplace assets, asset-store content, starter templates, or demo projects | Asset licenses and visual identity are not clean-room implementation evidence. | `THIRD_PARTY_NOTICES.md` and legal review required before any external asset distribution. |
| Default themes, fonts, icons, screenshots, editor chrome, UI builder layouts, widget designer layouts, or scene/control trees | Visual/API similarity and asset copying are out of scope. | Source ledger review and package smoke counters must remain first-party. |
| Unity UXML/USS imports, Unreal UMG/Widget Blueprint/Slate imports, or Godot scene/control tree imports | Serialized import parity is an unsupported compatibility claim. | Public names and importer claims are rejected by static checks and plan gates. |
| Visual parity, API parity, workflow parity, replacement-equivalence, or compatibility claims for Unity, Unreal Engine, Godot, or UI middleware | The milestone is first-party functionality, not an external-engine clone or compatibility layer. | Docs, package counters, plan checkboxes, and static guards must preserve explicit non-claims. |

## Forbidden Public Names

The following tokens are forbidden in public runtime UI, generated-game, and editor-core API surfaces unless a future legal/architecture decision explicitly changes this ledger:

```text
UXML
USS
UMG
Slate
WidgetBlueprint
UnityEngine
UnityEditor
Godot
CanvasLayer
ControlNode
UnrealEd
BlueprintGraph
DearImGui
ImGui
RmlUi
Noesis
Slint
Qt
```

`tools/check-first-party-ui-clean-room.ps1` scans engine public headers, generated/game code and manifests, and `editor/core/include` for these tokens. Documentation, legal records, and this source ledger may mention the tokens only as provenance, non-claim, or prohibition text.

## Dependency Entry Gate

No new dependency enters this milestone through this ledger. Official documentation references are not repository dependencies and do not require notices by themselves.

Any future HarfBuzz, FreeType, Fontconfig, ICU, AT-SPI, UI middleware, font, icon, image, or platform redistributable selection must land in a separate dependency/legal slice before implementation can claim dependency-ready:

- update `vcpkg.json` behind a non-default feature when applicable;
- update `docs/dependencies.md`;
- update `docs/legal-and-licensing.md`;
- update `THIRD_PARTY_NOTICES.md`;
- update bootstrap and dependency policy checks if package-manager state changes;
- keep third-party headers, object models, native handles, and serialized formats out of public gameplay/runtime UI APIs.

## Trademark And Marketing Non-Claims

Unity, Unreal Engine, Godot, Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, HarfBuzz, FreeType, Microsoft, Apple, Vulkan, and W3C names may appear in docs/legal/provenance discussion only when identifying sources, host gates, or unsupported claims.

Product-facing feature names, sample output, package counters, public C++ identifiers, editor panel ids, schema ids, and marketing text must use first-party MIRAIKANAI terminology. The project must not claim compatibility, parity, equivalence, replacement status, importer support, or look-and-feel similarity with those products unless a future plan, legal review, and validation evidence explicitly authorize that narrower claim.

## Review Evidence

| Date | Evidence | Result |
| --- | --- | --- |
| 2026-06-24 | Context7 queries against `/websites/unity3d_6000_0_manual`, `/websites/dev_epicgames_unreal-engine`, and `/godotengine/godot-docs` for runtime UI, editor UI, HUD, widget, focus, and accessibility categories. | Category-only reference confirmed. Returned snippets/API details remain forbidden implementation input. |
| 2026-06-24 | `docs/superpowers/plans/2026-06-24-first-party-runtime-ui-and-editor-platform-production-v1.md` clean-room and non-claim rules reviewed for Task 1. | Task 1 implements a ledger and static guard before runtime/editor code changes. |
| 2026-06-24 | `tools/check-first-party-ui-clean-room.ps1` added as the first static clean-room guard. | Public/API surfaces fail closed on forbidden external engine or UI middleware public names. |
