#pragma once

#include "imgui.h"

#include "ui_window_base.h"
#include "midi/midi.h"

namespace ml::ui {
    class MidiSettingsUi final : public WindowBase
    {
    public:
        explicit MidiSettingsUi(midi::MidiEngine &midiEngine);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        midi::MidiEngine *midiEngine_ = nullptr;
    };
}