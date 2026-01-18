#include "pch.h"
#include "ModernUI.h"
#include <algorithm>
#include <cmath>

namespace UI {
    namespace Modern {

        // Helper: Draw Vector Icons
        void DrawIcon(ImDrawList* dl, IconType icon, ImVec2 center, float size, ImU32 color) {
            float hs = size * 0.5f; // Half size
            
            switch (icon) {
            case IconType::Search: {
                // Magnifying glass
                dl->AddCircle(center, hs * 0.6f, color, 12, 2.0f);
                float lineLen = hs * 0.8f;
                dl->AddLine(ImVec2(center.x + hs * 0.4f, center.y + hs * 0.4f), 
                           ImVec2(center.x + lineLen, center.y + lineLen), color, 2.0f);
                break;
            }
            case IconType::Settings: {
                // Cog (simplified as circle + spokes)
                dl->AddCircle(center, hs * 0.5f, color, 0, 2.0f);
                for (int i = 0; i < 8; i++) {
                    float angle = (float)i * 3.14159f / 4.0f;
                    float cx = cosf(angle);
                    float cy = sinf(angle);
                    dl->AddLine(ImVec2(center.x + cx * hs * 0.5f, center.y + cy * hs * 0.5f),
                               ImVec2(center.x + cx * hs * 0.9f, center.y + cy * hs * 0.9f), color, 2.0f);
                }
                break;
            }
            case IconType::Play: {
                // Triangle
                ImVec2 p1(center.x - hs * 0.4f, center.y - hs * 0.6f);
                ImVec2 p2(center.x - hs * 0.4f, center.y + hs * 0.6f);
                ImVec2 p3(center.x + hs * 0.6f, center.y);
                dl->AddTriangleFilled(p1, p2, p3, color);
                break;
            }
            case IconType::Stop: {
                // Square
                dl->AddRectFilled(ImVec2(center.x - hs * 0.5f, center.y - hs * 0.5f),
                                 ImVec2(center.x + hs * 0.5f, center.y + hs * 0.5f), color, 2.0f);
                break;
            }
            case IconType::Shuffle: {
                // Crossed arrows
                dl->AddLine(ImVec2(center.x - hs * 0.6f, center.y - hs * 0.4f), ImVec2(center.x + hs * 0.6f, center.y + hs * 0.4f), color, 2.0f);
                dl->AddLine(ImVec2(center.x - hs * 0.6f, center.y + hs * 0.4f), ImVec2(center.x + hs * 0.6f, center.y - hs * 0.4f), color, 2.0f);
                // Arrow heads
                dl->AddLine(ImVec2(center.x + hs * 0.6f, center.y + hs * 0.4f), ImVec2(center.x + hs * 0.3f, center.y + hs * 0.4f), color, 2.0f);
                dl->AddLine(ImVec2(center.x + hs * 0.6f, center.y - hs * 0.4f), ImVec2(center.x + hs * 0.3f, center.y - hs * 0.4f), color, 2.0f);
                break;
            }
            case IconType::Delete: {
                // Trash can
                dl->AddLine(ImVec2(center.x - hs * 0.5f, center.y - hs * 0.5f), ImVec2(center.x + hs * 0.5f, center.y - hs * 0.5f), color, 2.0f); // Lid
                dl->AddRect(ImVec2(center.x - hs * 0.4f, center.y - hs * 0.5f), ImVec2(center.x + hs * 0.4f, center.y + hs * 0.7f), color, 0.0f, 15, 2.0f); // Body
                break;
            }
            case IconType::Check: {
                // Checkmark
                dl->AddLine(ImVec2(center.x - hs * 0.6f, center.y), ImVec2(center.x - hs * 0.2f, center.y + hs * 0.6f), color, 2.0f);
                dl->AddLine(ImVec2(center.x - hs * 0.2f, center.y + hs * 0.6f), ImVec2(center.x + hs * 0.7f, center.y - hs * 0.6f), color, 2.0f);
                break;
            }
            case IconType::Cross: {
                // X
                float s = hs * 0.5f;
                dl->AddLine(ImVec2(center.x - s, center.y - s), ImVec2(center.x + s, center.y + s), color, 2.0f);
                dl->AddLine(ImVec2(center.x + s, center.y - s), ImVec2(center.x - s, center.y + s), color, 2.0f);
                break;
            }
            case IconType::Refresh: {
                // Arrow circle
                dl->AddCircle(center, hs * 0.6f, color, 0, 2.0f);
                // Simple arrow head at top
                dl->AddTriangleFilled(ImVec2(center.x, center.y - hs * 0.8f), ImVec2(center.x + 4, center.y - hs * 0.6f), ImVec2(center.x - 4, center.y - hs * 0.6f), color);
                break;
            }
            default: break;
            }
        }

        // --- Widgets ---

        bool PrimaryButton(const char* id, const char* label, IconType icon, const ImVec2& size_arg) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiStyle& style = g.Style;
            const ImGuiID imguiId = window->GetID(id);
            const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

            // Calculate size - account for icon space only if icon is provided
            bool hasIcon = (icon != IconType::None);
            float iconSpace = hasIcon ? 36.0f : 0.0f;
            float minWidth = label_size.x + style.FramePadding.x * 2.0f + iconSpace;
            float minHeight = label_size.y + style.FramePadding.y * 2.0f;

            ImVec2 pos = window->DC.CursorPos;
            // If explicit size given, use it but ensure minimum for text
            ImVec2 size;
            if (size_arg.x > 0 && size_arg.y > 0) {
                size = ImVec2(ImMax(size_arg.x, minWidth), ImMax(size_arg.y, minHeight));
            } else {
                size = ImVec2(minWidth, minHeight);
            }

            const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
            ImGui::ItemSize(size, style.FramePadding.y);
            if (!ImGui::ItemAdd(bb, imguiId)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, imguiId, &hovered, &held);

            // Render
            const ImU32 bg_col = held ? Theme::Col_AccentHover : hovered ? Theme::Col_Accent : IM_COL32(0, 150, 220, 255);

            // Draw Shadow
            window->DrawList->AddRectFilled(ImVec2(bb.Min.x+2, bb.Min.y+2), ImVec2(bb.Max.x+2, bb.Max.y+2), IM_COL32(0,0,0,100), Theme::Radius_Medium);
            // Draw BG
            window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, Theme::Radius_Medium);

            // Calculate text position (centered, with icon offset if present)
            float textX;
            if (hasIcon) {
                // Draw Icon
                DrawIcon(window->DrawList, icon, ImVec2(bb.Min.x + 18, bb.GetCenter().y), 14.0f, Theme::Col_Text);
                textX = bb.Min.x + 36;
            } else {
                // Center text horizontally
                textX = bb.Min.x + (size.x - label_size.x) * 0.5f;
            }

            // Draw Text
            ImVec2 textPos(textX, bb.Min.y + (size.y - label_size.y) * 0.5f);
            ImGui::RenderText(textPos, label);

            return pressed;
        }

        bool SecondaryButton(const char* id, const char* label, IconType icon, const ImVec2& size_arg) {
            // Similar to Primary but different colors
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiStyle& style = g.Style;
            const ImGuiID imguiId = window->GetID(id);
            const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

            // Calculate size - account for icon space only if icon is provided
            bool hasIcon = (icon != IconType::None);
            float iconSpace = hasIcon ? 36.0f : 0.0f;
            float minWidth = label_size.x + style.FramePadding.x * 2.0f + iconSpace;
            float minHeight = label_size.y + style.FramePadding.y * 2.0f;

            ImVec2 pos = window->DC.CursorPos;
            ImVec2 size;
            if (size_arg.x > 0 && size_arg.y > 0) {
                size = ImVec2(ImMax(size_arg.x, minWidth), ImMax(size_arg.y, minHeight));
            } else if (size_arg.x > 0) {
                size = ImVec2(ImMax(size_arg.x, minWidth), minHeight);
            } else {
                size = ImVec2(minWidth, minHeight);
            }

            const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
            ImGui::ItemSize(size, style.FramePadding.y);
            if (!ImGui::ItemAdd(bb, imguiId)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, imguiId, &hovered, &held);

            const ImU32 bg_col = held ? Theme::Col_CardBgHover : hovered ? Theme::Col_CardBg : IM_COL32(60, 60, 65, 255);

            window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, Theme::Radius_Medium);
            window->DrawList->AddRect(bb.Min, bb.Max, Theme::Col_TextDisabled, Theme::Radius_Medium, ImDrawCornerFlags_All, 1.0f);

            // Calculate text position (centered, with icon offset if present)
            float textX;
            if (hasIcon) {
                DrawIcon(window->DrawList, icon, ImVec2(bb.Min.x + 18, bb.GetCenter().y), 14.0f, Theme::Col_Text);
                textX = bb.Min.x + 36;
            } else {
                textX = bb.Min.x + (size.x - label_size.x) * 0.5f;
            }

            ImVec2 textPos(textX, bb.Min.y + (size.y - label_size.y) * 0.5f);
            ImGui::RenderText(textPos, label);

            return pressed;
        }

        bool IconButton(const char* id, IconType icon, const ImVec2& size_arg, bool active) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiID imguiId = window->GetID(id);
            
            ImVec2 pos = window->DC.CursorPos;
            ImVec2 size = size_arg;
            const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
            
            ImGui::ItemSize(size);
            if (!ImGui::ItemAdd(bb, imguiId)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, imguiId, &hovered, &held);

            // Colors
            ImU32 bg_col = active ? Theme::Col_Accent : (hovered ? Theme::Col_CardBgHover : Theme::Col_CardBg);
            ImU32 icon_col = active ? Theme::Col_Text : (hovered ? Theme::Col_Accent : Theme::Col_TextDisabled);

            // Render
            window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, Theme::Radius_Small);
            DrawIcon(window->DrawList, icon, bb.GetCenter(), size.x * 0.5f, icon_col);

            return pressed;
        }

        bool Toggle(const char* id, bool* v, const char* label) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiID imguiId = window->GetID(id);
            
            float height = ImGui::GetFrameHeight();
            float width = height * 1.8f;
            
            ImVec2 pos = window->DC.CursorPos;
            const ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
            
            ImGui::ItemSize(bb);
            if (!ImGui::ItemAdd(bb, imguiId)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, imguiId, &hovered, &held);
            if (pressed) {
                *v = !(*v);
                ImGui::MarkItemEdited(imguiId);
            }

            // Anim
            float t = *v ? 1.0f : 0.0f; // Could be animated with state storage
            
            ImU32 bg_col = *v ? Theme::Col_Accent : Theme::Col_CardBg;
            ImU32 knob_col = Theme::Col_Text;

            // Track
            window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, height * 0.5f);
            
            // Knob
            float pad = 2.0f;
            float knobRadius = (height - pad * 2.0f) * 0.5f;
            float knobX = bb.Min.x + pad + knobRadius + (width - pad * 2.0f - knobRadius * 2.0f) * t;
            window->DrawList->AddCircleFilled(ImVec2(knobX, bb.GetCenter().y), knobRadius, knob_col);

            // Label
            if (label) {
                ImGui::SameLine();
                ImGui::Text("%s", label);
            }

            return pressed;
        }

        bool Checkbox(const char* label, bool* v) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiStyle& style = g.Style;
            const ImGuiID id = window->GetID(label);

            const float boxSize = 18.0f;
            const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);

            ImVec2 pos = window->DC.CursorPos;
            ImVec2 totalSize(boxSize + style.ItemInnerSpacing.x + labelSize.x, ImMax(boxSize, labelSize.y));
            const ImRect bb(pos, ImVec2(pos.x + totalSize.x, pos.y + totalSize.y));

            ImGui::ItemSize(totalSize, style.FramePadding.y);
            if (!ImGui::ItemAdd(bb, id)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
            if (pressed) {
                *v = !(*v);
                ImGui::MarkItemEdited(id);
            }

            // Checkbox box position
            ImVec2 boxMin = pos;
            ImVec2 boxMax(pos.x + boxSize, pos.y + boxSize);

            // Draw checkbox background
            ImU32 bgColor = *v ? Theme::Col_Accent : Theme::Col_CardBg;
            window->DrawList->AddRectFilled(boxMin, boxMax, bgColor, 3.0f);

            // Draw border (visible when unchecked for contrast)
            ImU32 borderColor = *v ? Theme::Col_AccentHover : IM_COL32(100, 100, 110, 255);
            if (hovered) borderColor = Theme::Col_Accent;
            window->DrawList->AddRect(boxMin, boxMax, borderColor, 3.0f, ImDrawCornerFlags_All, 2.0f);

            // Draw checkmark when checked
            if (*v) {
                ImVec2 center((boxMin.x + boxMax.x) * 0.5f, (boxMin.y + boxMax.y) * 0.5f);
                DrawIcon(window->DrawList, IconType::Check, center, boxSize * 0.6f, Theme::Col_Text);
            }

            // Draw label
            ImVec2 labelPos(pos.x + boxSize + style.ItemInnerSpacing.x, pos.y + (boxSize - labelSize.y) * 0.5f);
            window->DrawList->AddText(labelPos, Theme::Col_Text, label);

            return pressed;
        }

        bool StateChip(const char* label, bool* active) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems) return false;

            ImGuiContext& g = *GImGui;
            const ImGuiID id = window->GetID(label);

            const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
            const ImVec2 padding(12.0f, 6.0f);
            const ImVec2 size(labelSize.x + padding.x * 2, labelSize.y + padding.y * 2);

            ImVec2 pos = window->DC.CursorPos;
            const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

            ImGui::ItemSize(size);
            if (!ImGui::ItemAdd(bb, id)) return false;

            bool hovered, held;
            bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
            if (pressed) {
                *active = !(*active);
                ImGui::MarkItemEdited(id);
            }

            // Colors based on state
            ImU32 bgColor, textColor, borderColor;
            if (*active) {
                bgColor = Theme::Col_Accent;
                textColor = Theme::Col_Text;
                borderColor = Theme::Col_AccentHover;
            } else {
                bgColor = Theme::Col_CardBg;
                textColor = Theme::Col_TextDisabled;
                borderColor = IM_COL32(80, 80, 90, 255);
            }
            if (hovered) {
                bgColor = *active ? Theme::Col_AccentHover : Theme::Col_CardBgHover;
            }

            // Draw pill background
            float radius = size.y * 0.5f;
            window->DrawList->AddRectFilled(bb.Min, bb.Max, bgColor, radius);
            window->DrawList->AddRect(bb.Min, bb.Max, borderColor, radius, ImDrawCornerFlags_All, 1.5f);

            // Draw checkmark icon when active
            if (*active) {
                ImVec2 iconPos(bb.Min.x + 8, bb.GetCenter().y);
                DrawIcon(window->DrawList, IconType::Check, iconPos, 10.0f, textColor);
            }

            // Draw label (offset if active to make room for checkmark)
            float textOffsetX = *active ? 16.0f : 0.0f;
            ImVec2 textPos(bb.Min.x + padding.x + textOffsetX, bb.Min.y + padding.y);
            window->DrawList->AddText(textPos, textColor, label);

            return pressed;
        }

        void BeginCard(const char* id, const ImVec2& size_arg) {
            ImGui::BeginGroup(); // For layout
            
            // Logic to draw card BG behind everything in the group
            // We use a Child window to clip and isolate
            ImGui::BeginChild(id, size_arg, false, ImGuiWindowFlags_NoScrollbar);
            
            ImVec2 p_min = ImGui::GetWindowPos();
            ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowWidth(), p_min.y + ImGui::GetWindowHeight());
            
            ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, Theme::Col_CardBg, Theme::Radius_Medium);
            ImGui::GetWindowDrawList()->AddRect(p_min, p_max, IM_COL32(255,255,255,10), Theme::Radius_Medium, ImDrawCornerFlags_All, 1.0f); // Border
            
            ImGui::SetCursorPos(ImVec2(Theme::Padding, Theme::Padding));
            ImGui::BeginGroup(); // Content group
        }

        void EndCard() {
            ImGui::EndGroup(); // Content group
            ImGui::EndChild();
            ImGui::EndGroup(); // Main group
        }

        void StatusBadge(const char* text, ImU32 color) {
            ImVec2 textSize = ImGui::CalcTextSize(text);
            ImVec2 pad(8, 2);
            ImVec2 size(textSize.x + pad.x * 2, textSize.y + pad.y * 2);
            
            ImVec2 pos = ImGui::GetCursorScreenPos();
            
            ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), color, 10.0f);
            ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + pad.x, pos.y + pad.y), Theme::Col_Text, text);
            
            ImGui::Dummy(size);
        }

        bool SearchBar(const char* id, char* buffer, size_t bufferSize, const char* hint) {
            // Style overrides for this specific widget
            ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::Col_CardBg);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Theme::Radius_Large);
            
            bool ret = ImGui::InputTextWithHint(id, hint, buffer, bufferSize);
            
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            
            return ret;
        }

        void PushFontLarge() {
            ImGui::SetWindowFontScale(Theme::FontScaleLarge);
        }

        void PopFont() {
            ImGui::SetWindowFontScale(Theme::FontScale);
        }

    } // namespace Modern
} // namespace UI
