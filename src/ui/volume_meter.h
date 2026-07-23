#pragma once

namespace ml::ui {
    void volumeMeter(const float leftDb, const float rightDb, float minDb = -60.0f, float maxDb = 6.0f);
}