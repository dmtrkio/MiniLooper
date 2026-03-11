#pragma once

#include "dsp/parameter/audio_parameter.h"
#include "dsp/parameter/parameter_tree.h"

namespace ui {
    void parameterUi(dsp::parameter::Parameter &param);
    void parameterTreeUi(dsp::parameter::ParameterTree &paramTree, const std::string &prefix = "");
    dsp::parameter::ParameterTree testParameterTree();
}