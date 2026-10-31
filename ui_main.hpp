#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "colorbot.hpp"
#include "ui_theme.hpp"
#include <cstdio>
#include <string>

namespace UIRenderer {
    using UITheme::Px;

    struct MenuState {
        int tab = 0;
        int binding = 0;
        bool wait_for_release = false;
        int suppressed_key = 0;
        bool overlay_unavailable = false;
    };

    inline MenuState& State() { static MenuState state; return state; }
    inline bool IsCapturingKey() { return State().binding != 0 || State().suppressed_key != 0; }

    inline std::string GetKeyName(int vkey) {
        switch (vkey) {
        case VK_LBUTTON: return "Mouse 1";
        case VK_RBUTTON: return "Mouse 2";
        case VK_MBUTTON: return "Mouse 3";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        case VK_SPACE: return "Space";
        case VK_SHIFT: return "Shift";
        case VK_CONTROL: return "Ctrl";
        case VK_MENU: return "Alt";
        case VK_INSERT: return "Insert";
        }
        if ((vkey >= '0' && vkey <= '9') || (vkey >= 'A' && vkey <= 'Z'))
            return std::string(1, static_cast<char>(vkey));
        if (vkey >= VK_F1 && vkey <= VK_F24)
            return "F" + std::to_string(vkey - VK_F1 + 1);
        LONG scan = static_cast<LONG>(MapVirtualKeyW(vkey, MAPVK_VK_TO_VSC) << 16);
        if (vkey == VK_LEFT || vkey == VK_RIGHT || vkey == VK_UP || vkey == VK_DOWN ||
            vkey == VK_DELETE || vkey == VK_HOME || vkey == VK_END || vkey == VK_PRIOR || vkey == VK_NEXT)
            scan |= 1 << 24;
        wchar_t keyName[64] = {};
        if (GetKeyNameTextW(scan, keyName, 64)) {
            char utf8[192] = {};
            WideCharToMultiByte(CP_UTF8, 0, keyName, -1, utf8, 192, nullptr, nullptr);
            return utf8;
        }
        char fallback[16];
        std::snprintf(fallback, sizeof(fallback), "Key %02X", vkey);
        return fallback;
    }

    inline void UpdateKeybind(BotConfig& config) {
        auto& state = State();
        if (state.suppressed_key && !(GetAsyncKeyState(state.suppressed_key) & 0x8000))
            state.suppressed_key = 0;
        if (!state.binding) return;
        if (ImGui::GetIO().AppFocusLost) { state.binding = 0; return; }
        if (state.wait_for_release) {
            for (int key = 1; key < 256; ++key)
                if (GetAsyncKeyState(key) & 0x8000) return;
            state.wait_for_release = false;
            return;
        }
        for (int key = 1; key < 256; ++key) {
            if (!(GetAsyncKeyState(key) & 0x8000)) continue;
            if (key != VK_ESCAPE) {
                if (state.binding == 1) config.shoot_vkey = key;
                else config.toggle_vkey = key;
            }
            state.binding = 0;
            state.suppressed_key = key;
            break;
        }
    }

    inline void TransparentButtonColors() {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.025f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.045f));
    }

    inline void TabButton(const char* label, int tab, ImVec2 origin) {
        ImGui::SetCursorScreenPos(origin);
        TransparentButtonColors();
        ImGui::PushStyleColor(ImGuiCol_Text, State().tab == tab ? UITheme::kText : UITheme::kMuted);
        ImGui::PushFont(UITheme::heading_font);
        if (ImGui::Button(label, ImVec2(Px(92), Px(58)))) State().tab = tab;
        ImGui::PopFont();
        ImGui::PopStyleColor(4);
        if (State().tab == tab) {
            float textWidth = ImGui::CalcTextSize(label).x;
            float center = origin.x + Px(46);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(center - textWidth * 0.5f, origin.y + Px(51)),
                ImVec2(center + textWidth * 0.5f, origin.y + Px(54)),
                ImGui::GetColorU32(UITheme::kAccent), Px(2));
        }
    }

    inline void RailButton(int tab, ImVec2 origin) {
        ImGui::SetCursorScreenPos(origin);
        TransparentButtonColors();
        ImGui::PushID(tab);
        if (ImGui::Button("##Navigation", ImVec2(Px(44), Px(44)))) State().tab = tab;
        ImGui::PopID();
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tab == 0 ? "Main" : "Legit");
        auto* draw = ImGui::GetWindowDrawList();
        ImU32 color = ImGui::GetColorU32(State().tab == tab ? UITheme::kAccent : UITheme::kMuted);
        ImVec2 c(origin.x + Px(22), origin.y + Px(22));
        if (tab == 0) {
            draw->AddCircle(c, Px(7), color, 24, Px(1.5f));
            draw->AddLine(ImVec2(c.x - Px(11), c.y), ImVec2(c.x - Px(4), c.y), color, Px(1.5f));
            draw->AddLine(ImVec2(c.x + Px(4), c.y), ImVec2(c.x + Px(11), c.y), color, Px(1.5f));
            draw->AddLine(ImVec2(c.x, c.y - Px(11)), ImVec2(c.x, c.y - Px(4)), color, Px(1.5f));
            draw->AddLine(ImVec2(c.x, c.y + Px(4)), ImVec2(c.x, c.y + Px(11)), color, Px(1.5f));
        } else {
            for (int i = 0; i < 3; ++i) {
                float y = c.y + Px((i - 1) * 7.0f);
                float knob = c.x + Px(i == 1 ? 4.0f : -3.0f);
                draw->AddLine(ImVec2(c.x - Px(9), y), ImVec2(c.x + Px(9), y), color, Px(1.5f));
                draw->AddCircleFilled(ImVec2(knob, y), Px(2.5f), color, 12);
            }
        }
        if (State().tab == tab)
            draw->AddRectFilled(ImVec2(origin.x + Px(50), origin.y + Px(8)),
                ImVec2(origin.x + Px(53), origin.y + Px(36)), color, Px(2));
    }

    struct Layout {
        float left, control, right;
        ImVec2 origin;
    };

    inline void Section(const Layout& layout, const char* label, float y) {
        ImGui::SetCursorScreenPos(ImVec2(layout.left, layout.origin.y + Px(y)));
        ImGui::PushFont(UITheme::heading_font);
        ImGui::TextUnformatted(label);
        ImGui::PopFont();
        float lineY = layout.origin.y + Px(y + 28);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(layout.left, lineY), ImVec2(layout.right, lineY),
            ImGui::GetColorU32(ImGuiCol_Separator));
    }

    inline void RowLabel(const Layout& layout, const char* label, float y) {
        ImGui::SetCursorScreenPos(ImVec2(layout.left, layout.origin.y + Px(y + 7)));
        ImGui::TextUnformatted(label);
    }

    inline bool ToggleRow(const Layout& layout, const char* label, const char* id, float y, bool& enabled) {
        RowLabel(layout, label, y);
        ImVec2 pos(layout.right - Px(42), layout.origin.y + Px(y + 4));
        ImGui::SetCursorScreenPos(pos);
        TransparentButtonColors();
        bool changed = ImGui::Button(id, ImVec2(Px(42), Px(22)));
        if (changed) enabled = !enabled;
        ImGui::PopStyleColor(3);
        auto* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, ImVec2(pos.x + Px(42), pos.y + Px(22)),
            enabled ? ImGui::GetColorU32(UITheme::kAccent) : IM_COL32(38, 38, 53, 255), Px(11));
        draw->AddCircleFilled(ImVec2(pos.x + Px(enabled ? 31.0f : 11.0f), pos.y + Px(11)),
            Px(7), enabled ? IM_COL32(240, 237, 255, 255) : IM_COL32(149, 149, 166, 255), 24);
        ImGui::SetCursorScreenPos(ImVec2(layout.right - Px(83), layout.origin.y + Px(y + 7)));
        ImGui::TextColored(enabled ? UITheme::kText : UITheme::kMuted, "%s", enabled ? "On" : "Off");
        return changed;
    }

    inline bool StepperRow(const Layout& layout, const char* label, const char* id, float y, int& value) {
        RowLabel(layout, label, y);
        int previous = value;
        value = CaptureArea::ClampSize(value);
        ImVec2 pos(layout.control, layout.origin.y + Px(y));
        auto* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(pos, ImVec2(layout.right, pos.y + Px(32)),
            ImGui::GetColorU32(ImGuiCol_FrameBg), Px(5));
        ImGui::PushID(id);
        TransparentButtonColors();
        ImGui::PushButtonRepeat(true);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + Px(4), pos.y));
        ImGui::BeginDisabled(value <= CaptureArea::kMinSize);
        if (ImGui::Button("-", ImVec2(Px(32), Px(32)))) --value;
        ImGui::EndDisabled();
        ImGui::SetCursorScreenPos(ImVec2(layout.right - Px(36), pos.y));
        ImGui::BeginDisabled(value >= CaptureArea::kMaxSize);
        if (ImGui::Button("+", ImVec2(Px(32), Px(32)))) ++value;
        ImGui::EndDisabled();
        ImGui::PopButtonRepeat();
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        value = CaptureArea::ClampSize(value);
        char text[16];
        std::snprintf(text, sizeof(text), "%d px", value);
        float textWidth = ImGui::CalcTextSize(text).x;
        draw->AddText(ImVec2((pos.x + layout.right - textWidth) * 0.5f, pos.y + Px(7)),
            ImGui::GetColorU32(UITheme::kText), text);
        return value != previous;
    }

    inline bool SliderRow(const Layout& layout, const char* label, const char* id,
                          float y, int& value, int min, int max, const char* format) {
        RowLabel(layout, label, y);
        ImGui::PushID(id);
        ImGui::SetCursorScreenPos(ImVec2(layout.right - Px(70), layout.origin.y + Px(y)));
        ImGui::SetNextItemWidth(Px(70));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        bool changed = ImGui::DragInt("##Value", &value, 1.0f, min, max, format, ImGuiSliderFlags_AlwaysClamp);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drag to adjust. Ctrl + click to type a value.");

        float width = layout.right - Px(88) - layout.control;
        ImVec2 pos(layout.control, layout.origin.y + Px(y));
        ImGui::SetCursorScreenPos(pos);
        // The bundled SliderInt overrides widths and colors. Keep its input behavior
        // with explicit bounds, then draw the menu's own track and thumb.
        auto* window = ImGui::GetCurrentWindow();
        auto& context = *GImGui;
        ImGuiID sliderId = window->GetID("##Slider");
        ImRect bounds(pos, ImVec2(pos.x + width, pos.y + Px(30)));
        ImGui::ItemSize(bounds);
        if (!ImGui::ItemAdd(bounds, sliderId)) { ImGui::PopID(); return changed; }
        bool hovered = ImGui::ItemHoverable(bounds, sliderId);
        if ((hovered && ImGui::IsMouseClicked(0)) || context.NavActivateId == sliderId || context.NavActivateInputId == sliderId) {
            ImGui::SetActiveID(sliderId, window);
            ImGui::SetFocusID(sliderId, window);
            ImGui::FocusWindow(window);
            context.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
        ImRect grab;
        bool sliderChanged = ImGui::SliderBehavior(bounds, sliderId, ImGuiDataType_S32,
            &value, &min, &max, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput, &grab);
        if (sliderChanged) ImGui::MarkItemEdited(sliderId);
        changed |= sliderChanged;
        bool active = context.ActiveId == sliderId || hovered;
        auto* draw = ImGui::GetWindowDrawList();
        float centerY = (bounds.Min.y + bounds.Max.y) * 0.5f;
        float startX = pos.x + Px(8);
        float endX = bounds.Max.x - Px(8);
        float knobX = (grab.Min.x + grab.Max.x) * 0.5f;
        draw->AddRectFilled(ImVec2(startX, centerY - Px(3)), ImVec2(endX, centerY + Px(3)),
            IM_COL32(24, 24, 36, 255), Px(3));
        draw->AddRectFilled(ImVec2(startX, centerY - Px(3)), ImVec2(knobX, centerY + Px(3)),
            ImGui::GetColorU32(UITheme::kAccent), Px(3));
        draw->AddCircleFilled(ImVec2(knobX, centerY), Px(active ? 6.0f : 5.0f),
            active ? IM_COL32(155, 134, 255, 255) : IM_COL32(129, 107, 247, 255), 24);
        ImGui::RenderNavHighlight(bounds, sliderId);
        ImGui::PopID();
        return changed;
    }

    inline bool ComboField(const Layout& layout, const char* id, float y, int& selected,
                           const char* const* options, int count, bool colorSwatch = false) {
        ImGui::PushID(id);
        ImVec2 pos(layout.control, layout.origin.y + Px(y));
        float width = layout.right - layout.control;
        ImGui::SetCursorScreenPos(pos);
        if (ImGui::Button("##Field", ImVec2(width, Px(32)))) ImGui::OpenPopup("Options");
        auto* draw = ImGui::GetWindowDrawList();
        draw->AddText(ImVec2(pos.x + Px(colorSwatch ? 32.0f : 12.0f), pos.y + Px(7)),
            ImGui::GetColorU32(UITheme::kText), options[selected]);
        if (colorSwatch)
            draw->AddCircleFilled(ImVec2(pos.x + Px(17), pos.y + Px(16)), Px(4), IM_COL32(155, 100, 245, 255), 20);
        ImU32 arrow = ImGui::GetColorU32(UITheme::kMuted);
        draw->AddLine(ImVec2(layout.right - Px(21), pos.y + Px(14)), ImVec2(layout.right - Px(17), pos.y + Px(18)), arrow, Px(1.4f));
        draw->AddLine(ImVec2(layout.right - Px(17), pos.y + Px(18)), ImVec2(layout.right - Px(13), pos.y + Px(14)), arrow, Px(1.4f));
        bool changed = false;
        float popupHeight = Px(12.0f + static_cast<float>(count) * 30.0f) +
            ImGui::GetStyle().ItemSpacing.y * static_cast<float>(count - 1);
        float popupY = pos.y + Px(36);
        if (popupY + popupHeight > ImGui::GetIO().DisplaySize.y - Px(12))
            popupY = pos.y - popupHeight - Px(4);
        ImGui::SetNextWindowPos(ImVec2(pos.x, popupY));
        ImGui::SetNextWindowSize(ImVec2(width, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(6), Px(6)));
        if (ImGui::BeginPopup("Options", ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.5f));
            for (int i = 0; i < count; ++i) {
                ImGui::PushID(i);
                ImGui::PushStyleColor(ImGuiCol_Button, i == selected ? ImVec4(0.16f, 0.12f, 0.29f, 1.0f) : ImVec4(0, 0, 0, 0));
                if (ImGui::Button(options[i], ImVec2(width - Px(12), Px(30)))) {
                    selected = i;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                if (i == selected) {
                    ImVec2 item = ImGui::GetItemRectMin();
                    ImGui::RenderCheckMark(ImGui::GetWindowDrawList(), ImVec2(layout.right - Px(31), item.y + Px(10)),
                        ImGui::GetColorU32(UITheme::kAccent), Px(10));
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::PopStyleVar();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();
        return changed;
    }

    inline void KeybindRow(const Layout& layout, const char* label, float y, int binding, int key) {
        RowLabel(layout, label, y);
        ImGui::SetCursorScreenPos(ImVec2(layout.control, layout.origin.y + Px(y)));
        ImGui::PushID(binding);
        bool listening = State().binding == binding;
        if (listening) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.69f, 0.61f, 1.0f, 1.0f));
        std::string text = listening ? "Press a key..." : GetKeyName(key);
        std::string buttonLabel = text + "###Keybind";
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(layout.right - layout.control, Px(32)))) {
            State().binding = binding;
            State().wait_for_release = true;
        }
        if (listening) {
            ImGui::PopStyleColor();
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                ImGui::GetColorU32(UITheme::kAccent), Px(5));
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click, then press a keyboard or mouse button. Esc cancels.");
        ImGui::PopID();
    }

    inline void Render(ColorBot& bot, bool& should_exit) {
        auto& config = bot.GetConfig();
        auto& state = State();
        UpdateKeybind(config);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("CB Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
        ImVec2 origin = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        auto* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(origin, ImVec2(origin.x + Px(64), origin.y + size.y), IM_COL32(14, 14, 23, 255));
        draw->AddLine(ImVec2(origin.x + Px(64), origin.y), ImVec2(origin.x + Px(64), origin.y + size.y), IM_COL32(23, 23, 34, 255));
        draw->AddLine(ImVec2(origin.x + Px(64), origin.y + Px(58)),
            ImVec2(origin.x + size.x, origin.y + Px(58)), IM_COL32(23, 23, 34, 255));
        ImGui::SetCursorScreenPos(ImVec2(origin.x + Px(21), origin.y + Px(20)));
        ImGui::PushFont(UITheme::heading_font);
        ImGui::TextColored(UITheme::kAccent, "CB");
        ImGui::PopFont();
        TabButton("Main", 0, ImVec2(origin.x + Px(82), origin.y));
        TabButton("Legit", 1, ImVec2(origin.x + Px(174), origin.y));
        RailButton(0, ImVec2(origin.x + Px(10), origin.y + Px(80)));
        RailButton(1, ImVec2(origin.x + Px(10), origin.y + Px(140)));

        ImVec2 close(origin.x + size.x - Px(46), origin.y + Px(14));
        ImGui::SetCursorScreenPos(close);
        TransparentButtonColors();
        if (ImGui::Button("##Close", ImVec2(Px(28), Px(28)))) should_exit = true;
        ImU32 closeColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? UITheme::kText : UITheme::kMuted);
        ImGui::PopStyleColor(3);
        draw->AddLine(ImVec2(close.x + Px(10), close.y + Px(10)), ImVec2(close.x + Px(18), close.y + Px(18)), closeColor, Px(1.4f));
        draw->AddLine(ImVec2(close.x + Px(18), close.y + Px(10)), ImVec2(close.x + Px(10), close.y + Px(18)), closeColor, Px(1.4f));

        Layout layout{ origin.x + Px(94), origin.x + Px(268), origin.x + size.x - Px(38), origin };
        if (state.tab == 0) {
            Section(layout, "Colorbot", 82);
            bool enabled = config.enabled.load();
            if (ToggleRow(layout, "Enable", "##Enable", 124, enabled)) config.enabled = enabled;

            RowLabel(layout, "Target color", 168);
            const char* colors[] = { "Purple" };
            int color = 0;
            ComboField(layout, "Color", 168, color, colors, 1, true);

            Section(layout, "Keybinds", 238);
            KeybindRow(layout, "Shoot", 280, 1, config.shoot_vkey.load());
            KeybindRow(layout, "Toggle", 324, 2, config.toggle_vkey.load());
            ImGui::SetCursorScreenPos(ImVec2(layout.left, origin.y + Px(379)));
            ImGui::TextColored(UITheme::kMuted, "%s", state.binding ? "Press a key or mouse button. Esc cancels." : "Click a keybind to change it.");
        } else {
            state.binding = 0;
            Section(layout, "Timing", 82);
            int tolerance = config.tolerance.load();
            if (SliderRow(layout, "Tolerance", "Tolerance", 124, tolerance, 0, 50, "%d")) config.tolerance = tolerance;
            int cooldown = config.cooldown_ms.load();
            if (SliderRow(layout, "Shot delay", "Cooldown", 164, cooldown, 1, 1000, "%d ms")) config.cooldown_ms = cooldown;
            int minDelay = config.delay_min_ms.load();
            int maxDelay = config.delay_max_ms.load();
            if (SliderRow(layout, "Human min", "MinDelay", 204, minDelay, 1, 500, "%d ms")) {
                config.delay_min_ms = minDelay;
                if (minDelay > maxDelay) config.delay_max_ms = maxDelay = minDelay;
            }
            if (SliderRow(layout, "Human max", "MaxDelay", 244, maxDelay, 1, 500, "%d ms")) {
                config.delay_max_ms = maxDelay;
                if (maxDelay < minDelay) config.delay_min_ms = minDelay = maxDelay;
            }
            Section(layout, "Capture area", 304);
            int width = config.capture_x.load();
            int height = config.capture_y.load();
            if (StepperRow(layout, "Width (X)", "CaptureX", 346, width)) config.capture_x = width;
            if (StepperRow(layout, "Height (Y)", "CaptureY", 386, height)) config.capture_y = height;
            bool showOverlay = config.show_capture_area.load();
            if (ToggleRow(layout, "Show overlay", "##CaptureOverlay", 428, showOverlay)) config.show_capture_area = showOverlay;
            if (showOverlay) {
                ImGui::SetCursorScreenPos(ImVec2(layout.left, origin.y + Px(475)));
                ImGui::TextColored(UITheme::kMuted, "%s", state.overlay_unavailable ?
                    "Overlay could not be displayed." : "Outline follows X/Y at screen center.");
            }
        }
        ImGui::SetCursorScreenPos(ImVec2(layout.left, origin.y + size.y - Px(30)));
        ImGui::TextColored(UITheme::kMuted, "Insert to hide menu");
        ImGui::End();
    }
}
