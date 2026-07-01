#pragma once

#include "dsp/parameter/audio_parameter.h"
#include "dsp/parameter/parameter_tree.h"
#include "ui/ui_window_base.h"

namespace ml::ui {
    enum ParameterUiFlags : std::uint32_t {
        kParameterUiFlags_None = 0,
        kParameterUiFlags_DragForRanged = 1 << 0,
    };

    void parameterUi(
        dsp::parameter::Parameter &param,
        const char* nameOverride = nullptr,
        ParameterUiFlags flags = kParameterUiFlags_None
    );

    void parameterTreeUi(dsp::parameter::ParameterTree paramTree);

    void parameterTreeUiWindowed(dsp::parameter::ParameterTree paramTree, bool &opened, const std::string &prefix = "");

    class ParameterTreeUi final : public WindowBase
    {
    public:
        explicit ParameterTreeUi(dsp::parameter::ParameterTree paramTree, const std::string &prefix = "")
            : title_(prefix + paramTree.getName())
            , paramTree_(std::move(paramTree)) {}

        [[nodiscard]] const char* getTitle() const override { return title_.c_str(); }

    protected:
        void drawContent() override
        {
            parameterTreeUi(paramTree_);
        }

    private:
        std::string title_;
        dsp::parameter::ParameterTree paramTree_;
    };
}