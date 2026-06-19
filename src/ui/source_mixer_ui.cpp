#include "source_mixer_ui.h"

#include "parameter_ui.h"

namespace ui {
    SourceMixerUi::SourceMixerUi(looper::Looper &looper)
        : looper_(&looper)
    {}

    const char* SourceMixerUi::getTitle() const { return "Source Mixer"; }

    void SourceMixerUi::drawContent()
    {
        const auto paramTree = looper_->getParameterTree()["SourceMixer"];
        // temporary view
        parameterTreeUi(paramTree);
    }
}