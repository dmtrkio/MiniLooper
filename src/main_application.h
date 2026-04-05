#pragma once

#include <memory>
#include <vector>

#include "looper/looper.h"
#include "midi/midi.h"
#include "ui/ui_window_base.h"

class MainApplication
{
public:
    MainApplication(int argc, const char * const *argv);
    ~MainApplication();

    MainApplication(const MainApplication &) = delete;
    MainApplication(const MainApplication &&) = delete;

    MainApplication &operator=(const MainApplication &) = delete;
    MainApplication &operator=(const MainApplication &&) = delete;

    void onFrame();

private:
    void drawTopBarMenu();

    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;

    std::vector<std::unique_ptr<ui::WindowBase>> windowRegistry_;
};