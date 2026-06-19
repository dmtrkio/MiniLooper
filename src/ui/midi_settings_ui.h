#pragma once

#include "ui_window_base.h"
#include "midi/midi.h"

namespace ui {
    class MidiSettingsUi final : public WindowBase
    {
    public:
        explicit MidiSettingsUi(midi::MidiEngine *midiEngine);

        [[nodiscard]] const char* getTitle() const override;

    protected:
        void drawContent() override;

    private:
        midi::MidiEngine *midiEngine_ = nullptr;
    };
}