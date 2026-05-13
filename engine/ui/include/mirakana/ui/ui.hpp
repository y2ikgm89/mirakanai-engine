// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

struct ElementId {
    std::string value;

    friend bool operator==(const ElementId& lhs, const ElementId& rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

[[nodiscard]] bool empty(const ElementId& id) noexcept;

struct Rect {
    float x{0.0F};
    float y{0.0F};
    float width{0.0F};
    float height{0.0F};

    friend bool operator==(const Rect& lhs, const Rect& rhs) noexcept = default;
};

struct Size {
    float width{0.0F};
    float height{0.0F};
};

struct EdgeInsets {
    float top{0.0F};
    float right{0.0F};
    float bottom{0.0F};
    float left{0.0F};
};

struct SizeConstraints {
    float min_width{0.0F};
    float min_height{0.0F};
    float max_width{0.0F};
    float max_height{0.0F};
};

enum class SemanticRole : std::uint8_t {
    none,
    root,
    panel,
    button,
    label,
    text_field,
    list,
    list_item,
    image,
    checkbox,
    slider,
    dialog,
};

enum class LayoutMode : std::uint8_t {
    none,
    row,
    column,
    stack,
};

enum class AnchorMode : std::uint8_t {
    top_left,
    top_right,
    bottom_left,
    bottom_right,
    center,
    fill,
};

enum class TextDirection : std::uint8_t {
    automatic,
    left_to_right,
    right_to_left,
};

enum class TextWrapMode : std::uint8_t {
    clip,
    ellipsis,
    wrap,
};

struct TextContent {
    std::string label;
    std::string localization_key;
    std::string font_family;
    TextDirection direction{TextDirection::automatic};
    TextWrapMode wrap{TextWrapMode::clip};
};

struct ImageContent {
    std::string resource_id;
    std::string asset_uri;
    std::string tint_token;
};

struct Style {
    LayoutMode layout{LayoutMode::none};
    AnchorMode anchor{AnchorMode::top_left};
    EdgeInsets margin;
    EdgeInsets padding;
    float gap{0.0F};
    SizeConstraints size;
    float dpi_scale{1.0F};
    std::string background_token;
    std::string foreground_token;
};

struct ElementDesc {
    ElementId id;
    ElementId parent;
    SemanticRole role{SemanticRole::none};
    Rect bounds;
    bool visible{true};
    bool enabled{true};
    TextContent text;
    ImageContent image;
    std::string accessibility_label;
    Style style;
};

struct Element {
    ElementId id;
    ElementId parent;
    SemanticRole role{SemanticRole::none};
    Rect bounds;
    bool visible{true};
    bool enabled{true};
    TextContent text;
    ImageContent image;
    std::string accessibility_label;
    Style style;
    std::vector<ElementId> children;
};

class UiDocument {
  public:
    [[nodiscard]] bool try_add_element(ElementDesc desc);
    [[nodiscard]] const Element* find(const ElementId& id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::vector<Element> traverse() const;
    [[nodiscard]] bool set_visible(const ElementId& id, bool visible) noexcept;
    [[nodiscard]] bool set_enabled(const ElementId& id, bool enabled) noexcept;
    [[nodiscard]] bool set_text(const ElementId& id, TextContent text);
    [[nodiscard]] bool is_descendant_or_same(const ElementId& ancestor, const ElementId& id) const noexcept;

  private:
    [[nodiscard]] Element* find_mutable(const ElementId& id) noexcept;
    void append_subtree(const Element& element, std::vector<Element>& out) const;

    std::vector<Element> elements_;
};

[[nodiscard]] bool is_valid_rect(Rect rect) noexcept;
[[nodiscard]] bool is_valid_style(const Style& style) noexcept;
[[nodiscard]] bool is_valid_text_content(const TextContent& text) noexcept;
[[nodiscard]] bool is_valid_image_content(const ImageContent& image) noexcept;
[[nodiscard]] Size constrain_size(Size size, SizeConstraints constraints) noexcept;
[[nodiscard]] Style resolve_style(const Style& parent, const Style& element);
[[nodiscard]] std::string_view semantic_role_id(SemanticRole role) noexcept;

struct ElementLayout {
    ElementId id;
    ElementId parent;
    SemanticRole role{SemanticRole::none};
    Rect bounds;
    bool visible{true};
};

struct LayoutResult {
    std::vector<ElementLayout> elements;
};

[[nodiscard]] const ElementLayout* find_layout(const LayoutResult& layout, const ElementId& id) noexcept;
[[nodiscard]] LayoutResult solve_layout(const UiDocument& document, const ElementId& root, Rect viewport);

enum class AdapterBoundary : std::uint8_t {
    text_shaping,
    bidirectional_text,
    line_breaking,
    font_rasterization,
    glyph_atlas,
    ime_composition,
    accessibility_bridge,
    image_decoding,
    renderer_submission,
    clipboard,
    platform_integration,
    middleware,
};

struct AdapterContract {
    AdapterBoundary boundary{AdapterBoundary::text_shaping};
    std::string_view id;
    std::string_view purpose;
};

[[nodiscard]] std::vector<AdapterContract> required_adapter_contracts();

struct TextLayoutRequest {
    std::string text;
    std::string font_family;
    TextDirection direction{TextDirection::automatic};
    TextWrapMode wrap{TextWrapMode::clip};
    float max_width{0.0F};
};

struct TextLayoutRun {
    std::string text;
    Rect bounds;
};

struct MonospaceTextLayoutPolicy {
    float glyph_advance{8.0F};
    float whitespace_advance{4.0F};
    float line_height{16.0F};
};

[[nodiscard]] bool is_valid_monospace_text_layout_policy(MonospaceTextLayoutPolicy policy) noexcept;

class ITextShapingAdapter {
  public:
    virtual ~ITextShapingAdapter() = default;
    [[nodiscard]] virtual std::vector<TextLayoutRun> shape_text(const TextLayoutRequest& request) = 0;
};

class ILineBreakingAdapter {
  public:
    virtual ~ILineBreakingAdapter() = default;
    [[nodiscard]] virtual std::vector<TextLayoutRun> break_lines(const TextLayoutRequest& request) = 0;
};

struct FontRasterizationRequest {
    std::string font_family;
    std::uint32_t glyph{0};
    float pixel_size{0.0F};
};

struct GlyphAtlasAllocation {
    std::uint32_t glyph{0};
    Rect atlas_bounds;
};

class IFontRasterizerAdapter {
  public:
    virtual ~IFontRasterizerAdapter() = default;
    [[nodiscard]] virtual GlyphAtlasAllocation rasterize_glyph(const FontRasterizationRequest& request) = 0;
};

struct ImeComposition {
    ElementId target;
    std::string composition_text;
    std::size_t cursor_index{0};
};

class IImeAdapter {
  public:
    virtual ~IImeAdapter() = default;
    virtual void update_composition(const ImeComposition& composition) = 0;
};

struct AccessibilityNode {
    ElementId id;
    SemanticRole role{SemanticRole::none};
    std::string label;
    Rect bounds;
    std::string localization_key;
    bool enabled{true};
    bool focusable{false};
    ElementId parent;
    std::size_t depth{0};
};

class IAccessibilityAdapter {
  public:
    virtual ~IAccessibilityAdapter() = default;
    virtual void publish_nodes(const std::vector<AccessibilityNode>& nodes) = 0;
};

struct ImageDecodeRequest {
    std::string asset_uri;
    std::vector<std::byte> bytes;
};

enum class ImageDecodePixelFormat : std::uint8_t {
    unknown,
    rgba8_unorm,
};

struct ImageDecodeResult {
    std::uint32_t width{0};
    std::uint32_t height{0};
    ImageDecodePixelFormat pixel_format{ImageDecodePixelFormat::unknown};
    std::vector<std::byte> pixels;
};

class IImageDecodingAdapter {
  public:
    virtual ~IImageDecodingAdapter() = default;
    [[nodiscard]] virtual std::optional<ImageDecodeResult> decode_image(const ImageDecodeRequest& request) = 0;
};

struct RendererBox {
    ElementId id;
    SemanticRole role{SemanticRole::none};
    Rect bounds;
    std::string background_token;
    std::string foreground_token;
    bool enabled{true};
};

struct RendererTextRun {
    ElementId id;
    Rect bounds;
    TextContent text;
    std::string foreground_token;
    bool enabled{true};
};

struct RendererImagePlaceholder {
    ElementId id;
    Rect bounds;
    ImageContent image;
    bool enabled{true};
};

struct RendererSubmission {
    std::vector<Element> elements;
    std::vector<ElementLayout> layouts;
    std::vector<RendererBox> boxes;
    std::vector<RendererTextRun> text_runs;
    std::vector<RendererImagePlaceholder> image_placeholders;
    std::vector<AccessibilityNode> accessibility_nodes;
};

[[nodiscard]] RendererSubmission build_renderer_submission(const UiDocument& document, const LayoutResult& layout);

enum class AdapterPayloadDiagnosticCode : std::uint8_t {
    invalid_text_bounds,
    unresolved_text_localization_key,
    empty_text_payload,
    invalid_text_shaping_text,
    invalid_text_shaping_font_family,
    invalid_text_shaping_max_width,
    invalid_text_shaping_result,
    invalid_image_bounds,
    empty_image_reference,
    invalid_image_decode_uri,
    empty_image_decode_bytes,
    invalid_image_decode_result,
    invalid_accessibility_bounds,
    invalid_ime_target,
    invalid_ime_cursor,
    invalid_platform_text_input_target,
    invalid_platform_text_input_bounds,
    invalid_text_edit_target,
    mismatched_committed_text_target,
    mismatched_text_edit_command_target,
    mismatched_text_edit_clipboard_command_target,
    invalid_text_edit_cursor,
    invalid_text_edit_selection,
    invalid_committed_text,
    invalid_text_edit_command,
    invalid_text_edit_clipboard_command,
    invalid_clipboard_text_target,
    invalid_clipboard_text,
    invalid_clipboard_text_result,
    invalid_font_family,
    invalid_font_glyph,
    invalid_font_pixel_size,
    invalid_font_allocation,
    invalid_text_layout_policy,
    text_layout_clipped,
    unsupported_text_direction,
};

struct AdapterPayloadDiagnostic {
    ElementId id;
    AdapterPayloadDiagnosticCode code{AdapterPayloadDiagnosticCode::invalid_text_bounds};
    std::string message;
};

struct PlatformTextInputRequest {
    ElementId target;
    Rect text_bounds;
};

// Text must be strict UTF-8. Offsets are byte offsets on scalar boundaries.
// A selection covers [cursor_byte_offset, cursor_byte_offset + selection_byte_length).
struct TextEditState {
    ElementId target;
    std::string text;
    std::size_t cursor_byte_offset{0};
    std::size_t selection_byte_length{0};
};

struct CommittedTextInput {
    ElementId target;
    std::string text;
};

// Host-independent edit commands. Host key mapping, key repeat, clipboard,
// selection UI, IME sessions, and grapheme/word movement live outside mirakana_ui.
enum class TextEditCommandKind : std::uint8_t {
    move_cursor_backward,
    move_cursor_forward,
    move_cursor_to_start,
    move_cursor_to_end,
    delete_backward,
    delete_forward,
};

struct TextEditCommand {
    ElementId target;
    TextEditCommandKind kind{TextEditCommandKind::move_cursor_backward};
};

// Clipboard commands are host-independent. Platform shortcut mapping, rich
// clipboard formats, selection UI, and native text services live outside mirakana_ui.
enum class TextEditClipboardCommandKind : std::uint8_t {
    copy_selection,
    cut_selection,
    paste_text,
};

struct TextEditClipboardCommand {
    ElementId target;
    TextEditClipboardCommandKind kind{TextEditClipboardCommandKind::copy_selection};
};

struct ClipboardTextWriteRequest {
    ElementId target;
    std::string text;
};

struct ClipboardTextReadRequest {
    ElementId target;
};

struct TextAdapterGlyphPlaceholder {
    std::size_t code_unit_offset{0};
    std::size_t code_unit_count{0};
    Rect bounds;
    std::uint32_t glyph{0};
};

struct TextAdapterLine {
    std::size_t code_unit_offset{0};
    std::size_t code_unit_count{0};
    Rect bounds;
    std::vector<TextAdapterGlyphPlaceholder> glyphs;
};

struct TextAdapterRow {
    ElementId id;
    Rect bounds;
    std::string text;
    std::string localization_key;
    std::string font_family;
    TextDirection direction{TextDirection::automatic};
    TextWrapMode wrap{TextWrapMode::clip};
    std::string foreground_token;
    bool enabled{true};
    std::vector<TextAdapterLine> lines;
};

struct TextAdapterPayload {
    std::vector<TextAdapterRow> rows;
    std::vector<AdapterPayloadDiagnostic> diagnostics;
};

struct ImageAdapterRow {
    ElementId id;
    Rect bounds;
    std::string resource_id;
    std::string asset_uri;
    std::string tint_token;
    bool enabled{true};
};

struct ImageAdapterPayload {
    std::vector<ImageAdapterRow> rows;
    std::vector<AdapterPayloadDiagnostic> diagnostics;
};

struct TextShapingRequestPlan {
    TextLayoutRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct TextShapingResult {
    bool shaped{false};
    std::vector<TextLayoutRun> runs;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct ImageDecodeRequestPlan {
    ImageDecodeRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct ImageDecodeDispatchResult {
    bool decoded{false};
    std::optional<ImageDecodeResult> image;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct AccessibilityPayload {
    std::vector<AccessibilityNode> nodes;
    std::vector<AdapterPayloadDiagnostic> diagnostics;
};

struct FontRasterizationRequestPlan {
    FontRasterizationRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct FontRasterizationResult {
    bool rasterized{false};
    std::optional<GlyphAtlasAllocation> allocation;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct ImeCompositionPublishPlan {
    ImeComposition composition;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct ImeCompositionPublishResult {
    bool published{false};
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct PlatformTextInputSessionPlan {
    PlatformTextInputRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct PlatformTextInputSessionResult {
    bool begun{false};
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct PlatformTextInputEndPlan {
    ElementId target;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct PlatformTextInputEndResult {
    bool ended{false};
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct TextEditCommitPlan {
    TextEditState state;
    CommittedTextInput input;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct TextEditCommitResult {
    bool committed{false};
    TextEditState state;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct TextEditCommandPlan {
    TextEditState state;
    TextEditCommand command;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct TextEditCommandResult {
    bool applied{false};
    TextEditState state;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct TextEditClipboardCommandPlan {
    TextEditState state;
    TextEditClipboardCommand command;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct TextEditClipboardCommandResult {
    bool applied{false};
    TextEditState state;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct ClipboardTextWritePlan {
    ClipboardTextWriteRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct ClipboardTextWriteResult {
    bool written{false};
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct ClipboardTextReadPlan {
    ClipboardTextReadRequest request;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct ClipboardTextReadResult {
    bool read{false};
    bool has_text{false};
    std::string text;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct AccessibilityPublishPlan {
    std::vector<AccessibilityNode> nodes;
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct AccessibilityPublishResult {
    bool published{false};
    std::size_t nodes_published{0};
    std::vector<AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

class IPlatformIntegrationAdapter {
  public:
    virtual ~IPlatformIntegrationAdapter() = default;
    virtual void begin_text_input(const PlatformTextInputRequest& request) = 0;
    virtual void end_text_input(const ElementId& target) = 0;
};

class IClipboardTextAdapter {
  public:
    virtual ~IClipboardTextAdapter() = default;
    virtual void set_clipboard_text(std::string_view text) = 0;
    [[nodiscard]] virtual bool has_clipboard_text() const = 0;
    [[nodiscard]] virtual std::string clipboard_text() const = 0;
};

[[nodiscard]] TextAdapterPayload build_text_adapter_payload(const RendererSubmission& submission);
[[nodiscard]] TextAdapterPayload build_text_adapter_payload(const RendererSubmission& submission,
                                                            MonospaceTextLayoutPolicy policy);
[[nodiscard]] ImageAdapterPayload build_image_adapter_payload(const RendererSubmission& submission);
[[nodiscard]] AccessibilityPayload build_accessibility_payload(const RendererSubmission& submission);
[[nodiscard]] TextShapingRequestPlan plan_text_shaping_request(const TextLayoutRequest& request);
[[nodiscard]] TextShapingResult shape_text_run(ITextShapingAdapter& adapter, const TextLayoutRequest& request);
[[nodiscard]] ImageDecodeRequestPlan plan_image_decode_request(const ImageDecodeRequest& request);
[[nodiscard]] ImageDecodeDispatchResult decode_image_request(IImageDecodingAdapter& adapter,
                                                             const ImageDecodeRequest& request);
[[nodiscard]] FontRasterizationRequestPlan plan_font_rasterization_request(const FontRasterizationRequest& request);
[[nodiscard]] FontRasterizationResult rasterize_font_glyph(IFontRasterizerAdapter& adapter,
                                                           const FontRasterizationRequest& request);
[[nodiscard]] ImeCompositionPublishPlan plan_ime_composition_update(const ImeComposition& composition);
[[nodiscard]] ImeCompositionPublishResult publish_ime_composition(IImeAdapter& adapter,
                                                                  const ImeComposition& composition);
[[nodiscard]] PlatformTextInputSessionPlan plan_platform_text_input_session(const PlatformTextInputRequest& request);
[[nodiscard]] PlatformTextInputSessionResult begin_platform_text_input(IPlatformIntegrationAdapter& adapter,
                                                                       const PlatformTextInputRequest& request);
[[nodiscard]] PlatformTextInputEndPlan plan_platform_text_input_end(const ElementId& target);
[[nodiscard]] PlatformTextInputEndResult end_platform_text_input(IPlatformIntegrationAdapter& adapter,
                                                                 const ElementId& target);
[[nodiscard]] TextEditCommitPlan plan_committed_text_input(const TextEditState& state, const CommittedTextInput& input);
[[nodiscard]] TextEditCommitResult apply_committed_text_input(const TextEditState& state,
                                                              const CommittedTextInput& input);
// Movement clears selection and collapses backward/forward selections to their
// start/end. Deletion removes selection before scalar fallback. Valid no-ops set applied=true.
[[nodiscard]] TextEditCommandPlan plan_text_edit_command(const TextEditState& state, const TextEditCommand& command);
[[nodiscard]] TextEditCommandResult apply_text_edit_command(const TextEditState& state, const TextEditCommand& command);
[[nodiscard]] TextEditClipboardCommandPlan plan_text_edit_clipboard_command(const TextEditState& state,
                                                                            const TextEditClipboardCommand& command);
[[nodiscard]] TextEditClipboardCommandResult apply_text_edit_clipboard_command(IClipboardTextAdapter& adapter,
                                                                               const TextEditState& state,
                                                                               const TextEditClipboardCommand& command);
[[nodiscard]] ClipboardTextWritePlan plan_clipboard_text_write(const ClipboardTextWriteRequest& request);
[[nodiscard]] ClipboardTextWriteResult write_clipboard_text(IClipboardTextAdapter& adapter,
                                                            const ClipboardTextWriteRequest& request);
[[nodiscard]] ClipboardTextReadPlan plan_clipboard_text_read(const ClipboardTextReadRequest& request);
[[nodiscard]] ClipboardTextReadResult read_clipboard_text(IClipboardTextAdapter& adapter,
                                                          const ClipboardTextReadRequest& request);
[[nodiscard]] AccessibilityPublishPlan plan_accessibility_publish(const AccessibilityPayload& payload);
[[nodiscard]] AccessibilityPublishResult publish_accessibility_payload(IAccessibilityAdapter& adapter,
                                                                       const AccessibilityPayload& payload);

class IRendererSubmissionAdapter {
  public:
    virtual ~IRendererSubmissionAdapter() = default;
    virtual void submit_ui(const RendererSubmission& submission) = 0;
};

enum class NavigationDirection : std::uint8_t {
    next,
    previous,
    up,
    down,
    left,
    right,
};

enum class InputModality : std::uint8_t {
    mouse,
    touch,
    keyboard,
    gamepad,
};

class InteractionState {
  public:
    [[nodiscard]] const ElementId& focused() const noexcept;
    [[nodiscard]] const ElementId& hovered() const noexcept;
    [[nodiscard]] const ElementId& active() const noexcept;
    [[nodiscard]] const ElementId& modal_layer() const noexcept;

    [[nodiscard]] bool set_focus(const UiDocument& document, const ElementId& id);
    [[nodiscard]] bool route_navigation(const UiDocument& document, NavigationDirection direction);
    void set_hovered(const ElementId& id) noexcept;
    void set_active(const ElementId& id) noexcept;
    void push_modal_layer(ElementId id);
    [[nodiscard]] bool pop_modal_layer() noexcept;

  private:
    [[nodiscard]] bool can_focus(const UiDocument& document, const ElementId& id) const noexcept;

    ElementId focused_;
    ElementId hovered_;
    ElementId active_;
    ElementId modal_layer_;
};

struct TransitionState {
    ElementId element;
    std::string property;
    float start_value{0.0F};
    float end_value{0.0F};
    float duration_seconds{0.0F};
    float elapsed_seconds{0.0F};
};

struct TransitionSample {
    float value{0.0F};
    float progress{0.0F};
    bool finished{false};
};

[[nodiscard]] TransitionSample sample_transition(const TransitionState& transition) noexcept;
[[nodiscard]] TransitionSample advance_transition(TransitionState& transition, float delta_seconds);

struct TextBinding {
    ElementId element;
    std::string source_key;
};

class BindingContext {
  public:
    void set_value(std::string key, std::string value);
    [[nodiscard]] std::optional<std::string_view> value(std::string_view key) const noexcept;

  private:
    std::vector<std::pair<std::string, std::string>> values_;
};

[[nodiscard]] bool apply_text_binding(UiDocument& document, const TextBinding& binding, const BindingContext& context);

struct CommandBinding {
    std::string id;
    std::function<void()> action;
    bool enabled{true};
};

class CommandRegistry {
  public:
    [[nodiscard]] bool try_add(CommandBinding command);
    [[nodiscard]] bool execute(std::string_view id) const;

  private:
    [[nodiscard]] const CommandBinding* find(std::string_view id) const noexcept;

    std::vector<CommandBinding> commands_;
};

} // namespace mirakana::ui
