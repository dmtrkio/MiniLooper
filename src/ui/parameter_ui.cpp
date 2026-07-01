#include "parameter_ui.h"

#include "dsp/parameter/parameter_tree.h"
#include "toggle_widget.h"

#include "imgui.h"

namespace ml::ui {
    void parameterUi(dsp::parameter::Parameter &param, const char* nameOverride, ParameterUiFlags flags)
    {
        using namespace dsp::parameter;

        ImGui::PushID(&param);

        const char* name = nameOverride ? nameOverride : param.getName().c_str();

        switch (param.getType()) {
            case ParameterType::Float: {
                auto value = param.get<float>();
                const auto [min, max] = *param.getRange<float>();
                if (flags & kParameterUiFlags_DragForRanged) {
                    if (ImGui::DragFloat(name, &value, 0.01f, min, max)) {
                        param.set(value);
                    }
                    break;
                }
                if (ImGui::SliderFloat(name, &value, min, max)) {
                    param.set(value);
                }
                break;
            }
            case ParameterType::Integer: {
                auto value = param.get<std::int32_t>();
                const auto [min, max] = *param.getRange<std::int32_t>();
                if (flags & kParameterUiFlags_DragForRanged) {
                    if (ImGui::DragInt(name, &value, 1.0f, min, max)) {
                        param.set(value);
                    }
                    break;
                }
                if (ImGui::SliderInt(name, &value, min, max)) {
                    param.set(value);
                }
                break;
            }
            case ParameterType::Boolean: {
                bool value = param.get<bool>();
                if (toggle(name, value)) {
                    param.set(value);
                }
                break;
            }
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
            ImGui::OpenPopup("ParameterContext");
        }

        if (ImGui::BeginPopup("ParameterContext")) {
            if (ImGui::MenuItem("Set to Default")) {
                param.setToDefault();
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    static void traverseParameterTree(dsp::parameter::ParameterTree paramTree)
    {
        if (paramTree.isParameter()) {
            parameterUi(paramTree.asParameterUnsafe());
        } else {
            ImGui::PushID(paramTree.getName().c_str());

            if (ImGui::CollapsingHeader(paramTree.getName().c_str())) {
                ImGui::Indent();
                paramTree.forEachChild(traverseParameterTree);
                ImGui::Unindent();
            }

            ImGui::PopID();
        }
    }

    void parameterTreeUi(dsp::parameter::ParameterTree paramTree)
    {
        if (paramTree.isSubTree()) {
            ImGui::PushID(paramTree.getName().c_str());
            paramTree.forEachChild(traverseParameterTree);
            ImGui::PopID();
        } else {
            parameterUi(paramTree.asParameterUnsafe());
        }
    }

    void parameterTreeUiWindowed(dsp::parameter::ParameterTree paramTree, bool &opened, const std::string& prefix)
    {
        if (!opened) return;

        ImGui::PushID(&paramTree);
        const std::string title = prefix + paramTree.getName();

        ImGui::Begin(title.c_str(), &opened);

        parameterTreeUi(paramTree);

        ImGui::End();
        ImGui::PopID();
    }
}