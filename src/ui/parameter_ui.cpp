#include "parameter_ui.h"

#include "imgui.h"

namespace ui {
    void parameterUi(dsp::parameter::Parameter &param)
    {
        using namespace dsp::parameter;

        ImGui::PushID(&param);

        switch (param.getType()) {
            case ParameterType::Float: {
                auto value = param.get<float>();
                const auto [min, max] = *param.getRange<float>();
                if (ImGui::SliderFloat(param.getName().c_str(), &value, min, max)) {
                    param.set(value);
                }
                break;
            }
            case ParameterType::Integer: {
                auto value = param.get<std::int32_t>();
                const auto [min, max] = *param.getRange<std::int32_t>();
                if (ImGui::SliderInt(param.getName().c_str(), &value, min, max)) {
                    param.set(value);
                }
                break;
            }
            case ParameterType::Boolean: {
                bool value = param.get<bool>();
                if (ImGui::Checkbox(param.getName().c_str(), &value)) {
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

    static void traverseParameterTree(dsp::parameter::ParameterTree &paramTree)
    {
        if (paramTree.isParameter()) {
            parameterUi(paramTree.asParameterUnsafe());
        } else {
            ImGui::PushID(&paramTree);

            if (ImGui::CollapsingHeader(paramTree.getName().c_str())) {
                ImGui::Indent();
                paramTree.forEachChild(traverseParameterTree);
                ImGui::Unindent();
            }

            ImGui::PopID();
        }
    }

    void parameterTreeUi(dsp::parameter::ParameterTree &paramTree, bool *opened, const std::string& prefix)
    {
        if ((opened != nullptr) && (!(*opened))) return;

        ImGui::PushID(&paramTree);
        const std::string title = prefix + paramTree.getName();

        if (opened == nullptr) {
            ImGui::Begin(title.c_str());
        } else {
            ImGui::Begin(title.c_str(), opened);
        }

        if (paramTree.isSubTree()) {
            paramTree.forEachChild(traverseParameterTree);
        } else {
            parameterUi(paramTree.asParameterUnsafe());
        }

        ImGui::End();
        ImGui::PopID();
    }

    dsp::parameter::ParameterTree testParameterTree()
    {
        using namespace dsp::parameter;
        ParameterTree paramTree{"Test Parameter Tree"};
        paramTree.addParameters({Parameter::makeBoolean("On", false)});

        ParameterTree group1{"Group 1"};
        group1.addParameter(Parameter::makeFloat("Gain", 50.0f, dsp::Range{0.0f, 100.0f}));
        group1.addParameter(Parameter::makeFloat("Mix", 50.0f, dsp::Range{0.0f, 100.0f}));
        auto *g1 = paramTree.addSubTree(std::move(group1));

        g1->addSubTree(ParameterTree{"Nested", {
            Parameter::makeInteger("Count", 0, dsp::Range{0, 10}),
            Parameter::makeFloat("Pan", 0.0f, dsp::Range{-1.0f, 1.0f}),
        }});

        ParameterTree group2{"Group 2"};
        group2.addParameter(Parameter::makeFloat("Cutoff", 50.0f, dsp::Range{0.0f, 100.0f}));
        group2.addParameter(Parameter::makeFloat("Q", 50.0f, dsp::Range{0.0f, 100.0f}));
        paramTree.addSubTree(std::move(group2));

        return paramTree;
    }

}
