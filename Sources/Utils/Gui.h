#pragma once

#include <imgui.h>

#include "Core/Enum.h"
namespace Rastery {
template <typename T, std::enable_if_t<has_enum_info_v<T>, bool> = true>
bool dropdown(const char label[], T& var, bool sameLine = false) {
    bool changed = false;
    if (sameLine) ImGui::SameLine();
    const std::string& currentValue = enumToString(var);
    if (ImGui::BeginCombo(label, currentValue.c_str())) {
        const auto& items = EnumInfo<T>::items();
        for (const auto& item : items) {
            bool selected = var == item.first;
            if (ImGui::Selectable(item.second.c_str(), &selected)) {
                var = item.first;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}
}  // namespace Rastery
