#pragma once

#include <memory>

#include "looper/looper.h"
#include "midi/midi.h"

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
    looper::Looper looper_;
    std::unique_ptr<midi::MidiEngine> midiEngine_;
};